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

bool ff8_background_save_textures(const uint8_t *map_data, const uint8_t *mim_data, const char *filename)
{
	if (trace_all || trace_vram) ffnx_trace("%s %s\n", __func__, filename);

	std::unordered_map<uint16_t, Tile> tilesPerPositionInTexture;
	uint8_t hasBpp = 0;
	const uint16_t *palettes_data = (const uint16_t *)(mim_data + 0x1000);
	const uint8_t *textures_data = mim_data + 0x3000;

	while (true) {
		Tile tile;

		memcpy(&tile, map_data, sizeof(Tile));

		if (tile.x == 0x7fff) {
			break;
		}

		uint8_t texture_id = tile.texID & 0xF;
		uint8_t bpp = (tile.texID >> 7) & 3;

		ffnx_info("dst %d %d %d src %d %d texid %d\n", tile.x, tile.y, tile.z, tile.srcX, tile.srcY, texture_id);

		tilesPerPositionInTexture[texture_id | ((tile.srcX / 16) << 4) | ((tile.srcY / 16) << 8)] = tile;

		hasBpp |= 1 << bpp;

		map_data += sizeof(Tile);
	}

	for (uint8_t bpp = 0; bpp < 3; ++bpp) {
		if (! (hasBpp & (1 << bpp))) {
			continue;
		}

		for (int row = 0; row < 16; ++row) {
			uint8_t width = 13 * 64 * (4 >> bpp);

			ffnx_info("row=%d bpp=%d\n", row, bpp);
			// allocate PBO
			uint32_t image_data_size = width * 16 * sizeof(uint32_t);
			uint32_t *image_data = (uint32_t*)driver_calloc(image_data_size, 1); // Set to 0

			if (image_data == nullptr) {
				return false;
			}

			uint32_t *target = image_data;
			char filename_tex[MAX_PATH] = {};

			snprintf(filename_tex, sizeof(filename_tex), "%s-%d-%d", filename, bpp, row);

			for (uint8_t texture_id = 0; texture_id < 13; ++texture_id) {
				for (uint8_t col = 0; col < (4 << (2 - bpp)); ++col) {
					auto it = tilesPerPositionInTexture.find(texture_id | (col << 4) | (row << 8));
					if (it == tilesPerPositionInTexture.end()) {
						continue;
					}

					const Tile &tile = it->second;
					uint8_t bpp_tile = (tile.texID >> 7) & 3;

					if (bpp_tile != bpp) {
						ffnx_error("%s: inconsistent bpp between texture and tile %d != %d\n", __func__, bpp, bpp_tile);

						return false;
					}

					uint8_t pal_id = (tile.palID >> 6) & 0xF;
					const uint8_t *texture_data_start = textures_data + texture_id * 128 + tile.srcY * 1664;
					const uint16_t *palette_data_start = palettes_data + pal_id * 256;

					if (bpp == 2) {
						const uint16_t *texture_data = (const uint16_t *)texture_data_start + tile.srcX;

						for (int y = 0; y < 16; ++y) {
							for (int x = 0; x < 16; ++x) {
								*(target + x) = fromR5G5B5Color(*(texture_data + x), false);
							}

							target += width;
							texture_data += 1664;
						}
					} else if (bpp == 1) {
						const uint8_t *texture_data = texture_data_start + tile.srcX;

						for (int y = 0; y < 16; ++y) {
							for (int x = 0; x < 16; ++x) {
								*(target + x) = fromR5G5B5Color(palette_data_start[*(texture_data + x)], false);
							}

							target += width;
							texture_data += 1664;
						}
					} else {
						const uint8_t *texture_data = texture_data_start + tile.srcX / 2;

						for (int y = 0; y < 16; ++y) {
							for (int x = 0; x < 16 / 2; ++x) {
								uint8_t index = *(texture_data + x);
								*(target + x) = fromR5G5B5Color(palette_data_start[index & 0xF], false);
								*(target + x + 1) = fromR5G5B5Color(palette_data_start[index >> 4], false);
							}

							target += width;
							texture_data += 1664 / 2;
						}
					}
				}
			}

			save_texture(image_data, image_data_size, width, 16, 0, filename_tex, false);

			driver_free(image_data);
		}
	}



	return true;
}
