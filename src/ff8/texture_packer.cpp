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
#include "texture_packer.h"
#include "../saveload.h"
#include "../patch.h"
#include "mod.h"
#include <set>

uint32_t *image_data_scaled_cache = nullptr;
uint32_t image_data_scaled_size_cache = 0;

// Scale 32-bit BGRA image in place
void scale_up_image_data_in_place(uint32_t *sourceAndTarget, uint32_t w, uint32_t h, uint8_t scale)
{
	if (scale <= 1)
	{
		return;
	}

	uint32_t *source = sourceAndTarget + w * h,
		*target = sourceAndTarget + (w * scale) * (h * scale);

	for (int y = 0; y < h; ++y)
	{
		uint32_t *source_line_start = source;

		for (int i = 0; i < scale; ++i)
		{
			source = source_line_start;

			for (int x = 0; x < w; ++x)
			{
				source -= 1;
				target -= scale;

				for (int i = 0; i < scale; ++i)
				{
					target[i] = *source;
				}
			}
		}
	}
}

TexturePacker::TexturePacker() :
	_vramTextureIds(VRAM_WIDTH * VRAM_HEIGHT, INVALID_TEXTURE)
{
}

void TexturePacker::cleanVramTextureIds(const TextureInfos &texture)
{
	for (int prevY = 0; prevY < texture.h(); ++prevY)
	{
		int clearKey = texture.x() + (texture.y() + prevY) * VRAM_WIDTH;
		std::fill_n(_vramTextureIds.begin() + clearKey, texture.w(), INVALID_TEXTURE);
	}
}

void TexturePacker::cleanTextures(ModdedTextureId previousTextureId)
{
	auto it = _textures.find(previousTextureId);

	if (it == _textures.end())
	{
		return;
	}

	if (trace_all || trace_vram) ffnx_info("TexturePacker::%s: clear texture %s (textureId = %d)\n", __func__, it->second.name().empty() ? "N/A" : it->second.name().c_str(), previousTextureId);

	cleanVramTextureIds(it->second.texture());

	if (it->second.mod() != nullptr) {
		delete it->second.mod();
		_textures.erase(it);
	}

	// Clean older pointers
	auto begin = _texturesIdsPerVramPageId.begin(), end = _texturesIdsPerVramPageId.end();
	while (begin != end)
	{
		if (begin->second == previousTextureId)
		{
			begin = _texturesIdsPerVramPageId.erase(begin);
		}
		else
		{
			begin++;
		}
	}
}

void TexturePacker::setVramTextureId(ModdedTextureId textureId, int xBpp2, int y, int wBpp2, int h, bool isIdentified)
{
	auto t1 = highResolutionNow();

	if (trace_all || trace_vram) ffnx_trace("%s: textureId=%d xBpp2=%d y=%d wBpp2=%d h=%d isIdentified=%d\n", __func__, textureId, xBpp2, y, wBpp2, h, isIdentified);

	std::set<VramPageId> vramPageIds;

	for (int i = 0; i < h; ++i)
	{
		int vramY = y + i;

		for (int j = 0; j < wBpp2; ++j)
		{
			int vramX = xBpp2 + j;
			int key = vramX + vramY * VRAM_WIDTH;
			ModdedTextureId previousTextureId = _vramTextureIds.at(key);

			if (previousTextureId != INVALID_TEXTURE)
			{
				cleanTextures(previousTextureId);
			}

			_vramTextureIds[key] = textureId;

			vramPageIds.insert(makeVramPageId(vramX, vramY));
		}
	}

	if (isIdentified)
	{
		// Set texture id per vram page
		for (VramPageId pageId: vramPageIds)
		{
			_texturesIdsPerVramPageId.insert(std::pair<VramPageId, ModdedTextureId>(pageId, textureId));
		}
	}

	ffnx_info("TexturePacker::%s ellapsedTime=%Lf\n", __func__, elapsedMicroseconds(t1));
}

