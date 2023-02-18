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
#include "../renderer.h"
#include "../saveload.h"
#include "../patch.h"
#include "file.h"
#include <set>

TexturePacker::TexturePacker() :
	_vram(nullptr), _vramTextureIds(VRAM_WIDTH * VRAM_HEIGHT, INVALID_TEXTURE), _disableDrawTexturesBackground(false)
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

void TexturePacker::cleanTextures(ModdedTextureId previousTextureId, bool keepMods)
{
	if (_textures.contains(previousTextureId))
	{
		Texture &previousTexture = _textures.at(previousTextureId);

		if (trace_all || trace_vram) ffnx_info("TexturePacker::%s: clear texture %s (textureId = %d)\n", __func__, previousTexture.name().c_str(), previousTextureId);

		cleanVramTextureIds(previousTexture);

		_textures.erase(previousTextureId);
	}
	else if (!keepMods && _externalTextures.contains(previousTextureId))
	{
		Texture &previousTexture = _externalTextures.at(previousTextureId);

		if (trace_all || trace_vram) ffnx_info("TexturePacker::%s: clear texture %s (textureId = %d)\n", __func__, previousTexture.name().c_str(), previousTextureId);

		cleanVramTextureIds(previousTexture);

		previousTexture.destroyImage();
		_externalTextures.erase(previousTextureId);
	}
	else if (!keepMods && _backgroundTextures.contains(previousTextureId))
	{
		TextureBackground &previousTexture = _backgroundTextures.at(previousTextureId);

		if (trace_all || trace_vram) ffnx_info("TexturePacker::%s: clear texture background %s (textureId = %d)\n", __func__, previousTexture.name().c_str(), previousTextureId);

		cleanVramTextureIds(previousTexture);

		previousTexture.destroyImage();
		_backgroundTextures.erase(previousTextureId);
	}
	else if (_textureRedirections.contains(previousTextureId))
	{
		TextureRedirection &previousTexture = _textureRedirections.at(previousTextureId);

		if (trace_all || trace_vram) ffnx_info("TexturePacker::%s: clear texture redirection (textureId = %d)\n", __func__, previousTextureId);

		cleanVramTextureIds(previousTexture.oldTexture());

		previousTexture.destroyImage();
		_textureRedirections.erase(previousTextureId);
	}
}

void TexturePacker::setVramTextureId(ModdedTextureId textureId, int x, int y, int w, int h, bool keepMods)
{
	for (int i = 0; i < h; ++i)
	{
		int vramY = y + i;

		for (int j = 0; j < w; ++j)
		{
			int vramX = x + j;
			int key = vramX + vramY * VRAM_WIDTH;
			ModdedTextureId previousTextureId = _vramTextureIds.at(key);

			if (previousTextureId != INVALID_TEXTURE)
			{
				cleanTextures(previousTextureId, keepMods);
			}

			_vramTextureIds[key] = textureId;
		}
	}
}

void TexturePacker::uploadTexture(const uint8_t *source, int x, int y, int w, int h)
{
	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s x=%d y=%d w=%d h=%d\n", __func__, x, y, w, h);

	uint8_t *vram = vramSeek(x, y);
	const int vramLineWidth = VRAM_DEPTH * VRAM_WIDTH;
	const int lineWidth = VRAM_DEPTH * w;

	for (int i = 0; i < h; ++i)
	{
		memcpy(vram, source, lineWidth);

		source += lineWidth;
		vram += vramLineWidth;
	}
}

void TexturePacker::setTexture(const char *name, int x, int y, int w, int h, Tim::Bpp bpp, bool isPal)
{
	bool hasNamedTexture = name != nullptr && *name != '\0';

	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s %s x=%d y=%d w=%d h=%d bpp=%d isPal=%d\n", __func__, hasNamedTexture ? name : "N/A", x, y, w, h, bpp, isPal);

	ModdedTextureId textureId = INVALID_TEXTURE;

	if (hasNamedTexture && !isPal)
	{
		Texture tex(name, x, y, w, h, bpp);
		textureId = makeTextureId(x, y);

		if (tex.createImage())
		{
			_externalTextures[textureId] = tex;
		}
		else
		{
			_textures[textureId] = tex;
		}
	}

	setVramTextureId(textureId, x, y, w, h);
}

