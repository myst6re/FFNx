/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2023 myst6re                                            //
//    Copyright (C) 2023 Julian Xhokaxhiu                                   //
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

#include <string>
#include <unordered_map>
#include <list>
#include <vector>

#include "../ff8.h"
#include "../image/tim.h"
#include "field/background.h"

typedef uint32_t ModdedTextureId;
typedef uint8_t VramPageId; // (y << 4) | x

constexpr int VRAM_WIDTH = 1024;
constexpr int VRAM_HEIGHT = 512;
constexpr int VRAM_PAGE_WIDTH = 64;
constexpr int VRAM_PAGE_HEIGHT = 256;
constexpr int VRAM_DEPTH = 2;
constexpr ModdedTextureId INVALID_TEXTURE = ModdedTextureId(0xFFFFFFFF);
constexpr int MAX_SCALE = 128;
constexpr int FF8_BASE_RESOLUTION_X = 320;

class ModdedTexture;

class TexturePacker {
public:
	class TextureInfos {
	public:
		TextureInfos();
		TextureInfos(
			int x, int y, int w, int h,
			Tim::Bpp bpp, bool multiBpp = false
		);
		inline bool isValid() const {
			return _x >= 0;
		}
		inline int x() const {
			return _x;
		}
		inline int pixelX() const {
			return _x * (4 >> _bpp);
		}
		inline int y() const {
			return _y;
		}
		inline int w() const {
			return _w;
		}
		inline int pixelW() const {
			return _w * (4 >> _bpp);
		}
		inline int h() const {
			return _h;
		}
		inline Tim::Bpp bpp() const {
			return _bpp;
		}
		inline bool hasMultiBpp() const {
			return _multiBpp;
		}
	private:
		int _x, _y;
		int _w, _h;
		Tim::Bpp _bpp;
		bool _multiBpp;
	};

	struct TiledTex {
		TiledTex();
		TiledTex(int x, int y, Tim::Bpp bpp, int palX, int palY);
		inline bool isValid() const {
			return x >= 0;
		}
		inline bool isPalValid() const {
			return palX >= 0 || bpp == Tim::Bpp16;
		}
		inline VramPageId vramPageId() const {
			return makeVramPageId(x, y);
		}
		int x, y;
		int palX, palY;
		Tim::Bpp bpp;
	};

	class IdentifiedTexture {
	public:
		IdentifiedTexture();
		IdentifiedTexture(
			const char *name,
			const TextureInfos &texture,
			const TextureInfos &palette = TextureInfos()
		);
		void setMod(ModdedTexture *mod);
		inline ModdedTexture *mod() const {
			return _mod;
		}
		inline const TextureInfos &texture() const {
			return _texture;
		}
		inline const TextureInfos &palette() const {
			return _palette;
		}
		inline const std::string &name() const {
			return _name;
		}
		inline bool isValid() const {
			return !_name.empty();
		}
	private:
		TextureInfos _texture, _palette;
		std::string _name;
		ModdedTexture *_mod;
	};

	enum TextureTypes {
		NoTexture = 0,
		ExternalTexture = 1,
		InternalTexture = 2,
		RemoveTexture = 4
	};

	explicit TexturePacker();
	bool setTexture(const char *name, const TextureInfos &texture, const TextureInfos &palette = TextureInfos());
	bool setTextureBackground(const char *name, int x, int y, int w, int h, const std::vector<Tile> &mapTiles, int bgTexId = -1, const char *extension = nullptr, char *found_extension = nullptr);
	// Override a part of the VRAM from another part of the VRAM, typically with biggest textures (Worldmap)
	bool setTextureRedirection(const TextureInfos &oldTexture, const TextureInfos &newTexture, uint32_t *imageData);
	void animateTextureByCopy(int sourceXBpp2, int y, int sourceWBpp2, int sourceH, int targetXBpp2, int targetY);
	void clearTiledTexs();
	void clearTextures();
	// Returns the textures matching the tiledTex
	std::list<IdentifiedTexture> matchTextures(const TiledTex &tiledTex, bool withModsOnly = false, bool notDrawnOnly = false) const;
	void registerTiledTex(const uint8_t *texData, int x, int y, Tim::Bpp bpp, int palX = -1, int palY = -1);
	void registerPaletteWrite(const uint8_t *texData, int palX, int palY);
	TiledTex getTiledTex(const uint8_t *texData) const;

	TextureTypes drawTextures(
		const uint8_t *texData, uint32_t *rgbaImageData, uint32_t dataSize, int originalW, int originalH,
		uint8_t *outScale, uint32_t **outTarget
	);

	static void debugSaveTexture(int textureId, const uint32_t *source, int w, int h, bool removeAlpha, bool after, TextureTypes textureType);
private:
	inline static ModdedTextureId makeTextureId(int xBpp2, int y) {
		return xBpp2 + y * VRAM_WIDTH;
	}
	inline static VramPageId makeVramPageId(int xBpp2, int y) {
		return (xBpp2 / VRAM_PAGE_WIDTH) | ((y / VRAM_PAGE_HEIGHT) << 4);
	}

	void setVramTextureId(ModdedTextureId textureId, int x, int y, int w, int h, bool isIdentified);
	uint8_t getMaxScale(const TiledTex &tiledTex) const;
	TextureTypes drawTextures(const std::list<IdentifiedTexture> &textures, const TiledTex &tiledTex, uint32_t *target, int w, int h, uint8_t scale);
	void cleanVramTextureIds(const TextureInfos &texture);
	void cleanTextures(ModdedTextureId textureId);

	// Link between texture data pointer sent to the graphic driver and VRAM coordinates
	std::unordered_map<const uint8_t *, TiledTex> _tiledTexs;
	// Keep track of where textures are uploaded to the VRAM
	std::vector<ModdedTextureId> _vramTextureIds; // ModdedTextureId[VRAM_WIDTH * VRAM_HEIGHT]
	// List of uploaded textures to the VRAM
	std::unordered_map<ModdedTextureId, IdentifiedTexture> _textures;
	// Cache optimization to list textures per VRAM page
	std::unordered_multimap<VramPageId, ModdedTextureId> _texturesIdsPerVramPageId;
};
