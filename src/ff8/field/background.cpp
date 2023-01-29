/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2023 myst6re                                            //
//    Copyright (C) 2023 Julian Xhokaxhiu                                   //
//    Copyright (C) 2023 Cosmos                                             //
//    Copyright (C) 2023 Tang-Tang Zhou                                     //
//                                                                          //
//    This file is part of FFNx                                             //
//                                                                          //
//    FFNx is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU General Public License as published by  //
//    the Free Software Foundation, either version 3 of the License         //
//                                                                          //
//    FFNx is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of        //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
//    GNU General Public License for more details.                          //
/****************************************************************************/

#include "background.h"
#include "../../image/tim.h"
#include "../../saveload.h"
#include "../../log.h"
#include <unordered_map>

constexpr int TEXTURE_WIDTH_BYTES = 128; // Real texture width depends on the texture depth (bpp4 => 256, bpp8 => 128, bpp16 => 64)
constexpr int TEXTURE_WIDTH_BPP4 = 256;
constexpr int TEXTURE_HEIGHT = 256;
constexpr int VRAM_PAGE_MIM_MAX_COUNT = 13;
constexpr int MIM_DATA_WIDTH_BYTES = TEXTURE_WIDTH_BYTES * VRAM_PAGE_MIM_MAX_COUNT;
constexpr int MIM_DATA_HEIGHT = TEXTURE_HEIGHT;
constexpr int TILE_SIZE = 16;
constexpr int PALETTE_SIZE = 256;

bool ff8_background_save_textures(const uint8_t *map_data, const uint8_t *mim_data, const char *filename)
{
	if (trace_all || trace_vram) ffnx_trace("%s %s\n", __func__, filename);

	std::unordered_map<uint16_t, Tile> tiles_per_position_in_texture;
	std::unordered_map<uint8_t, Tim::Bpp> min_depths_per_texture_id;

	const uint16_t *palettes_data = reinterpret_cast<const uint16_t *>(mim_data + 0x1000);
	const uint8_t *textures_data = mim_data + 0x3000;

	// Parse MAP tiles
	while (true) {
		Tile tile;

		memcpy(&tile, map_data, sizeof(Tile));

		if (tile.x == 0x7fff) {
			break;
		}

		uint8_t texture_id = tile.texID & 0xF;
		Tim::Bpp bpp = Tim::Bpp((tile.texID >> 7) & 3);

		ffnx_info("dst %d %d %d src %d %d texid %d bpp %d\n", tile.x, tile.y, tile.z, tile.srcX, tile.srcY, texture_id, int(bpp));

		tiles_per_position_in_texture[texture_id | ((tile.srcX / 16) << 4) | ((tile.srcY / 16) << 8)] = tile;

		auto it = min_depths_per_texture_id.find(texture_id);
		if (it == min_depths_per_texture_id.end()) {
			min_depths_per_texture_id[texture_id] = bpp;
		} else {
			min_depths_per_texture_id[texture_id] = Tim::Bpp(std::min(int(bpp), int(it->second)));
		}

		map_data += sizeof(Tile);
	}

	uint32_t* const image_data_start = new uint32_t[TEXTURE_WIDTH_BPP4 * TEXTURE_HEIGHT];

	if (image_data_start == nullptr) {
		return false;
	}

	// Save textures
	for (const std::pair<uint8_t, Tim::Bpp> &pair: min_depths_per_texture_id) {
		const uint8_t texture_id = pair.first;
		const Tim::Bpp min_depth = pair.second;
		const uint16_t width = TEXTURE_WIDTH_BPP4 >> min_depth;
		const uint32_t image_data_size = width * TEXTURE_HEIGHT * sizeof(uint32_t);

		// Fill with zeroes (transparent image)
		memset(image_data_start, 0, image_data_size);

		char filename_tex[MAX_PATH] = {};

		snprintf(filename_tex, sizeof(filename_tex), "%s_%d", filename, texture_id);

		for (uint8_t row = 0; row < 16; ++row) {
			for (uint8_t col = 0; col < width / 16; ++col) {
				auto it = tiles_per_position_in_texture.find(texture_id | (col << 4) | (row << 8));
				if (it == tiles_per_position_in_texture.end()) {
					ffnx_info("texture_id=%d row=%d col=%d not found\n", texture_id, row, col);
					continue;
				}

				ffnx_info("texture_id=%d row=%d col=%d\n", texture_id, row, col);

				const Tile &tile = it->second;
				Tim::Bpp bpp = Tim::Bpp((tile.texID >> 7) & 3);
				uint8_t pal_id = (tile.palID >> 6) & 0xF;
				const uint8_t *texture_data_start = textures_data + texture_id * TEXTURE_WIDTH_BYTES + tile.srcY * MIM_DATA_WIDTH_BYTES;
				const uint16_t *palette_data_start = bpp == Tim::Bpp16 ? nullptr : palettes_data + pal_id * PALETTE_SIZE;
				uint32_t *target = image_data_start + row * TILE_SIZE * width;

				ffnx_info("bpp=%d pal_id=%d srcX=%d srcY=%d palette_data_start=%X texture_data_start=%X\n", bpp, pal_id, tile.srcX,tile.srcY, int(palette_data_start), int(texture_data_start));

				if (bpp == Tim::Bpp16) {
					const uint16_t *texture_data = reinterpret_cast<const uint16_t *>(texture_data_start) + tile.srcX;
					target += col * TILE_SIZE;

					for (int y = 0; y < TILE_SIZE; ++y) {
						for (int x = 0; x < TILE_SIZE; ++x) {
							*(target + x) = fromR5G5B5Color(*(texture_data + x), true);
						}

						target += width;
						texture_data += MIM_DATA_WIDTH_BYTES / 2;
					}
				} else if (bpp == Tim::Bpp8) {
					const uint8_t *texture_data = texture_data_start + tile.srcX;
					target += col * TILE_SIZE;

					for (int y = 0; y < TILE_SIZE; ++y) {
						for (int x = 0; x < TILE_SIZE; ++x) {
							*(target + x) = fromR5G5B5Color(palette_data_start[*(texture_data + x)], true);
						}

						target += width;
						texture_data += MIM_DATA_WIDTH_BYTES;
					}
				} else {
					const uint8_t *texture_data = texture_data_start + tile.srcX / 2;
					target += col * TILE_SIZE;

					for (int y = 0; y < TILE_SIZE; ++y) {
						for (int x = 0; x < TILE_SIZE / 2; ++x) {
							uint8_t index = *(texture_data + x);
							*(target + x * 2) = fromR5G5B5Color(palette_data_start[index & 0xF], true);
							*(target + x * 2 + 1) = fromR5G5B5Color(palette_data_start[index >> 4], true);
						}

						target += width;
						texture_data += MIM_DATA_WIDTH_BYTES;
					}
				}
			}
		}

		save_texture(image_data_start, image_data_size, width, TEXTURE_HEIGHT, 0, filename_tex, false);
	}

	delete[] image_data_start;

	return true;
}