void TexturePacker::setTextureBackground(const char *name, int x, int y, int w, int h, const std::vector<Tile> &mapTiles)
{
	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s %s x=%d y=%d w=%d h=%d tileCount=%d\n", __func__, name, x, y, w, h, mapTiles.size());

	TextureBackground tex(name, x, y, w, h, mapTiles);
	ModdedTextureId textureId = makeTextureId(x, y);

	removeMe = 0;

	if (tex.createImage())
	{
		_backgroundTextures[textureId] = tex;
	}
	else
	{
		_textures[textureId] = tex;
	}

	setVramTextureId(textureId, x, y, w, h);
}

bool TexturePacker::setTextureRedirection(const TextureInfos &oldTexture, const TextureInfos &newTexture, uint32_t *imageData)
{
	if (trace_all || trace_vram)  ffnx_info("TexturePacker::%s: old=(%d, %d, %d, %d) => new=(%d, %d, %d, %d)\n", __func__,
		oldTexture.x(), oldTexture.y(), oldTexture.w(), oldTexture.h(),
		newTexture.x(), newTexture.y(), newTexture.w(), newTexture.h());

	TextureRedirection redirection(oldTexture, newTexture);
	if (redirection.isValid() && redirection.createImage(imageData))
	{
		ModdedTextureId textureId = makeTextureId(oldTexture.x(), oldTexture.y());
		_textureRedirections[textureId] = redirection;

		setVramTextureId(textureId, oldTexture.x(), oldTexture.y(), oldTexture.w(), oldTexture.h(), true);

		return true;
	}

	return false;
}

uint8_t TexturePacker::getMaxScale(const uint8_t *texData) const
{
	if (_externalTextures.empty() && _backgroundTextures.empty() && _textureRedirections.empty())
	{
		return 1;
	}

	if (!_tiledTexs.contains(texData))
	{
		if (trace_all || trace_vram) ffnx_warning("TexturePacker::%s Unknown tex data\n", __func__);

		return 0;
	}

	const TiledTex &tiledTex = _tiledTexs.at(texData);

	uint8_t maxScale = 1;

	for (int y = 0; y < 256; ++y)
	{
		int vramY = tiledTex.y + y;

		for (int x = 0; x < 64; ++x)
		{
			int vramX = tiledTex.x + x;
			ModdedTextureId textureId = _vramTextureIds.at(vramX + vramY * VRAM_WIDTH);

			if (textureId != INVALID_TEXTURE)
			{
				uint8_t scale = 0;

				if (_externalTextures.contains(textureId))
				{
					scale = _externalTextures.at(textureId).scale();
				}
				else if (_backgroundTextures.contains(textureId))
				{
					scale = _backgroundTextures.at(textureId).scale();
				}
				else if (_textureRedirections.contains(textureId))
				{
					scale = _textureRedirections.at(textureId).scale();
				}

				if (scale > maxScale)
				{
					maxScale = scale;
				}
			}
		}
	}

	return maxScale;
}

void TexturePacker::getTextureNames(const uint8_t *texData, std::list<std::string> &names) const
{
	if (_externalTextures.empty() && _backgroundTextures.empty() && _textures.empty())
	{
		return;
	}

	if (!_tiledTexs.contains(texData))
	{
		ffnx_warning("TexturePacker::%s Unknown tex data %p\n", __func__, texData);

		return;
	}

	const TiledTex &tiledTex = _tiledTexs.at(texData);
	std::set<ModdedTextureId> textureIds;

	for (int y = 0; y < 256; ++y)
	{
		int vramY = tiledTex.y + y;

		for (int x = 0; x < 64; ++x)
		{
			int vramX = tiledTex.x + x;
			ModdedTextureId textureId = _vramTextureIds.at(vramX + vramY * VRAM_WIDTH);

			if (textureId != INVALID_TEXTURE)
			{
				if (_externalTextures.contains(textureId))
				{
					textureIds.insert(textureId);
				}
				else if (_backgroundTextures.contains(textureId))
				{
					textureIds.insert(textureId);
				}
				else if (_textures.contains(textureId))
				{
					textureIds.insert(textureId);
				}
			}
		}
	}

	for (ModdedTextureId textureId: textureIds)
	{
		if (_externalTextures.contains(textureId))
		{
			names.push_back(_externalTextures.at(textureId).name());
		}
		else if (_backgroundTextures.contains(textureId))
		{
			names.push_back(_backgroundTextures.at(textureId).name());
		}
		else
		{
			names.push_back(_textures.at(textureId).name());
		}
	}
}