bool TexturePacker::setTexture(const char *name, const TextureInfos &texture, const TextureInfos &palette)
{
	auto t1 = highResolutionNow();
	bool hasNamedTexture = name != nullptr && *name != '\0';

	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s %s xBpp2=%d y=%d wBpp2=%d h=%d bpp=%d xPal=%d yPal=%d wPal=%d hPal=%d\n", __func__,
		hasNamedTexture ? name : "N/A",
		texture.x(), texture.y(), texture.w(), texture.h(), texture.bpp(),
		palette.x(), palette.y(), palette.w(), palette.h());

	ModdedTextureId textureId = makeTextureId(texture.x(), texture.y());
	setVramTextureId(textureId, texture.x(), texture.y(), texture.w(), texture.h(), hasNamedTexture);

	IdentifiedTexture &tex = _textures[textureId] = IdentifiedTexture(name, texture, palette);

	if (hasNamedTexture)
	{
		TextureModStandard *mod = new TextureModStandard(tex);

		if (mod->createImages())
		{
			tex.setMod(mod);
		}
		else
		{
			delete mod;
		}
	}

	ffnx_info("TexturePacker::%s ellapsedTime=%Lf\n", __func__, elapsedMicroseconds(t1));

	return tex.mod() != nullptr;
}

bool TexturePacker::setTextureBackground(const char *name, int x, int y, int w, int h, const std::vector<Tile> &mapTiles, int bgTexId, const char *extension, char *found_extension)
{
	auto t1 = highResolutionNow();
	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s %s x=%d y=%d w=%d h=%d tileCount=%d bgTexId=%d\n", __func__, name, x, y, w, h, mapTiles.size(), bgTexId);

	ModdedTextureId textureId = makeTextureId(x, y);
	setVramTextureId(textureId, x, y, w, h, true);

	IdentifiedTexture &tex = _textures[textureId] = IdentifiedTexture(name, TextureInfos(x, y, w, h, Tim::Bpp16, true));

	TextureBackground *mod = new TextureBackground(tex, mapTiles, bgTexId);
	if (mod->createImages(extension, found_extension))
	{
		tex.setMod(mod);
	}
	else
	{
		delete mod;
	}

	ffnx_info("TexturePacker::%s ellapsedTime=%Lf\n", __func__, elapsedMicroseconds(t1));

	return tex.mod() != nullptr;
}

bool TexturePacker::setTextureRedirection(const TextureInfos &oldTexture, const TextureInfos &newTexture, uint32_t *imageData)
{
	if (trace_all || trace_vram)  ffnx_info("TexturePacker::%s: old=(%d, %d, %d, %d) => new=(%d, %d, %d, %d)\n", __func__,
		oldTexture.x(), oldTexture.y(), oldTexture.w(), oldTexture.h(),
		newTexture.x(), newTexture.y(), newTexture.w(), newTexture.h());

	ModdedTextureId textureId = makeTextureId(oldTexture.x(), oldTexture.y());

	auto it = _textures.find(textureId);

	if (it == _textures.end())
	{
		ffnx_warning("TexturePacker::%s cannot find original texture of redirection\n", __func__);

		return false;
	}

	if (it->second.mod() != nullptr) // Already modded by external textures
	{
		return false;
	}

	setVramTextureId(textureId, oldTexture.x(), oldTexture.y(), oldTexture.w(), oldTexture.h(), true);

	IdentifiedTexture &tex = _textures[textureId] = IdentifiedTexture("redirection", oldTexture);
	TextureRedirection *redirection = new TextureRedirection(tex, newTexture);
	if (redirection->isValid() && redirection->createImages(imageData))
	{
		tex.setMod(redirection);

		return true;
	}

	delete redirection;

	return true; // True to not delete imageData pointer again
}

