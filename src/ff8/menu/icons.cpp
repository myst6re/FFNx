/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2025 myst6re                                            //
//    Copyright (C) 2025 Julian Xhokaxhiu                                   //
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

#include "icons.h"
#include "../../image/tim.h"
#include "../../log.h"
#include "../../saveload.h"

#include <map>
#include <set>

unsigned int *ff8_draw_icon_get_icon_sp1_infos(int icon_id, int &states_count)
{
	int *icon_sp1_data = ((int*(*)())ff8_externals.get_icon_sp1_data)();

	if (icon_id >= icon_sp1_data[0])
	{
		states_count = 0;

		return nullptr;
	}

	states_count = HIWORD(icon_sp1_data[icon_id + 1]);

	return (unsigned int *)((char *)icon_sp1_data + uint16_t(icon_sp1_data[icon_id + 1]));
}

struct sp1Entry {
	uint8_t x, y;
	uint16_t palID;
	uint8_t w, field5, h, field7;
};

bool ff8_icons_save_textures(const unsigned char *image_data, const uint32_t *palette_data, uint32_t width, uint32_t height, const char *prefix)
{
	if (trace_all || trace_vram) ffnx_trace("%s: %s\n", __func__, prefix);

	std::set<TimRect> rects;
	std::set<uint8_t> palettes;

	// Static rules
	rects.insert(TimRect(1, 0, 48, 255, 63)); // Window background 1
	rects.insert(TimRect(1, 0, 64, 111, 79)); // Window background 2
	rects.insert(TimRect(5, 0, 16, 7, 21)); // Progress bar
	rects.insert(TimRect(13, 168, 24, 246, 31)); // Numbers
	rects.insert(TimRect(13, 160, 97, 173, 107)); // Lv. 1
	rects.insert(TimRect(13, 240, 144, 255, 151)); // Lv. 2
	rects.insert(TimRect(13, 232, 152, 255, 159)); // %/:
	rects.insert(TimRect(13, 256, 256, 139, 199)); // Some texts
	rects.insert(TimRect(13, 56, 216, 62, 223)); // A
	rects.insert(TimRect(2, 128, 96, 143, 107)); // Elemental icon
	rects.insert(TimRect(2, 144, 96, 159, 107)); // Elemental icon
	rects.insert(TimRect(2, 144, 112, 159, 123)); // Mental icon
	rects.insert(TimRect(2, 144, 240, 159, 251)); // Mental icon

	for (int icon_id = 0; ; ++icon_id) {
		int states_count = 0;
		sp1Entry *data = (sp1Entry *)ff8_draw_icon_get_icon_sp1_infos(icon_id, states_count);

		if (data == nullptr) {
			break;
		}

		for (int state = 0; state < states_count; ++state) {
			uint8_t pal_id = (data->palID >> 6) & 0xF;
			ffnx_info("%s: icon=%d state=%d field0=%X pal_id=%d %d %d rect=(%d, %d, %d, %d)\n", __func__, icon_id, state, data->palID, pal_id, data->field5, data->field7, data->x, data->y, data->w, data->h);

			palettes.insert(pal_id);

			if (data->x == 0 && data->y == 0 && data->w == 8 && data->h == 8 || pal_id == 0) {
				continue;
			}

			rects.insert(TimRect(pal_id, data->x, data->y, data->x + data->w - 1, data->y + data->h - 1));

			data++;
		}
	}

	uint32_t *converted_image_data = new uint32_t[width * height];

	for (const uint8_t pal_id: palettes) {
		uint32_t o = 0;
		memset(converted_image_data, 0, width * height * sizeof(uint32_t));
		std::set<TimRect> to_delete;

		for(uint32_t y = 0; y < height; y++) {
			for(uint32_t x = 0; x < width; x++) {
				bool found = false;
				for (const TimRect &rect: rects) {
					if (rect.match(x, y) && (pal_id == 0 || pal_id == rect.palIndex)) {
						converted_image_data[o] = palette_data[rect.palIndex * 16 + image_data[o]];
						to_delete.insert(rect);
						found = true;
						break;
					}
				}
				if (!found && pal_id == 0) {
					converted_image_data[o] = palette_data[0 * 16 + image_data[o]];
				}

				o++;
			}
		}

		if (!to_delete.empty()) {
			save_texture(converted_image_data, width * height * sizeof(uint32_t), width, height, pal_id, prefix, false);

			for (const TimRect &rect: to_delete) {
				rects.erase(rect);
			}
		}

	}

	return true;
}
