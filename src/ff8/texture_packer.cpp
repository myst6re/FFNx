
#include "texture_packer.h"
#include "../renderer.h"
#include "../saveload.h"
#include "../patch.h"
#include "../image/tim.h"

#include <png.h>

TexturePacker::TexturePacker() :
	_vram(nullptr)
{
	memset(_vramTextureIds, INVALID_TEXTURE, VRAM_WIDTH * VRAM_HEIGHT);
}

void TexturePacker::setTexture(const char *name, const uint8_t *source, int x, int y, int w, int h, uint8_t bpp, bool isPal)
{
	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s x=%d y=%d w=%d h=%d bpp=%d\n", __func__, x, y, w, h, bpp);

	bool hasNamedTexture = name != nullptr && *name != '\0';
	uint8_t *vram = vramSeek(x, y);
	const int vramLineWidth = VRAM_DEPTH * VRAM_WIDTH;
	const int lineWidth = VRAM_DEPTH * w;
	Texture tex;
	ModdedTextureId textureId = INVALID_TEXTURE;

	if (hasNamedTexture)
	{
		if (isPal)
		{
			for (std::pair<const ModdedTextureId, Texture> &pair: _moddedTextures)
			{
				Texture &t = pair.second;

				if (t.name().compare(name) == 0)
				{
					if (trace_all || trace_vram) ffnx_info("TexturePacker::%s: associate palette with existing texture\n", __func__);
					t.setPal(TextureInfos(x, y, w, h, bpp));
					_moddedTextures[pair.first] = t;
					break;
				}
			}
		}
		else
		{
			tex = Texture(name, x, y, w, h, bpp);

			if (tex.createImage() || save_textures)
			{
				textureId = x + y * VRAM_WIDTH;
				_moddedTextures[textureId] = tex;
			}
		}
	}

	for (int i = 0; i < h; ++i)
	{
		memcpy(vram, source, lineWidth);

		int vramY = y + i;

		for (int j = 0; j < w; ++j)
		{
			int vramX = x + j;
			int key = vramX + vramY * VRAM_WIDTH;
			ModdedTextureId previousTextureId = _vramTextureIds[key];

			if (previousTextureId != INVALID_TEXTURE && _moddedTextures.contains(previousTextureId))
			{
				Texture &previousTexture = _moddedTextures[previousTextureId];

				if (trace_all || trace_vram) ffnx_info("TexturePacker::%s: clear texture %s (textureId = %d)\n", __func__, previousTexture.name().c_str(), previousTextureId);

				for (int prevY = 0; prevY < previousTexture.h(); ++prevY)
				{
					int clearKey = previousTexture.x() + (previousTexture.y() + prevY) * VRAM_WIDTH;
					std::fill_n(_vramTextureIds + clearKey, previousTexture.w(), INVALID_TEXTURE);
				}

				previousTexture.destroyImage();
				_moddedTextures.erase(previousTextureId);
			}

			_vramTextureIds[key] = textureId;
		}

		source += lineWidth;
		vram += vramLineWidth;
	}

	updateMaxScale();
}

void TexturePacker::updateMaxScale()
{
	_maxScaleCached = 1;

	for (const std::pair<ModdedTextureId, Texture> &pair: _moddedTextures)
	{
		const Texture &tex = pair.second;
		const uint8_t texScale = tex.scale();

		if (texScale > _maxScaleCached) {
			_maxScaleCached = texScale;
		}
	}

	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s scale=%d\n", __func__, _maxScaleCached);
}

