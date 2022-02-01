
#include "texture_packer.h"
#include "../renderer.h"
#include "../patch.h"

#include <png.h>

TexturePacker::TexturePacker(uint8_t *vram, int w, int h, Depth depth) :
    _vram(vram), _w(w), _h(h), _depth(depth)
{
}

void TexturePacker::setTexture(const char *name, const uint8_t *texture, int x, int y, int w, int h, Depth depth)
{
    ffnx_trace("TexturePacker::%s %s x=%d y=%d w=%d h=%d\n", __func__, name, x, y, w, h);

    setTextureName(name, x, y, w, h, depth);

    uint8_t *vram = vram_seek(x, y);
    const int vramLineWidth = _depth * _w;
    const int lineWidth = depth == Indexed4Bit
        ? w / 2
        : int(depth) * w;

    for (int i = 0; i < h; ++i)
    {
        memcpy(vram, texture, lineWidth);

        texture += lineWidth;
        vram += vramLineWidth;
    }
}

void TexturePacker::getTexture(uint8_t *target, int x, int y, int w, int h, Depth depth) const
{
    ffnx_trace("TexturePacker::%s x=%d y=%d w=%d h=%d\n", __func__, x, y, w, h);

    std::string textureName = textureNameFromInfos(x, y, w, h);

    uint8_t *psxvram_buffer_pointer = vram_seek(x, y);
    const int vramLineWidth = depth == Indexed4Bit
        ? _w / 2
        : int(_depth) * _w;
    const int lineWidth = int(depth) * w;

    for (int i = 0; i < h; ++i)
    {
        memcpy(target, psxvram_buffer_pointer, lineWidth);

        target += lineWidth;
        psxvram_buffer_pointer += vramLineWidth;
    }
}

void TexturePacker::getTextureColors(uint8_t *target, int x, int y, int w, int h, Depth depth) const
{
    std::string textureName = textureNameFromInfos(x, y, w, h);

    uint8_t *psxvram_buffer_pointer = vram_seek(x, y);
    const int vramLineWidth = _depth * _w;
    const int lineWidth = int(depth) * w;

    ffnx_trace("TexturePacker::%s x=%d y=%d w=%d h=%d depth=%d vramLineWidth=%d lineWidth=%d\n", __func__, x, y, w, h, int(depth), vramLineWidth, lineWidth);

    if (depth == Indexed4Bit)
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
    else if (depth == R5B5G5)
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
    else
    {
        for (int i = 0; i < h; ++i)
        {
            memcpy(target, psxvram_buffer_pointer, lineWidth);

            target += lineWidth;
            psxvram_buffer_pointer += vramLineWidth;
        }
    }
}

void TexturePacker::copyTexture(int sourceX, int sourceY, int targetX, int targetY, int w, int h)
{
    ffnx_trace("TexturePacker::%s sourceX=%d sourceY=%d targetX=%d targetY=%d w=%d h=%d\n", __func__, sourceX, sourceY, targetX, targetY, w, h);

    uint8_t *source = vram_seek(sourceX, sourceY);
    uint8_t *target = vram_seek(targetX, targetY);

    for (int i = 0; i < h; ++i)
    {
        memcpy(target, source, w * int(_depth));

        target += _w * _depth;
        source += _w * _depth;
    }
}

void TexturePacker::fill(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, Depth depth)
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

void TexturePacker::setTextureName(const char *name, int x, int y, int w, int h, Depth depth)
{
    // FIXME: optimize with better lookup and cache
    std::list<Texture>::const_iterator it = _textures.begin();

    while (it != _textures.end())
    {
        const Texture &texture = *it;

        if (texture.intersect(x, y, w, h))
        {
            ffnx_trace("%s: texture deleted %s\n", __func__, texture.name().c_str());

            _textures.erase(it);
        }

        ++it;
    }

    _textures.push_back(Texture(name, x, y, w, h, depth));
}

std::string TexturePacker::textureNameFromInfos(int x, int y, int w, int h) const
{
    // FIXME: optimize with better lookup and cache
    std::list<Texture>::const_iterator it = _textures.begin();

    while (it != _textures.end())
    {
        const Texture &texture = *it;

        if (texture.intersect(x, y, w, h))
        {
            ffnx_trace("%s: texture found %s\n", __func__, texture.name().c_str());

            return texture.name();
        }

        ++it;
    }

    return "";
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
    _x(0), _y(0), _w(0), _h(0), _depth(R5B5G5)
{
}

TexturePacker::Texture::Texture(
    const char *name,
    int x, int y, int w, int h,
    TexturePacker::Depth depth
) : _name(name), _x(x), _y(y), _w(w), _h(h), _depth(depth)
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
