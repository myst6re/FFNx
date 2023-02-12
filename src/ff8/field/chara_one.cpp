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

#include "chara_one.h"
#include "../../image/tim.h"
#include "../../saveload.h"
#include "../../log.h"

std::vector<Model> ff8_chara_one_parse_models(const uint8_t *chara_one_data, size_t size)
{
	std::vector<Model> models;

	const uint8_t *cur = chara_one_data;

	uint32_t count;
	memcpy(&count, cur, 4);
	cur += 4;

	for (uint32_t i = 0; i < count && cur - chara_one_data < size - 16; ++i) {
		uint32_t offset;
		memcpy(&offset, cur, 4);
		cur += 4;

		if (offset == 0) {
			break;
		}

		uint32_t section_size;
		memcpy(&section_size, cur, 4);
		cur += 4;

		uint32_t flag;
		memcpy(&flag, cur, 4);
		cur += 4;

		if (flag == section_size) {
			memcpy(&flag, cur, 4);
			cur += 4;
		}

		Model model = Model();

		if (flag >> 24 != 0xd0) { // NPCs (not main characters)
			uint32_t timOffset;

			ffnx_info("%s: %d %d %d\n", __func__, i, int(cur - chara_one_data), size);

			if ((flag & 0xFFFFFF) == 0) {
				model.texturesData.push_back(chara_one_data + offset + 4);
			}

			while (cur - chara_one_data < size) {
				memcpy(&timOffset, cur, 4);
				cur += 4;
				ffnx_info("%s: %d timOffset=0x%X\n", __func__, i, timOffset);

				if (timOffset == 0xFFFFFFFF) {
					break;
				}

				model.texturesData.push_back(chara_one_data + offset + 4 + (timOffset & 0xFFFFFF));
			}
		}

		if (cur - chara_one_data < 16) {
			break;
		}

		cur += 4;
		char name[5] = "";
		for (uint8_t j = 0; j < 4; ++j) {
			if (cur[j] > 'z' || cur[j] < '0') {
				name[j] = '\0';
				break;
			}

			name[j] = cur[j];
		}
		strncpy(model.name, name, 4);
		ffnx_info("%s: %d name=%s\n", __func__, i, model.name);

		models.push_back(model);
		cur += 12;
	}

	return models;
}

bool ff8_chara_one_save_textures(const std::vector<Model> &models, const char *filename)
{
	if (trace_all || trace_vram) ffnx_trace("%s %s\n", __func__, filename);

	for (const Model &model: models) {
		int textureId = 0;
		for (const uint8_t *texturePointer: model.texturesData) {
			ffnx_info("%s: texturePointer=0x%X\n", __func__, texturePointer);
			ffnx_info("%X %X %X %X\n", *(uint32_t *)(texturePointer - 4), *(uint32_t *)texturePointer, *(uint32_t *)(texturePointer + 4), *(uint32_t *)(texturePointer + 8));
			char name[MAX_PATH] = {};
			snprintf(name, sizeof(name), "%s-%s-%d", filename, model.name, textureId);
			if (!Tim::fromTimData(texturePointer).save(name)) {
				return false;
			}
			++textureId;
		}
	}

	return true;
}