struct FieldStateBackground {
	uint8_t stack_data[0x140];
	uint32_t field_140;
	uint32_t field_144;
	uint32_t field_148;
	uint32_t field_14c;
	uint32_t field_150;
	uint32_t field_154;
	uint32_t field_158;
	uint32_t field_15c;
	uint32_t execution_flags; // bgdraw: 0x10, bganime/rbganime: 0x980 (0x800: animation ongoing)
	uint32_t field_164;
	uint32_t field_168;
	uint32_t field_16c;
	uint32_t field_170;
	uint8_t field_174; // has anim?
	uint8_t field_175; // has anim mask?
	uint16_t current_instruction_position; // field_176
	uint32_t field_178;
	uint32_t field_17c;
	uint32_t field_180;
	uint8_t stack_current_position; // field_184
	uint8_t field_185;
	uint8_t field_186;
	uint8_t field_187;
	uint16_t bgstate; // field_188, set to -1 if off
	uint16_t field_18a;
	uint16_t bgparam_anim_start; // field_18c
	uint16_t bgparam_anim_end; // field_18e
	uint16_t bgparam_anim_speed1; // field_190
	uint16_t bgparam_anim_speed2; // field_192
	uint16_t bgparam_anim_flags; // field_194
	uint16_t field_196;
	uint32_t field_198;
	uint16_t bgshadeloop_remember_stack_pointer; // field_19c
	uint16_t bgshade_add_value; // field_19e
	uint16_t field_1a0; // bgshadeloop
	uint16_t field_1a2; // bgshadeloop
	uint8_t field_1a4; // bgshadeloop
	uint8_t field_1a5; // bgshadeloop
	uint8_t field_1a6; // bgshadeloop
	uint8_t bgshade_color1r; // field_1a7
	uint8_t bgshade_color1g; // field_1a8
	uint8_t bgshade_color1b; // field_1a9
	uint8_t bgshade_color2r; // field_1aa
	uint8_t bgshade_color2g; // field_1ab
	uint8_t bgshade_color2b; // field_1ac
	uint8_t bgshade_color1r_2; // field_1ad
	uint8_t bgshade_color1g_2; // field_1ae
	uint8_t bgshade_color1b_2; // field_1af
	uint8_t bgshade_color2r_2; // field_1b0
	uint8_t bgshade_color2g_2; // field_1b1
	uint8_t bgshade_color2b_2; // field_1b2
	uint8_t field_1b3;
};

void TexturePacker::disableDrawTexturesBackground(bool disabled)
{
	_disableDrawTexturesBackground = disabled;
}