void TexturePacker::animateTextureByCopy(int sourceXBpp2, int sourceY, int sourceWBpp2, int sourceH, int targetXBpp2, int targetY)
{
	auto t1 = highResolutionNow();

	if (_textures.empty())
	{
		return;
	}

	ModdedTextureId textureId = _vramTextureIds.at(sourceXBpp2 + sourceY * VRAM_WIDTH),
		textureIdTarget = _vramTextureIds.at(targetXBpp2 + targetY * VRAM_WIDTH);
	if (textureId == INVALID_TEXTURE)
	{
		return;
	}

	auto it = _textures.find(textureId);

	if (it == _textures.end() || it->second.mod() == nullptr || !it->second.mod()->canCopyRect())
	{
		return;
	}

	if (textureId != textureIdTarget)
	{
		if (textureIdTarget == INVALID_TEXTURE)
		{
			return;
		}

		auto itTarget = _textures.find(textureIdTarget);

		if (itTarget == _textures.end() || itTarget->second.mod() == nullptr || !itTarget->second.mod()->canCopyRect())
		{
			return;
		}

		dynamic_cast<TextureModStandard *>(it->second.mod())->copyRect(sourceXBpp2, sourceY, sourceWBpp2, sourceH, targetXBpp2, targetY, *dynamic_cast<TextureModStandard *>(itTarget->second.mod()));
	}
	else
	{
		dynamic_cast<TextureModStandard *>(it->second.mod())->copyRect(sourceXBpp2, sourceY, sourceWBpp2, sourceH, targetXBpp2, targetY);
	}

	ffnx_info("TexturePacker::%s ellapsedTime=%Lf\n", __func__, elapsedMicroseconds(t1));
}

void TexturePacker::clearTiledTexs()
{
	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s\n", __func__);

	_tiledTexs.clear();
}

void TexturePacker::clearTextures()
{
	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s\n", __func__);

	std::fill_n(_vramTextureIds.begin(), _vramTextureIds.size(), INVALID_TEXTURE);

	_texturesIdsPerVramPageId.clear();

	for (auto &texture: _textures) {
		if (texture.second.mod() != nullptr) {
			delete texture.second.mod();
		}
	}
	_textures.clear();
}

std::list<TexturePacker::IdentifiedTexture> TexturePacker::matchTextures(const TiledTex &tiledTex, bool withModsOnly, bool notDrawnOnly) const
{
	std::list<IdentifiedTexture> ret;

	if (_textures.empty())
	{
		return ret;
	}

	if (!tiledTex.isValid()) {
		if (trace_all || trace_vram) ffnx_warning("TexturePacker::%s Unknown tex data\n", __func__);

		return ret;
	}

	auto [begin, end] = _texturesIdsPerVramPageId.equal_range(tiledTex.vramPageId());

	for (const std::pair<VramPageId, ModdedTextureId> &pair: std::ranges::subrange{begin, end})
	{
		auto it = _textures.find(pair.second);

		if (it == _textures.end())
		{
			continue;
		}

		const IdentifiedTexture &texture = it->second;
		ModdedTexture *mod = texture.mod();

		// Filtering on BPP and notDrawnOnly/withModsOnly parameters
		if (mod != nullptr)
		{
			if (!mod->isBpp(tiledTex.bpp) || (notDrawnOnly && mod->drawnOnce(tiledTex.palX, tiledTex.palY)))
			{
				continue;
			}
		}
		else
		{
			if (withModsOnly || (!texture.texture().hasMultiBpp() && texture.texture().bpp() != tiledTex.bpp))
			{
				continue;
			}
		}

		ret.push_back(texture);
	}

	return ret;
}

int textureId = 0;

