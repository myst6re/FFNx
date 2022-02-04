
#include "texture_packer.h"
#include "../renderer.h"
#include "../patch.h"

#include <png.h>

TexturePacker::TexturePacker(uint8_t *vram, int w, int h) :
    _vram(vram), _w(w), _h(h)
{
    memset(_vramTextureIds, 0, _w * _h);
}

void TexturePacker::setTexture(const char *name, const uint8_t *source, int x, int y, int w, int h)
{
    ffnx_trace("TexturePacker::%s %s x=%d y=%d w=%d h=%d\n", __func__, name, x, y, w, h);

    bool hasNamedTexture = name != nullptr && *name != '\0';
    uint8_t *vram = vram_seek(x, y);
    const int vramLineWidth = VRAM_DEPTH * _w;
    const int lineWidth = int(R5G5B5) * w;
    Texture tex(name, x, y, w, h);
    TextureId textureId = (vram - _vram) / VRAM_DEPTH;

    _textures[textureId] = tex;

    for (int i = 0; i < h; ++i)
    {
        memcpy(vram, source, lineWidth);

        if (hasNamedTexture)
        {
            const int vramPosition = (vram - _vram) / VRAM_DEPTH;

            for (int j = 0; j < w; ++j)
            {
                TextureId previousTextureId = _vramTextureIds[vramPosition + j];

                if (previousTextureId != 0 && _textures.contains(previousTextureId))
                {
                    _textures.erase(previousTextureId);
                }

                _vramTextureIds[vramPosition + j] = textureId;
            }
        }

        source += lineWidth;
        vram += vramLineWidth;
    }
}

void TexturePacker::getRect(uint8_t *target, int x, int y, int w, int h) const
{
    ffnx_trace("TexturePacker::%s x=%d y=%d w=%d h=%d\n", __func__, x, y, w, h);

    uint8_t *vram = vram_seek(x, y);
    const int vramLineWidth = VRAM_DEPTH * _w;
    const int lineWidth = VRAM_DEPTH * w;

    for (int i = 0; i < h; ++i)
    {
        memcpy(target, vram, lineWidth);

        /* const int vramPosition = (vram - _vram) / VRAM_DEPTH;

        for (int j = 0; j < w; ++j)
        {
            TextureId textureId = _vramTextureIds[vramPosition + j];

            if (textureId != 0 && _textures.contains(textureId))
            {
                _textures[textureId].getColor(x + j, y + i);
            }
        } */

        target += lineWidth;
        vram += vramLineWidth;
    }
}