TexturePacker::TextureTypes TexturePacker::drawTextures(const uint8_t *texData, struct texture_format *tex_format, uint32_t *target, const uint32_t *originalImageData, int originalW, int originalH, uint8_t scale, uint32_t paletteIndex)
{
	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s pointer=0x%X bitsperpixel=%d\n", __func__, texData, (tex_format ? tex_format->bitsperpixel : -1));

	auto it = _tiledTexs.find(texData);

	if (it != _tiledTexs.end())
	{
		const TiledTex &tex = it->second;

		if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s tex=(%d, %d) bpp=%d paletteIndex=%d\n", __func__, tex.x, tex.y, tex.bpp, paletteIndex);

		/* if (!tex.renderedOnce) {
			_tiledTexs[texData].renderedOnce = true;
			return NoTexture;
		} */

		//memset(target, 0, originalW * originalH * 4);

		//return drawTextures(target, tex, originalW, originalH, scale, paletteIndex);

		if (drawTextures(target, tex, originalW, originalH, scale, paletteIndex)) {
			char fileName[MAX_PATH] = {};
			sprintf(fileName, "backgroundTexture-%d", removeMe++);
			/* for (int y = 0; y < originalH; ++y) {
				for (int x = 0; x < originalW; ++x) {
					target[y * originalW + x] = originalImageData[y * originalW + x] | (0xffu << 24);
				}
			} */
			ffnx_info("TexturePacker::%s pointer=0x%X fileName=%s\n", __func__, texData, fileName);
			save_texture(originalImageData, originalW * originalH * 4, originalW, originalH, paletteIndex, fileName, false);

			return NoTexture;
			// return ExternalTexture;
		}

		return NoTexture;
	}

	if (trace_all || trace_vram) ffnx_warning("TexturePacker::%s Unknown tex data\n", __func__);

	return NoTexture;
}

TexturePacker::TextureTypes TexturePacker::drawTextures(uint32_t *target, const TiledTex &tiledTex, int targetW, int targetH, uint8_t scale, uint32_t paletteIndex)
{
	if (_externalTextures.empty() && _backgroundTextures.empty() && _textureRedirections.empty())
	{
		return NoTexture;
	}

	/* for (int i = 0; i < 32; ++i) {
		for (int j = 0; j < 8; ++j) {
			const struc_50 &s50 = ff8_externals.psx_texture_pages[0].struc_50_array[i];
			const texture_page &page = s50.texture_page[j];
			ffnx_info("%d %d bpp=%d, pointer=0x%X, field_0=%X, field_20=%X, x=%d, y=%d, palette_data=0x%X, vram_palette_pos=(%d, %d)\n", i, j, page.color_key, page.image_data, page.field_0, page.field_20, page.x, page.y, s50.vram_palette_data, (s50.vram_palette_pos & 0x3F), (s50.vram_palette_pos >> 6));
		}
	} */

	/*
	if (getmode_cached()->driver_mode == MODE_FIELD) {
		uint8_t **field_state_1_dword_1D9CC64 = (uint8_t **)0x1D9CC64;
		uint8_t **field_state_2_dword_1D9CC68 = (uint8_t **)0x1D9CC68;
		uint8_t **field_state_3_dword_1D9CDC8 = (uint8_t **)0x1D9CDC8;
		uint8_t **field_state_4_dword_1D9CC60 = (uint8_t **)0x1D9CC60;
		ffnx_trace("dll_gfx: load_texture 0x%x 0x%X param=%d param=%d param=%d param=%d\n", _texture_set, VREF(tex_header, image_data), *(int16_t *)(*field_state_1_dword_1D9CC64 + 392), *(int16_t *)(*field_state_2_dword_1D9CC68 + 392), *(int16_t *)(*field_state_3_dword_1D9CDC8 + 392), *(int16_t *)(*field_state_4_dword_1D9CC60 + 392));
	}
	*/

	int w = targetW;

	if (tiledTex.bpp <= int(Tim::Bpp16))
	{
		w /= 4 >> tiledTex.bpp;
	}
	else
	{
		ffnx_warning("%s: Unknown bpp %d\n", tiledTex.bpp);

		return NoTexture;
	}

	if (_disableDrawTexturesBackground) {
		if (trace_all || trace_vram) ffnx_info("TexturePacker::%s disabled\n", __func__);
	}

	TextureTypes drawnTextureTypes = NoTexture;

	int scaledW = w * scale,
		scaledH = targetH * scale;

	for (int y = 0; y < targetH; ++y)
	{
		int vramY = tiledTex.y + y;

		if (vramY >= VRAM_HEIGHT) {
			//ffnx_warning("%s: vram Y overflow\n", __func__);
			break;
		}

		for (int x = 0; x < w; ++x)
		{
			int vramX = tiledTex.x + x;

			if (vramX >= VRAM_WIDTH) {
				//ffnx_warning("%s: vram X overflow\n", __func__);
				break;
			}

			ModdedTextureId textureId = _vramTextureIds.at(vramX + vramY * VRAM_WIDTH);

			if (textureId != INVALID_TEXTURE)
			{
				if (_externalTextures.contains(textureId))
				{
					const Texture &texture = _externalTextures.at(textureId);

					int textureX = vramX - texture.x(),
						textureY = vramY - texture.y();

					texture.copyRect(textureX, textureY, target, x, y, targetW, scale);

					drawnTextureTypes = TextureTypes(int(drawnTextureTypes) | int(ExternalTexture));
				}
				else if (_backgroundTextures.contains(textureId) && ! _disableDrawTexturesBackground)
				{
					const TextureBackground &texture = _backgroundTextures.at(textureId);

					//ffnx_info("%s: vram=(%d, %d) bgTexture=(%d, %d) textureId=(%d => %d, %d)\n", __func__, vramX, vramY, texture.x(), texture.y(), textureId, textureId % VRAM_WIDTH, textureId / VRAM_WIDTH);

					int textureX = vramX - texture.x(),
						textureY = vramY - texture.y();

					texture.copyRect(textureX, textureY, tiledTex.bpp, target, x, y, targetW, scale);

					drawnTextureTypes = TextureTypes(int(drawnTextureTypes) | int(ExternalTexture));
				}
				else if (_textureRedirections.contains(textureId))
				{
					const TextureRedirection &redirection = _textureRedirections.at(textureId);

					int textureX = vramX - redirection.oldTexture().x(),
						textureY = vramY - redirection.oldTexture().y();

					redirection.copyRect(textureX, textureY, target, x, y, targetW, scale);

					drawnTextureTypes = TextureTypes(int(drawnTextureTypes) | int(InternalTexture));
				}
			}
		}
	}

	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s x=%d y=%d bpp=%d w=%d targetW=%d targetH=%d scale=%d drawnTextureTypes=0x%X\n", __func__, tiledTex.x, tiledTex.y, tiledTex.bpp, w, targetW, targetH, scale, drawnTextureTypes);

	return drawnTextureTypes;
}