bool TexturePacker::drawModdedTextures(const uint8_t *texData, uint32_t paletteIndex, uint32_t paletteEntries, uint32_t *target, int targetW, int targetH, uint8_t scale)
{
	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s pointer=0x%X paletteIndex=%d, paletteEntries=%d\n", __func__, texData, paletteIndex, paletteEntries);

	// Not supported for now
	/* if (paletteIndex != 0)
	{
		return false;
	} */

	if (_tiledTexs.contains(texData))
	{
		char fileName[MAX_PATH];
		struct stat dummy;
		const TiledTex &tex = _tiledTexs[texData];

		if (tex.bpp == 0)
		{
			snprintf(fileName, MAX_PATH, "texture-page-%d-%d.png", int(texData), paletteIndex);

			newRenderer.saveTexture(
				fileName,
				targetW * scale,
				targetH * scale,
				target
			);
		}
		bool ret = drawModdedTextures(target, tex, targetW, targetH, scale, paletteIndex, paletteEntries);
		// _tiledTexs.erase(texData);

		if (tex.bpp == 0 && ret)
		{
			snprintf(fileName, MAX_PATH, "modded-texture-%d-%d.png", int(texData), paletteIndex);

			newRenderer.saveTexture(
				fileName,
				targetW * scale,
				targetH * scale,
				target
			);
		}

		return ret;
	}

	if (trace_all || trace_vram) ffnx_warning("TexturePacker::%s Unknown tex data\n", __func__);

	return false;
}

bool TexturePacker::drawModdedTextures(uint32_t *target, const TiledTex &tiledTex, int targetW, int targetH, uint8_t scale, uint32_t paletteIndex, uint32_t paletteEntries)
{
	if (_moddedTextures.empty())
	{
		return false;
	}

	uint32_t *target_origin = target;
	int w = targetW;

	if (tiledTex.bpp <= 2)
	{
		w /= 4 >> tiledTex.bpp;
	}
	else
	{
		ffnx_warning("%s: Unknown bpp %d\n", tiledTex.bpp);

		return false;
	}

	bool hasModdedTexture = false;
	std::unordered_set<ModdedTextureId> matchedTextures;

	if (save_textures)
	{
		matchedTextures.reserve(_moddedTextures.size());
	}

	for (int i = 0; i < targetH; ++i)
	{
		int vramY = tiledTex.y + i;

		for (int j = 0; j < w; ++j)
		{
			int vramX = tiledTex.x + j;
			ModdedTextureId textureId = _vramTextureIds[vramY * VRAM_WIDTH + vramX];

			if (textureId != INVALID_TEXTURE && _moddedTextures.contains(textureId))
			{
				if (save_textures)
				{
					matchedTextures.insert(textureId);
				}
				else
				{
					Texture &texture = _moddedTextures[textureId];
					int textureX = vramX - texture.x(),
						textureY = vramY - texture.y();

					ffnx_info("toto %d\n", texture.pal().x());

					if (scale == 1)
					{
						if (tiledTex.bpp == 0)
						{
							*target = texture.getColor(textureX * 4 + 0, textureY);
							target += 1;
							*target = texture.getColor(textureX * 4 + 1, textureY);
							target += 1;
							*target = texture.getColor(textureX * 4 + 2, textureY);
							target += 1;
							*target = texture.getColor(textureX * 4 + 3, textureY);
						}
						else if (tiledTex.bpp == 1)
						{
							*target = texture.getColor(textureX * 2 + 0, textureY);
							target += 1;
							*target = texture.getColor(textureX * 2 + 1, textureY);
						}
						else if (tiledTex.bpp == 2)
						{
							*target = texture.getColor(textureX * 1 + 0, textureY);
						}
					}
					else
					{
						/* for (int yTex = 0; yTex < scale; ++yTex)
						{
							for (int xTex = 0; xTex < scale; ++xTex)
							{

							}
						} */
					}

					hasModdedTexture = true;
				}
			}
			else
			{
				if (tiledTex.bpp == 0)
				{
					target += 3;
				}
				else if (tiledTex.bpp == 1)
				{
					target += 1;
				}
			}

			target += 1;
		}
	}

	if (trace_all || trace_vram) ffnx_trace("TexturePacker::%s x=%d y=%d bpp=%d w=%d targetW=%d targetH=%d scale=%d hasModdedTexture=%d\n", __func__, tiledTex.x, tiledTex.y, tiledTex.bpp, w, targetW, targetH, scale, hasModdedTexture);

	if (save_textures)
	{
		for (ModdedTextureId textureId: matchedTextures)
		{
			const Texture &texture = _moddedTextures[textureId];

			if (trace_all || trace_vram) ffnx_trace(
				"%s: save %s pos=(%d %d) size=(%d %d) pal_pos(%d %d) pal_size(%d %d)\n", __func__,
				texture.name().c_str(), texture.x(), texture.y(), texture.w(), texture.h(), texture.pal().x(), texture.pal().y(), texture.pal().w(), texture.pal().h()
			);

			uint8_t *data = (uint8_t *)driver_malloc(texture.w() * texture.h() * VRAM_DEPTH),
				*palData = nullptr, *palDataCursor;

			getVramRect(data, texture);

			if (texture.hasPal())
			{
				const TextureInfos &pal = texture.pal();
				size_t palDataSize = pal.w() * pal.h() * VRAM_DEPTH;
				palData = (uint8_t *)driver_malloc(palDataSize);
				uint8_t *maxPalData = palData + palDataSize;

				getVramRect(palData, pal);

				palDataCursor = palData + paletteIndex * paletteEntries;
				uint16_t colorCount = texture.bpp() == 0 ? 16 : 256;

				if (palDataCursor + colorCount * VRAM_DEPTH >= maxPalData)
				{
					ffnx_warning("%s: palette overflow\n", __func__);
					palDataCursor = palData;
				}
			}

			ff8_tim tim = texture.toTim(data, (uint16_t *)palDataCursor);

			save_tim(texture.name().c_str(), texture.bpp(), &tim, paletteIndex);

			driver_free(data);
			if (palData != nullptr)
			{
				driver_free(palData);
			}
		}

		ffnx_trace(
				"%s2\n", __func__
			);

		return false;
	}

	return hasModdedTexture;
}