TexturePacker::TextureTypes TexturePacker::drawTextures(
	const uint8_t *texData, uint32_t *rgbaImageData, uint32_t dataSize, int originalW, int originalH, uint8_t *outScale, uint32_t **outTarget)
{
	auto t1 = highResolutionNow();

	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s texData=0x%X bitsperpixel=%d\n", __func__, texData);

	*outScale = 1;
	*outTarget = rgbaImageData;

	if (_textures.empty())
	{
		return NoTexture;
	}

	auto it = _tiledTexs.find(texData);

	if (it == _tiledTexs.end())
	{
		if (trace_all || trace_vram) ffnx_warning("TexturePacker::%s Unknown tiledTex data\n", __func__);

		return NoTexture;
	}

	const TiledTex &tiledTex = it->second;

	if (!tiledTex.isValid())
	{
		if (trace_all || trace_vram) ffnx_warning("TexturePacker::%s Invalid tiledTex data\n", __func__);

		return NoTexture;
	}

	if (!tiledTex.isPalValid())
	{
		if (trace_all || trace_vram) ffnx_warning("TexturePacker::%s Palette is not set yet, we should not consider this texture\n", __func__);

		return NoTexture;
	}

	std::list<IdentifiedTexture> textures = matchTextures(tiledTex, true, true);
	uint8_t scale = 0;

	for (const IdentifiedTexture &tex: textures)
	{
		scale = std::max(scale, tex.mod()->scale(tiledTex.palX, tiledTex.palY));
	}

	uint32_t *target = rgbaImageData;

	if (scale > 1)
	{
		uint32_t image_data_size = originalW * scale * originalH * scale * 4;
		// Allocate with cache
		if (image_data_scaled_size_cache == 0 || image_data_size > image_data_scaled_size_cache) {
			if (image_data_scaled_cache != nullptr) {
				driver_free(image_data_scaled_cache);
			}
			image_data_scaled_cache = (uint32_t *)driver_malloc(image_data_size);
			image_data_scaled_size_cache = image_data_size;
		}

		target = image_data_scaled_cache;

		// convert source data
		if (target != nullptr)
		{
			memcpy(target, rgbaImageData, dataSize);
			scale_up_image_data_in_place(target, originalW, originalH, scale);
		}
	}
	else if (scale == 0)
	{
		return TextureTypes::NoTexture;
	}

	*outScale = scale;
	*outTarget = target;

	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s tex=(%d, %d) bpp=%d paletteVram=(%d, %d)\n", __func__, tiledTex.x, tiledTex.y, tiledTex.bpp, tiledTex.palX, tiledTex.palY);

	//debugSaveTexture(textureId, rgbaImageData, originalW, originalH, true, false, TexturePacker::InternalTexture);

	TextureTypes ret = drawTextures(textures, tiledTex, target, originalW, originalH, scale);

	//debugSaveTexture(textureId, target, originalW * scale, originalH * scale, true, true, ret);

	textureId++;

	ffnx_info("TexturePacker::%s ellapsedTime=%Lf\n", __func__, elapsedMicroseconds(t1));

	return ret;
}

TexturePacker::TextureTypes TexturePacker::drawTextures(const std::list<IdentifiedTexture> &textures, const TiledTex &tiledTex, uint32_t *target, int targetW, int targetH, uint8_t scale)
{
	TextureTypes drawnTextureTypes = NoTexture;
	int x = tiledTex.x * (4 >> tiledTex.bpp), y = tiledTex.y;

	for (const IdentifiedTexture &texture: textures)
	{
		if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s matches %s (rect=%d,%d,%d,%d)\n", __func__, texture.name().empty() ? "N/A" : texture.name().c_str(), texture.texture().x(), texture.texture().y(), texture.texture().w(), texture.texture().h());

		TextureTypes textureType = texture.mod()->drawToImage(
			texture.texture().pixelX() - x, texture.texture().y() - y,
			target, targetW, targetH, scale, tiledTex.bpp,
			tiledTex.palX, tiledTex.palY
		);

		drawnTextureTypes = TextureTypes(int(drawnTextureTypes) | int(textureType));
	}

	if (trace_all || trace_vram)
	{
		ffnx_trace("TexturePacker::%s x=%d y=%d bpp=%d targetW=%d targetH=%d scale=%d drawnTextureTypes=0x%X\n", __func__, tiledTex.x, tiledTex.y, tiledTex.bpp, targetW, targetH, scale, drawnTextureTypes);
	}

	return drawnTextureTypes;
}