void TexturePacker::registerTiledTex(const uint8_t *texData, int x, int y, Tim::Bpp bpp, int palX, int palY)
{
	if (trace_all || trace_vram) ffnx_trace("%s pointer=0x%X x=%d y=%d bpp=%d palX=%d palY=%d psx_texture_pages=%X\n", __func__, texData, x, y, bpp, palX, palY, ff8_externals.psx_texture_pages);

	_tiledTexs[texData] = TiledTex(x, y, bpp, palX, palY);
}

bool TexturePacker::saveVram(const char *fileName, Tim::Bpp bpp) const
{
	uint16_t palette[256] = {};

	ff8_tim tim_infos = ff8_tim();

	tim_infos.img_data = _vram;
	tim_infos.img_w = VRAM_WIDTH;
	tim_infos.img_h = VRAM_HEIGHT;

	if (bpp < Tim::Bpp16)
	{
		tim_infos.pal_data = palette;
		tim_infos.pal_h = 1;
		tim_infos.pal_w = bpp == Tim::Bpp4 ? 16 : 256;

		// Greyscale palette
		for (int i = 0; i < tim_infos.pal_w; ++i)
		{
			uint8_t color = bpp == Tim::Bpp4 ? i * 16 : i;
			palette[i] = color | (color << 5) | (color << 10);
		}
	}

	return Tim(bpp, tim_infos).save(fileName, bpp);
}

TexturePacker::TextureInfos::TextureInfos() :
	_x(0), _y(0), _w(0), _h(0), _bpp(Tim::Bpp4)
{
}

TexturePacker::TextureInfos::TextureInfos(
	int x, int y, int w, int h,
	Tim::Bpp bpp
) : _x(x), _y(y), _w(w), _h(h), _bpp(bpp)
{
}

