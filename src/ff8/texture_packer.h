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

#include <string>
#include <map>
#include <unordered_set>
#include <bimg/bimg.h>

#include "../ff8.h"
#include "../image/tim.h"

typedef uint32_t ModdedTextureId;

#define VRAM_WIDTH 1024
#define VRAM_HEIGHT 512
#define VRAM_DEPTH 2
#define INVALID_TEXTURE ModdedTextureId(0xFFFFFFFF)
#define MAX_SCALE 10

class TexturePacker {
public:
	explicit TexturePacker();
	inline void setVram(uint8_t *vram) {
		_vram = vram;
	}
	void setTexture(const char *name, const uint8_t *texture, int x, int y, int w, int h, uint8_t bpp, bool isPal);
	bool hasTexture(const char *name, int x, int y, int w, int h);
	inline uint8_t getMaxScale() const {
		return _maxScaleCached;
	}
	void registerTiledTex(uint8_t *target, int x, int y, uint8_t bpp);
	bool drawModdedTextures(const uint8_t *texData, const uint32_t *paletteData, uint32_t paletteIndex, uint32_t paletteEntries, uint32_t *target, int targetW, int targetH, uint8_t scale);

	void saveVram(const char *fileName, uint8_t bpp) const;
private:
	inline uint8_t *vramSeek(int x, int y) const {
		return _vram + VRAM_DEPTH * (x + y * VRAM_WIDTH);
	}
	void updateMaxScale();
	class TextureInfos {
	public:
		TextureInfos();
		TextureInfos(
			int x, int y, int w, int h,
			uint8_t bpp
		);
		inline int x() const {
			return _x;
		}
		inline int y() const {
			return _y;
		}
		inline int w() const {
			return _w;
		}
		inline int h() const {
			return _h;
		}
		inline uint8_t bpp() const {
			return _bpp;
		}
	protected:
		int _x, _y;
		int _w, _h;
		uint8_t _bpp;
	};
	class Texture : public TextureInfos {
	public:
		Texture();
		Texture(
			const char *name,
			int x, int y, int w, int h,
			uint8_t bpp
		);
		inline const std::string &name() const {
			return _name;
		}
		uint8_t scale() const;
		bool createImage(uint8_t palette_index = 0);
		void destroyImage();
		inline bool hasImage() const {
			return _image != nullptr;
		}
		uint32_t getColor(int scaledX, int scaledY);
		inline bool hasPal() const {
			return _pal.bpp() != 255;
		}
		inline const TextureInfos &pal() const {
			return _pal;
		}
		inline void setPal(const TextureInfos &pal) {
			_pal = pal;
		}
		Tim toTim(uint8_t *imgData, uint16_t *palData) const;
	private:
		bimg::ImageContainer *_image;
		std::string _name;
		TextureInfos _pal;
	};
	struct TiledTex {
		TiledTex();
		TiledTex(int x, int y, uint8_t bpp);
		int x, y;
		uint8_t bpp;
	};
	void getVramRect(uint8_t *target, const TextureInfos &texture) const;
	bool drawModdedTextures(uint32_t *target, const uint32_t *paletteData, const TiledTex &tiledTex, int w, int h, uint8_t scale, uint32_t paletteIndex, uint32_t paletteEntries);

	uint8_t *_vram; // uint16_t[VRAM_WIDTH * VRAM_HEIGHT] aka uint8_t[VRAM_WIDTH * VRAM_HEIGHT * VRAM_DEPTH]
	std::map<const uint8_t *, TiledTex> _tiledTexs;
	ModdedTextureId _vramTextureIds[VRAM_WIDTH * VRAM_HEIGHT];
	std::map<ModdedTextureId, Texture> _moddedTextures;
	uint8_t _maxScaleCached;
};
