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

#pragma once

#include <stdint.h>
#include "../ff8.h"

inline uint32_t fromR5G5B5Color(uint16_t color)
{
	uint8_t r = color & 31,
		g = (color >> 5) & 31,
		b = (color >> 10) & 31;

	return (0xffu << 24) |
		((((r << 3) + (r >> 2)) & 0xffu) << 16) |
		((((g << 3) + (g >> 2)) & 0xffu) << 8) |
		(((b << 3) + (b >> 2)) & 0xffu);
}

class Tim;

class PaletteDetectionStrategy {
public:
	PaletteDetectionStrategy(const Tim *const tim) : _tim(tim) {};
	virtual bool isValid() const {
		return true;
	}
	virtual uint32_t palOffset(uint16_t x, uint16_t y) const = 0;
protected:
	const Tim *const _tim;
};

class PaletteDetectionStrategyFixed : public PaletteDetectionStrategy {
public:
	PaletteDetectionStrategyFixed(const Tim *const tim, uint16_t palX, uint16_t palY) :
		PaletteDetectionStrategy(tim), _palX(palX), _palY(palY) {}
	virtual uint32_t palOffset(uint16_t imgX, uint16_t imgY) const override;
private:
	uint16_t _palX, _palY;
};

class PaletteDetectionStrategyGrid : public PaletteDetectionStrategy {
public:
	PaletteDetectionStrategyGrid(const Tim *const tim, uint8_t cellCols, uint8_t cellRows);
	virtual bool isValid() const override;
	virtual uint32_t palOffset(uint16_t imgX, uint16_t imgY) const override;
private:
	uint8_t _cellCols, _cellRows;
	uint8_t _palCols, _colorPerPal;
};

class Tim {
	friend class PaletteDetectionStrategyFixed;
	friend class PaletteDetectionStrategyGrid;
public:
	Tim(uint8_t bpp, const ff8_tim &tim);
	uint16_t colorsPerPal() const;
	bool toRGBA32(uint32_t *target, uint8_t palX = 0, uint8_t palY = 0) const;
	bool toRGBA32MultiPaletteGrid(uint32_t *target, uint8_t cellCols, uint8_t cellRows) const;
	void save(const char *fileName, uint8_t palX = 0, uint8_t palY = 0) const;
	static Tim fromLzs(uint8_t *uncompressed_data);
private:
	bool toRGBA32(uint32_t *target, PaletteDetectionStrategy *paletteDetectionStrategy = nullptr) const;
	ff8_tim _tim;
	uint8_t _bpp;
};