bimg::ImageContainer *TexturePacker::TextureInfos::createImageContainer(const char *name, uint8_t palette_index, bool hasPal)
{
	char filename[MAX_PATH] = {}, langPath[16] = {};

	if(trace_all || trace_loaders || trace_vram) ffnx_trace("texture file name (VRAM): %s\n", name);

	if(save_textures) return nullptr;

	ff8_fs_lang_string(langPath);
	strcat(langPath, "/");

	for (uint8_t lang = 0; lang < 2; lang++)
	{
		for (size_t idx = 0; idx < mod_ext.size(); idx++)
		{
			if (hasPal) {
				_snprintf(filename, sizeof(filename), "%s/%s/%s%s_%02i.%s", basedir, mod_path.c_str(), langPath, name, palette_index, mod_ext[idx].c_str());
			} else {
				_snprintf(filename, sizeof(filename), "%s/%s/%s%s.%s", basedir, mod_path.c_str(), langPath, name, mod_ext[idx].c_str());
			}
			bimg::ImageContainer *image = newRenderer.createImageContainer(filename, bimg::TextureFormat::BGRA8);

			if (image != nullptr)
			{
				if (trace_all || trace_loaders || trace_vram) ffnx_trace("Using texture: %s\n", filename);

				return image;
			}
			else if (trace_all || trace_loaders || trace_vram)
			{
				ffnx_warning("Texture does not exist, skipping: %s\n", filename);
			}
		}

		*langPath = '\0';
	}

	return nullptr;
}

uint8_t TexturePacker::TextureInfos::computeScale(int sourcePixelW, int sourceH, int targetPixelW, int targetH)
{
	if (targetPixelW < sourcePixelW
		|| targetH < sourceH
		|| targetPixelW % sourcePixelW != 0
		|| targetH % sourceH != 0)
	{
		ffnx_warning("Texture redirection size must be scaled to the original texture size\n");

		return 0;
	}

	int scaleW = targetPixelW / sourcePixelW, scaleH = targetH / sourceH;

	if (scaleW != scaleH)
	{
		ffnx_warning("Texture redirection size must have the same ratio as the original texture: (%d / %d)\n", sourcePixelW, sourceH);

		return 0;
	}

	if (scaleW > MAX_SCALE)
	{
		ffnx_warning("Texture redirection size cannot exceed original size * %d\n", MAX_SCALE);

		return MAX_SCALE;
	}

	return scaleW;
}

void TexturePacker::TextureInfos::copyRect(
	const uint32_t *sourceRGBA, int sourceXBpp2, int sourceYBpp2, int sourceW, uint8_t sourceScale, Tim::Bpp sourceDepth,
	uint32_t *targetRGBA, int targetX, int targetY, int targetW, uint8_t targetScale)
{
	if (targetScale < sourceScale)
	{
		return;
	}

	uint8_t targetRectWidth = (4 >> int(sourceDepth)) * targetScale,
		targetRectHeight = targetScale,
		sourceRectWidth = (4 >> int(sourceDepth)) * sourceScale,
		sourceRectHeight = sourceScale;
	uint8_t scaleRatio = targetScale / sourceScale;

	targetX *= targetRectWidth;
	targetY *= targetRectHeight;
	targetW *= targetScale;

	int sourceX = sourceXBpp2 * sourceRectWidth;
	int sourceY = sourceYBpp2 * sourceRectHeight;

	for (int y = 0; y < targetRectHeight; ++y)
	{
		for (int x = 0; x < targetRectWidth; ++x)
		{
			*(targetRGBA + targetX + x + (targetY + y) * targetW) = *(sourceRGBA + sourceX + x / scaleRatio + (sourceY + y / scaleRatio) * sourceW);
		}
	}
}

TexturePacker::Texture::Texture() :
	TextureInfos(), _image(nullptr), _name(""), _scale(1)
{
}

TexturePacker::Texture::Texture(
	const char *name,
	int x, int y, int w, int h,
	Tim::Bpp bpp
) : TextureInfos(x, y, w, h, bpp), _image(nullptr), _name(name), _scale(1)
{
}

bool TexturePacker::Texture::createImage(uint8_t palette_index)
{
	char filename[MAX_PATH], langPath[16] = {};

	_image = createImageContainer(_name.c_str(), palette_index, bpp() != Tim::Bpp16);

	if (_image == nullptr)
	{
		return false;
	}

	uint8_t scale = computeScale();

	if (scale == 0)
	{
		destroyImage();

		return false;
	}

	_scale = scale;

	return true;
}

