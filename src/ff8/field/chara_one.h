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

#pragma once

#include "../../common.h"

#include <vector>
#include <unordered_map>

struct CharaOneModel {
	char name[6];
	bool isMch;
	std::vector<uint32_t> texturesData;
};

std::unordered_map<uint32_t, CharaOneModel> ff8_chara_one_parse_models(const uint8_t *chara_one_data, size_t size);
void ff8_mch_parse_model(CharaOneModel &model, const uint8_t *mch_data, size_t size);
bool ff8_chara_one_model_save_textures(const CharaOneModel &models, const uint8_t *chara_one_model_data, const char *dirname);