void TexturePacker::registerTiledTex(const uint8_t *texData, int x, int y, Tim::Bpp bpp, int palX, int palY)
{
	if (trace_all || trace_vram) ffnx_trace("%s pointer=0x%X x=%d y=%d bpp=%d palX=%d palY=%d\n", __func__, texData, x, y, bpp, palX, palY);

	if ((palX == -1 || palY == -1) && _tiledTexs.contains(texData))
	{
		if (palX == -1) palX = _tiledTexs[texData].palX;
		if (palY == -1) palY = _tiledTexs[texData].palY;
	}

	_tiledTexs[texData] = TiledTex(x, y, bpp, palX, palY);
}

void TexturePacker::registerPaletteWrite(const uint8_t *texData, int palX, int palY)
{
	if (trace_all || trace_vram) ffnx_trace("%s pointer=0x%X palX=%d palY=%d\n", __func__, texData, palX, palY);

	if (_tiledTexs.contains(texData))
	{
		TiledTex &tex = _tiledTexs[texData];
		tex.palX = palX;
		tex.palY = palY;
	}
	else
	{
		_tiledTexs[texData] = TiledTex(-1, -1, Tim::Bpp4, palX, palY);
	}
}

TexturePacker::TiledTex TexturePacker::getTiledTex(const uint8_t *texData) const
{
	auto it = _tiledTexs.find(texData);

	if (it != _tiledTexs.end())
	{
		return it->second;
	}

	return TiledTex();
}

void TexturePacker::debugSaveTexture(int textureId, const uint32_t *source, int w, int h, bool removeAlpha, bool after, TextureTypes textureType)
{
	uint32_t *target = new uint32_t[w * h];

	for (int i = 0; i < h * w; ++i)
	{
		target[i] = removeAlpha ? source[i] | 0xff000000 : source[i]; // Remove alpha
	}

	char filename[MAX_PATH];
	snprintf(filename, sizeof(filename), "texture-%d-%s-type-%s", textureId, after ? "z-after" : "a-before", textureType == TextureTypes::ExternalTexture ? "external" : (textureType == TextureTypes::InternalTexture ? "internal" : "nomatch"));

	if (trace_all || trace_vram) ffnx_trace("%s %s\n", __func__, filename);

	save_texture(target, w * h * 4, w, h, -1, filename, false);

	delete[] target;
}

TexturePacker::TextureInfos::TextureInfos() :
	_x(-1), _y(0), _w(0), _h(0), _bpp(Tim::Bpp4)
{
}

TexturePacker::TextureInfos::TextureInfos(
	int xBpp2, int y, int wBpp2, int h,
	Tim::Bpp bpp, bool multiBpp
) : _x(xBpp2), _y(y), _w(wBpp2), _h(h), _bpp(bpp), _multiBpp(multiBpp)
{
}

TexturePacker::TiledTex::TiledTex()
 : x(-1), y(-1), palX(0), palY(0), bpp(Tim::Bpp4)
{
}

TexturePacker::TiledTex::TiledTex(
	int x, int y, Tim::Bpp bpp, int palX, int palY
) : x(x), y(y), palX(palX), palY(palY), bpp(bpp)
{
}

TexturePacker::IdentifiedTexture::IdentifiedTexture() :
	_texture(TextureInfos()), _palette(TextureInfos()), _name(""), _mod(nullptr)
{
}

TexturePacker::IdentifiedTexture::IdentifiedTexture(
	const char *name,
	const TextureInfos &texture,
	const TextureInfos &palette
) : _texture(texture), _palette(palette), _name(name == nullptr ? "" : name), _mod(nullptr)
{
}

void TexturePacker::IdentifiedTexture::setMod(ModdedTexture *mod)
{
	if (_mod != nullptr)
	{
		delete _mod;
	}

	_mod = mod;
}