void TexturePacker::getVramRect(uint8_t *target, const TextureInfos &texture) const
{
	uint8_t *vram = vramSeek(texture.x(), texture.y());
	const int vramLineWidth = VRAM_DEPTH * VRAM_WIDTH;
	const int lineWidth = VRAM_DEPTH * texture.w();

	for (int i = 0; i < texture.h(); ++i)
	{
		memcpy(target, vram, lineWidth);

		target += lineWidth;
		vram += vramLineWidth;
	}
}

void TexturePacker::registerTiledTex(uint8_t *target, int x, int y, uint8_t bpp)
{
	if (trace_all || trace_vram) ffnx_trace("%s pointer=0x%X x=%d y=%d bpp=%d\n", __func__, target, x, y, bpp);

	_tiledTexs[target] = TiledTex(x, y, bpp);
}

bool TexturePacker::saveVram(const char *fileName) const
{
	uint32_t *vram = new uint32_t[VRAM_WIDTH * VRAM_HEIGHT];
	vramToR8G8B8(vram);

	bool ret = newRenderer.saveTexture(
		fileName,
		VRAM_WIDTH,
		VRAM_HEIGHT,
		vram
	);

	delete[] vram;

	return ret;
}

void TexturePacker::vramToR8G8B8(uint32_t *output) const
{
	uint16_t *vram = (uint16_t *)_vram;

	for (int y = 0; y < VRAM_HEIGHT; ++y)
	{
		for (int x = 0; x < VRAM_WIDTH; ++x)
		{
			*output = fromR5G5B5Color(*vram);

			++output;
			++vram;
		}
	}
}

TexturePacker::TextureInfos::TextureInfos() :
	_x(0), _y(0), _w(0), _h(0), _bpp(255)
{
}

TexturePacker::TextureInfos::TextureInfos(
	int x, int y, int w, int h,
	uint8_t bpp
) : _x(x), _y(y), _w(w), _h(h), _bpp(bpp)
{
}

