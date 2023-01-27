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

#include "../../common.h"

#pragma once

struct Tile {
	int16_t x, y, z;
	uint16_t texID; // 2 bits = depth | 2 bits = blend | 1 bit = draw | 4 bits = textureID
	uint16_t palID; // 6 bits = Always 30 | 4 bits = PaletteID | 6 bits = Always 0
	uint8_t srcX, srcY;
	uint8_t layerID; // 0-7
	uint8_t blendType; // 0-4
	uint8_t parameter, state;
};

bool ff8_background_save_textures(const uint8_t *map_data, const uint8_t *mim_data, const char *filename);
