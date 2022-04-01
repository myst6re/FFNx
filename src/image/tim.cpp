/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2022 myst6re                                            //
//    Copyright (C) 2022 Julian Xhokaxhiu                                   //
//    Copyright (C) 2022 Tang-Tang Zhou                                     //
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

#include "tim.h"

#include "../common.h"
#include "../log.h"
#include "../saveload.h"

Tim::Tim(uint8_t bpp, const ff8_tim &tim) :
	_bpp(bpp), _tim(tim)
{
	if (_bpp == 0)
	{
		_tim.img_w *= 4;
	}
	else if (_bpp == 1)
	{
		_tim.img_w *= 2;
	}
}

uint32_t PaletteDetectionStrategyFixed::palOffset(uint16_t, uint16_t) const
{
	return _palX + _palY * _tim->_tim.pal_w;
}

bool Tim::toRGBA32(uint32_t *target, uint8_t palX, uint8_t palY) const
{
	PaletteDetectionStrategyFixed fixed(this, palX, palY);
	return fixed.isValid() && toRGBA32(target, &fixed);
}

PaletteDetectionStrategyGrid::PaletteDetectionStrategyGrid(const Tim *const tim, uint8_t cellCols, uint8_t cellRows) :
	PaletteDetectionStrategy(tim), _cellCols(cellCols), _cellRows(cellRows), _palCols(1)
{
	_colorPerPal = tim->colorsPerPal();

	if (_colorPerPal > 0 && tim->_tim.pal_w > _colorPerPal)
	{
		_palCols = _tim->_tim.pal_w / _colorPerPal;
	}
}

bool PaletteDetectionStrategyGrid::isValid() const
{
	if (_tim->_tim.img_w % _cellCols != 0)
	{
		ffnx_error("PaletteDetectionStrategyGrid::%s img_w=%d % cellCols=%d != 0\n", __func__, _tim->_tim.img_w, _cellCols);
		return false;
	}

	if (_tim->_tim.pal_w % _colorPerPal == 0)
	{
		ffnx_error("PaletteDetectionStrategyGrid::%s pal width=%d not a multiple of %d\n", __func__, _tim->_tim.pal_w, _colorPerPal);
		return false;
	}

	if (_tim->_tim.pal_h * _palCols != _cellCols * _cellRows)
	{
		ffnx_error("PaletteDetectionStrategyGrid::%s not enough palette for this image\n", __func__);
		return false;
	}

	return true;
}

uint32_t PaletteDetectionStrategyGrid::palOffset(uint16_t imgX, uint16_t imgY) const
{
	// Direction: top to bottom then left to right
	uint16_t cellX = imgX / _cellCols, cellY = imgY / _cellRows;
	uint16_t palX = (cellX % _palCols) * _colorPerPal, palY = (cellX / _palCols) * _cellRows + cellY;

	return palX + palY * _tim->_tim.pal_w;
}

bool Tim::toRGBA32MultiPaletteGrid(uint32_t *target, uint8_t cellCols, uint8_t cellRows) const
{
	PaletteDetectionStrategyGrid grid(this, cellCols, cellRows);
	return grid.isValid() && toRGBA32(target, &grid);
}

bool Tim::toRGBA32(uint32_t *target, PaletteDetectionStrategy *paletteDetectionStrategy) const
{
	if (_tim.img_data == nullptr)
	{
		ffnx_error("%s img_data is null\n", __func__);

		return false;
	}

	if (_bpp == 0)
	{
		if (_tim.pal_data == nullptr || paletteDetectionStrategy == nullptr)
		{
			ffnx_error("%s bpp 0 without palette\n", __func__);

			return false;
		}

		uint8_t *img_data = _tim.img_data;

		for (int y = 0; y < _tim.img_h; ++y)
		{
			for (int x = 0; x < _tim.img_w / 2; ++x)
			{
				*target = fromR5G5B5Color((_tim.pal_data + paletteDetectionStrategy->palOffset(x, y))[*_tim.img_data & 0xF]);

				++target;

				*target = fromR5G5B5Color((_tim.pal_data + paletteDetectionStrategy->palOffset(x + 1, y))[*_tim.img_data >> 4]);

				++target;
				++img_data;
			}
		}
	}
	else if (_bpp == 1)
	{
		if (_tim.pal_data == nullptr || paletteDetectionStrategy == nullptr)
		{
			ffnx_error("%s bpp 1 without palette\n", __func__);

			return false;
		}

		uint8_t *img_data = _tim.img_data;

		for (int y = 0; y < _tim.img_h; ++y)
		{
			for (int x = 0; x < _tim.img_w; ++x)
			{
				*target = fromR5G5B5Color((_tim.pal_data + paletteDetectionStrategy->palOffset(x, y))[*_tim.img_data]);

				++target;
				++img_data;
			}
		}
	}
	else if (_bpp == 2)
	{
		uint16_t *img_data16 = (uint16_t *)_tim.img_data;

		for (int y = 0; y < _tim.img_h; ++y)
		{
			for (int x = 0; x < _tim.img_w; ++x)
			{
				*target = fromR5G5B5Color(*img_data16);

				++target;
				++img_data16;
			}
		}
	}
	else
	{
		ffnx_error("%s unknown bpp %d\n", __func__, _bpp);

		return false;
	}

	return true;
}

void Tim::save(const char *fileName, uint8_t palX, uint8_t palY) const
{
	// allocate PBO
	uint32_t image_data_size = _tim.img_w * _tim.img_h * 4;
	uint32_t *image_data = (uint32_t*)driver_malloc(image_data_size);

	// convert source data
	if (image_data != nullptr)
	{
		// TODO: multiple palettes
		if (toRGBA32(image_data, palX, palY))
		{
			// TODO: is animated
			save_texture(image_data, image_data_size, _tim.img_w, _tim.img_h, palX + palY * _tim.pal_w, fileName, false);
		}

		driver_free(image_data);
	}
}

Tim Tim::fromLzs(uint8_t *uncompressed_data)
{
	uint16_t *header = (uint16_t *)uncompressed_data;
	ff8_tim tim_infos = ff8_tim();
	tim_infos.img_w = header[2];
	tim_infos.img_h = header[3];
	tim_infos.img_data = uncompressed_data + 8;

	return Tim(2, tim_infos);
}

uint16_t Tim::colorsPerPal() const
{
	if (_bpp == 1)
	{
		return 256;
	}
	else if (_bpp == 0)
	{
		return 16;
	}

	return 0;
}