void TexturePacker::Texture::destroyImage()
{
	if (_image != nullptr) {
		bimg::imageFree(_image);
		_image = nullptr;
	}
}

uint8_t TexturePacker::Texture::computeScale() const
{
	return TextureInfos::computeScale(pixelW(), h(), _image->m_width, _image->m_height);
}

void TexturePacker::Texture::copyRect(int sourceXBpp2, int sourceYBpp2, uint32_t *target, int targetX, int targetY, int targetW, uint8_t targetScale) const
{
	TextureInfos::copyRect(
		(const uint32_t *)_image->m_data, sourceXBpp2, sourceYBpp2, _image->m_width, _scale, bpp(),
		target, targetX, targetY, targetW, targetScale
	);
}

TexturePacker::TextureBackground::TextureBackground() :
	Texture()
{
}

TexturePacker::TextureBackground::TextureBackground(
	const char *name,
	int x, int y, int w, int h,
	const std::vector<Tile> &mapTiles
) : Texture(name, x, y, w, h, Tim::Bpp16), _mapTiles(mapTiles)
{
	// Build tileIdsByPosition for fast lookup
	_tileIdsByPosition.reserve(mapTiles.size());
	size_t tileId = 0;
	for (const Tile &tile: mapTiles) {
		const uint8_t textureId = tile.texID & 0xF;
		uint16_t key = uint16_t(textureId) | (uint16_t(tile.srcX / TILE_SIZE) << 4) | (uint16_t(tile.srcY / TILE_SIZE) << 8);

		auto it = _tileIdsByPosition.find(key);
		// Remove some duplicates (but keep those which render differently)
		if (it == _tileIdsByPosition.end() || ! ff8_background_tiles_looks_alike(tile, mapTiles.at(it->second))) {
			_tileIdsByPosition.insert(std::pair<uint16_t, size_t>(key, tileId));
		}
		++tileId;
	}
	_colsCount = mapTiles.size() / (TEXTURE_HEIGHT / TILE_SIZE) + int(mapTiles.size() % (TEXTURE_HEIGHT / TILE_SIZE) != 0);

	ffnx_info("TextureBackground::%s: colsCount=%d\n", __func__, _colsCount);
}