void TexturePacker::scaledRect(uint8_t *sourceAndTarget, int w, int h, ColorFormat format, int scale)
{
    ffnx_trace("TexturePacker::%s w=%d h=%d format=%d scaled=%d\n", __func__, w, h, int(format), scale);

    if (scale <= 1) {
        return;
    }

    if (format == Format8Bit)
    {
        uint8_t *source = sourceAndTarget + w * h,
                *target = sourceAndTarget + (w * scale) * (h * scale);

        while (source > sourceAndTarget)
        {
            source -= 1;
            target -= scale;

            for (int i = 0; i < scale; ++i)
            {
                target[i] = *source;
            }
        }
    }
    else if (format == FormatR5G5B5 || format == FormatR5G5B5Hack)
    {
        uint16_t *source = (uint16_t *)sourceAndTarget + w * h,
                 *target = (uint16_t *)sourceAndTarget + (w * scale) * (h * scale);

        ffnx_trace("TexturePacker::%s sourceAndTarget=0x%X source=0x%X target=0x%X\n", __func__, sourceAndTarget, source, target);

        for (int y = 0; y < h; ++y)
        {
            uint16_t *source_line_start = source;

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
    else if (format == FormatR8G8B8A8 || format == FormatAlphaShift || FormatAlphaHeightHack)
    {
        uint32_t *source = (uint32_t *)sourceAndTarget + w * h,
                 *target = (uint32_t *)sourceAndTarget + (w * scale) * (h * scale);

        while (source > (uint32_t *)sourceAndTarget)
        {
            source -= 1;
            target -= scale;

            for (int i = 0; i < scale; ++i)
            {
                target[i] = *source;
            }
        }
    }
    else
    {
        ffnx_error("TexturePacker::%s Unknown format %d\n", __func__, format);
    }
}

void TexturePacker::getRect(uint8_t *buffer, int x, int y, int w, int h, PsDepth depth, ColorFormat targetFormat, int scale) const
{
    uint8_t *target = buffer;
    uint8_t *psxvram_buffer_pointer = vram_seek(x, y);
    const int vramLineWidth = VRAM_DEPTH * _w;
    const int lineWidth = int(depth) * w;

    ffnx_trace("TexturePacker::%s x=%d y=%d w=%d h=%d depth=%d vramLineWidth=%d lineWidth=%d\n", __func__, x, y, w, h, int(depth), vramLineWidth, lineWidth);

    if (depth == Indexed4Bit)
    {
        if (targetFormat == Format8Bit)
        {
            for (int i = 0; i < h; ++i)
            {
                for (int j = 0; j < w / 2; ++j)
                {
                    *target = *psxvram_buffer_pointer & 0xF;
                    *(target + 1) = (*psxvram_buffer_pointer >> 4) & 0xF;

                    psxvram_buffer_pointer += 1;
                    target += 2;
                }

                psxvram_buffer_pointer += vramLineWidth - w / 2;
            }
        }
        else
        {
            ffnx_error("TexturePacker::%s Cannot convert from Indexed4Bit to format %d\n", __func__, targetFormat);
        }
    }
    else if (depth == R5G5B5)
    {
        if (targetFormat == FormatR5G5B5)
        {
            for (int i = 0; i < h; ++i)
            {
                uint16_t *vram16 = (uint16_t *)psxvram_buffer_pointer;
                uint16_t *target16 = (uint16_t *)target;

                for (int j = 0; j < w; ++j)
                {
                    uint16_t color = *vram16;
                    // Convert color
                    *target16 = color & 0x3E0 | ((color & 0x1F) << 10) | (color >> 10) & 0x1F;

                    vram16 += 1;
                    target16 += 1;
                }

                psxvram_buffer_pointer += vramLineWidth;
                target += lineWidth;
            }
        }
        else if (targetFormat == FormatR5G5B5Hack)
        {
            for (int i = 0; i < h; ++i)
            {
                uint32_t *vram32 = (uint32_t *)psxvram_buffer_pointer;
                uint16_t *target16 = (uint16_t *)target;

                for (int j = 0; j < w / 2; ++j)
                {
                    uint32_t color = *vram32;
                    // Change one color / 2
                    *((uint32_t *)target16 + 1) = (color >> 10) & 0x1F001F | (2 * (color & 0x3E003E0 | ((color & 0xFFFF001F) << 10)));

                    vram32 += 1;
                    target16 += 1;
                }

                if ((w & 1) != 0) {
                    uint16_t color = *(int16_t *)vram32;
                    *target16 = (color >> 10) & 0x1F | (2 * ((color << 10) | color & 0x3E0));
                }

                psxvram_buffer_pointer += vramLineWidth;
                target += lineWidth;
            }
        }
        else
        {
            ffnx_error("TexturePacker::%s Cannot convert from R5G5B5 to format %d\n", __func__, targetFormat);
        }
    }
    else if (depth == Indexed8Bit)
    {
        if (targetFormat == Format8Bit)
        {
            for (int i = 0; i < h; ++i)
            {
                memcpy(target, psxvram_buffer_pointer, lineWidth);

                target += lineWidth;
                psxvram_buffer_pointer += vramLineWidth;
            }
        }
        else
        {
            ffnx_error("TexturePacker::%s Cannot convert from Indexed8Bit to format %d\n", __func__, targetFormat);
        }
    }

    scaledRect(buffer, w, h, targetFormat, scale);
}

void TexturePacker::getColors(uint8_t *target, int x, int y, int size, ColorFormat targetFormat, uint8_t colorShift) const
{
    // std::string textureName = textureNameFromInfos(x, y, w, h);

    uint16_t *psxvram_buffer_pointer = (uint16_t *)vram_seek(x, y);

    ffnx_trace("TexturePacker::%s x=%d y=%d size=%d\n", __func__, x, y, size);
    bool has_weird_color = false;

    if (targetFormat == FormatR8G8B8A8 || targetFormat == FormatAlphaShift || targetFormat == FormatAlphaHeightHack)
    {
        for (int i = 0; i < size; ++i)
        {
            uint16_t color = *psxvram_buffer_pointer;

            if (targetFormat == FormatAlphaShift || targetFormat == FormatAlphaHeightHack)
            {
                uint8_t r = (color >> 10) & 0x1F;
                uint8_t g = (color >> 5) & 0x1F;
                uint8_t b = color & 0x1F;

                if (targetFormat == FormatAlphaShift)
                {
                    uint32_t alpha = (r + g + b) << (1 - colorShift);

                    target[0] = 0x00;
                    target[1] = 0x00;
                    target[2] = 0x00;
                    target[3] = alpha > 0xFF ? 0xFF : alpha;
                }
                else if (r >= 8 || g >= 8 || b >= 8)
                {
                    target[0] = 0x08; // Eight
                    target[1] = 0x00;
                    target[2] = 0x00;
                    target[3] = 0xFF;
                }
                else
                {
                    target[0] = 0x00;
                    target[1] = 0x00;
                    target[2] = 0x00;
                    target[3] = 0x00;
                }
            }
            else if (targetFormat == FormatAlphaHeightHack)
            {

            }
            else if (color != 0)
            {
                if (color == 0x8000)
                {
                    target[0] = 0x08; // Eight
                    target[1] = 0x00;
                    target[2] = 0x00;
                    has_weird_color = true;
                }
                else
                {
                    target[0] = 0xFF * ((color >> 10) & 0x1F) / 0x1F;
                    target[1] = 0xFF * ((color >> 5) & 0x1F) / 0x1F;
                    target[2] = 0xFF * (color & 0x1F) / 0x1F;
                }

                target[3] = 0x7F;
            }
            else
            {
                target[0] = 0x01;
                target[1] = 0x00;
                target[2] = 0x00;
                target[3] = 0x00;
            }

            target += 4;
            psxvram_buffer_pointer++;
        }
    }
    else
    {
        ffnx_error("TexturePacker::%s Cannot convert to format %d\n", __func__, targetFormat);
    }

    if (has_weird_color)
    {
        ffnx_warning("TexturePacker::%s weird color detected\n", __func__);
    }
}

void TexturePacker::copyRect(int sourceX, int sourceY, int targetX, int targetY, int w, int h)
{
    ffnx_trace("TexturePacker::%s sourceX=%d sourceY=%d targetX=%d targetY=%d w=%d h=%d\n", __func__, sourceX, sourceY, targetX, targetY, w, h);

    uint8_t *source = vram_seek(sourceX, sourceY);
    uint8_t *target = vram_seek(targetX, targetY);

    for (int i = 0; i < h; ++i)
    {
        memcpy(target, source, w * int(VRAM_DEPTH));

        target += _w * VRAM_DEPTH;
        source += _w * VRAM_DEPTH;
    }
}

void TexturePacker::fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, PsDepth depth)
{
    ffnx_trace("TexturePacker::%s x=%d y=%d w=%d h=%d rgb=(%X, %X, %X)\n", __func__, x, y, w, h, r, g, b);

    const uint16_t color = (r >> 3) | (32 * ((g >> 3) | (32 * (b >> 3))));
    uint16_t *psxvram_buffer_pointer = (uint16_t *)vram_seek(x, y);

    for (int i = 0; i < h; ++i)
    {
        std::fill_n(psxvram_buffer_pointer, w, color);

        psxvram_buffer_pointer += _w;
    }
}

bool TexturePacker::saveVram(const char *fileName) const
{
    uint32_t *vram = new uint32_t[_w * _h];
    ffnx_trace("%s 1\n", __func__);
    vramToR8G8B8(vram);
    ffnx_trace("%s 2\n", __func__);

    bool ret = newRenderer.saveTexture(
        fileName,
        _w,
        _h,
        vram
    );

    delete[] vram;

    return ret;
}

void TexturePacker::vramToR8G8B8(uint32_t *output) const
{
    uint16_t *vram = (uint16_t *)_vram;

    for (int y = 0; y < _h; ++y)
    {
        for (int x = 0; x < _w; ++x)
        {
            *output = fromR5G5B5Color(*vram);

            ++output;
            ++vram;
        }
    }
}

TexturePacker::Texture::Texture() :
    _x(0), _y(0), _w(0), _h(0)
{
}

TexturePacker::Texture::Texture(
    const char *name,
    int x, int y, int w, int h
) : _name(name), _x(x), _y(y), _w(w), _h(h)
{
}

bool TexturePacker::Texture::contains(int x, int y, int w, int h) const
{
    int x2 = x + w,
        y2 = y + h,
        _x2 = _x + _w,
        _y2 = _y + _h;

    return x >= _x && x < _x2 && y >= _y && y < _y2
        && x2 > _x && x2 <= _x2 && y2 > _y && y2 <= _y2;
}

bool TexturePacker::Texture::intersect(int x, int y, int w, int h) const
{
    int x2 = x + w,
        y2 = y + h,
        _x2 = _x + _w,
        _y2 = _y + _h;

    return (x >= _x && x < _x2 || _x >= x && _x < x2)
        && (y >= _y && y < _y2 || _y >= y && _y < y2);
}

bool TexturePacker::Texture::match(int x, int y, int w, int h) const
{
    return _x == x && _y == y && _w == w && _h == h;
}

bool TexturePacker::Texture::operator==(const Texture &other) const
{
    return match(other._x, other._y, other._w, other._h);
}