TexturePacker::Texture::Texture() :
	TextureInfos(), _image(nullptr), _name(""), _pal(TextureInfos()), _logged(false)
{
}

TexturePacker::Texture::Texture(
	const char *name,
	int x, int y, int w, int h,
	uint8_t bpp
) : TextureInfos(x, y, w, h, bpp), _image(nullptr), _name(name), _pal(TextureInfos()), _logged(false)
{
}

bool TexturePacker::Texture::createImage(uint8_t palette_index)
{
	char filename[MAX_PATH];

	if(trace_all || trace_loaders || trace_vram) ffnx_trace("texture file name (VRAM): %s\n", _name.c_str());

	for (int idx = 0; idx < mod_ext.size(); idx++)
	{
		_snprintf(filename, sizeof(filename), "%s/%s/%s_%02i.%s", basedir, mod_path.c_str(), _name.c_str(), palette_index, mod_ext[idx].c_str());
		_image = newRenderer.createImageContainer(filename, bimg::TextureFormat::BGRA8);

		if (_image != nullptr)
		{
			if (trace_all || trace_loaders || trace_vram) ffnx_trace("Using texture: %s\n", filename);

			return true;
		}
		else if (trace_all || trace_loaders || trace_vram)
		{
			ffnx_warning("Texture does not exist, skipping: %s\n", filename);
		}
	}

	return false;
}

void TexturePacker::Texture::destroyImage()
{
	if (_image != nullptr) {
		bimg::imageFree(_image);
	}
}

uint8_t TexturePacker::Texture::scale() const
{
	if (_image == nullptr) {
		return 1;
	}

	int w = _w;

	if (_bpp == 0)
	{
		w *= 4;
	}
	else if (_bpp == 1)
	{
		w *= 2;
	}

	if (_image->m_width < w || _image->m_height < _h || _image->m_width % w != 0 || _image->m_height % _h != 0)
	{
		ffnx_warning("Texture size must be scaled to the original texture size: %s\n", _name.c_str());

		return 1;
	}

	int scaleW = _image->m_width / w, scaleH = _image->m_height / _h;

	if (scaleW != scaleH)
	{
		ffnx_warning("Texture size must have the same ratio as the original texture: %s\n", _name.c_str());

		return 1;
	}

	if (scaleW > MAX_SCALE)
	{
		ffnx_warning("Texture size cannot exceed original size * %d: %s\n", MAX_SCALE, _name.c_str());

		return MAX_SCALE;
	}

	return scaleW;
}

uint32_t TexturePacker::Texture::getColor(int scaledX, int scaledY)
{
	if (!_logged) {
		ffnx_info("%s: %d %d x=%d y=%d w=%d h=%d img_w=%d img_h=%d bpp=%d\n", _name.c_str(), scaledX, scaledY, _x, _y, _w, _h, _image->m_width, _image->m_height, _bpp);
		_logged = true;
	}
	return ((uint32_t *)_image->m_data)[scaledX + scaledY * _image->m_width];
}

ff8_tim TexturePacker::Texture::toTim(uint8_t *imgData, uint16_t *palData) const
{
	ff8_tim tim = ff8_tim();
	tim.img_x = _x;
	tim.img_y = _y;
	tim.img_w = _w;
	tim.img_h = _h;
	tim.img_data = imgData;

	if (hasPal())
	{
		tim.pal_x = _pal.x();
		tim.pal_y = _pal.y();
		tim.pal_w = _pal.w();
		tim.pal_h = _pal.h();
		tim.pal_data = palData;
	}

	return tim;
}

TexturePacker::TiledTex::TiledTex()
 : x(0), y(0), bpp(0)
{
}

TexturePacker::TiledTex::TiledTex(
	int x, int y, uint8_t bpp
) : x(x), y(y), bpp(bpp)
{
}