void TexturePacker::TextureBackground::copyRect(int sourceXBpp2, int sourceYBpp2, Tim::Bpp textureBpp, uint32_t *target, int targetX, int targetY, int targetW, uint8_t targetScale) const
{
	const uint16_t sourceXInBytes = sourceXBpp2 * 2;
	const uint8_t textureId = sourceXInBytes / TEXTURE_WIDTH_BYTES;
	const uint16_t srcX = sourceXInBytes % TEXTURE_WIDTH_BYTES, srcY = sourceYBpp2;

	//ffnx_trace("TextureBackground::%s: sourceXBpp2=%d, sourceYBpp2=%d, textureId=%d, srcX=%d, srcY=%d\n", __func__, sourceXBpp2, sourceYBpp2, textureId, srcX, srcY);

	auto [begin, end] = _tileIdsByPosition.equal_range(uint16_t(textureId) | (uint16_t(srcX / TILE_SIZE) << 4) | (uint16_t(srcY / TILE_SIZE) << 8));
	if (begin == end) {
		//ffnx_warning("TextureBackground::%s: tile not found textureId=%d, srcX=%d, srcY=%d\n", __func__, textureId, srcX, srcY);
		return;
	}

	/* if (_tileIdsByPosition.count(uint16_t(textureId) | (uint16_t(srcX / TILE_SIZE) << 4) | (uint16_t(srcY / TILE_SIZE) << 8)) > 1) {
		return;
	} */

	bool matched = false;

	// Iterate over matching tiles
	for (const std::pair<uint16_t, size_t> &pair: std::ranges::subrange{begin, end}) {
		const size_t tileId = pair.second;
		const Tile &tile = _mapTiles.at(tileId);

		if (tile.parameter != 255) { // FIXME
			uint8_t jsm_count_backgrounds = *(uint8_t *)0x1D9CDC0;

			if (tile.parameter < jsm_count_backgrounds) {
				FieldStateBackground *field_state_backgrounds = *(FieldStateBackground **)0x1D9CC64;

				if (tile.state != field_state_backgrounds[tile.parameter].bgstate >> 6) {
					continue;
				}
			} else {
				ffnx_warning("TextureBackground::%s: group script not found for background parameter %d\n", __func__, tile.parameter);

				continue;
			}
		}

		if (matched) {
			ffnx_warning("TextureBackground::%s: multiple tiles found for the same position (tile id %d)\n", __func__, tileId);

			continue;
		}

		Tim::Bpp bpp = Tim::Bpp((tile.texID >> 7) & 3);
		uint8_t col = tileId % _colsCount, row = tileId / _colsCount;
		int imageX = col * (TILE_SIZE / 2) + sourceXBpp2 % (TILE_SIZE / 2), imageY = row * TILE_SIZE + sourceYBpp2 % TILE_SIZE;

		//ffnx_trace("%s: sourceXBpp2=%d, sourceYBpp2=%d, textureId=%d, srcX=%d, srcY=%d, key=%d (%d, %d => %d, %d => %d)\n", __func__, sourceXBpp2, sourceYBpp2, textureId, srcX, srcY, uint16_t(textureId) | (uint16_t(srcX / TILE_SIZE) << 4) | (uint16_t(srcY / TILE_SIZE) << 8), uint16_t(textureId), uint16_t(srcX / TILE_SIZE), (uint16_t(srcX / TILE_SIZE) << 4), uint16_t(srcY / TILE_SIZE), (uint16_t(srcY / TILE_SIZE) << 8));

		//ffnx_trace("%s: tileId=%d, col=%d, row=%d, imageX=%d, imageY=%d, bpp=%d, scale=%d\n", __func__, tileId, col, row, imageX, imageY, int(bpp), scale());

		TextureInfos::copyRect(
			(const uint32_t *)image()->m_data, imageX, imageY, image()->m_width, scale(), bpp,
			target, targetX, targetY, targetW, targetScale
		);

		matched = true;
	}
}

uint8_t TexturePacker::TextureBackground::computeScale() const
{
	return TextureInfos::computeScale(_colsCount * TILE_SIZE, TEXTURE_HEIGHT, image()->m_width, image()->m_height);
}

TexturePacker::TiledTex::TiledTex()
 : x(0), y(0), palX(0), palY(0), bpp(Tim::Bpp4), renderedOnce(false)
{
}

TexturePacker::TiledTex::TiledTex(
	int x, int y, Tim::Bpp bpp, int palX, int palY
) : x(x), y(y), palX(palX), palY(palY), bpp(bpp), renderedOnce(false)
{
}

TexturePacker::TextureRedirection::TextureRedirection()
 : TextureInfos(), _image(nullptr), _scale(0)
{
}

TexturePacker::TextureRedirection::TextureRedirection(
	const TextureInfos &oldTexture,
	const TextureInfos &newTexture
) : TextureInfos(newTexture), _image(nullptr), _oldTexture(oldTexture),
	_scale(computeScale())
{
}

bool TexturePacker::TextureRedirection::createImage(uint32_t *imageData)
{
	_image = imageData;

	return _image != nullptr;
}

void TexturePacker::TextureRedirection::destroyImage()
{
	if (_image != nullptr) {
		driver_free(_image);
		_image = nullptr;
	}
}

uint8_t TexturePacker::TextureRedirection::computeScale() const
{
	return TextureInfos::computeScale(_oldTexture.pixelW(), _oldTexture.h(), pixelW(), h());
}

void TexturePacker::TextureRedirection::copyRect(int textureX, int textureY, uint32_t *target, int targetX, int targetY, int targetW, uint8_t targetScale) const
{
	TextureInfos::copyRect(
		_image, textureX, textureY, pixelW(), _scale, _oldTexture.bpp(),
		target, targetX, targetY, targetW, targetScale
	);
}
