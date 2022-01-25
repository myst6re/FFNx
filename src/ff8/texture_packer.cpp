
#include "texture_packer.h"
#include "../renderer.h"
#include "../patch.h"

#include <png.h>

TexturePacker::TexturePacker(uint8_t *vram, int w, int h, Depth depth) :
    _vram(vram), _w(w), _h(h), _depth(depth)
{
}

void TexturePacker::setTexture(const std::string &name, const uint8_t *texture, int x, int y, int w, int h, Depth depth)
{
    _textures[name] = Texture(name, x, y, w, h, depth);

    uint8_t *vram = vram_seek(x, y);
    int vramLineWidth = _depth * _w;
    int lineWidth = depth == R5B5G5 || depth == Indexed8Bit
        ? depth * w
        : w / 2;

    for (int i = 0; i < h; ++i)
    {
        memcpy(vram, texture, lineWidth);

        texture += lineWidth;
        vram += vramLineWidth;
    }
}

void TexturePacker::getTexture(uint8_t *texture, int x, int y, int w, int h, Depth depth)
{

}

void TexturePacker::copyTexture(int sourceX, int sourceY, int targetX, int targetY, int w, int h, Depth depth)
{

}

void TexturePacker::fill(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, Depth depth)
{
    ffnx_trace("%s x=%d y=%d w=%d h=%d rgb=(%X, %X, %X)\n", __func__, x, y, w, h, r, g, b);

    const uint16_t color = (r >> 3) | (32 * ((g >> 3) | (32 * (b >> 3))));
    uint16_t *psxvram_buffer_pointer = (uint16_t *)vram_seek(x, y);

    for (int i = 0; i < h; ++i)
    {
        if (w != 0)
        {
            std::fill_n(psxvram_buffer_pointer, w, color);
        }

        psxvram_buffer_pointer += _w;
    }
}

std::string TexturePacker::textureNameFromInfos(int x, int y, int w, int h, Depth depth)
{
    return "";
}

bool TexturePacker::saveVram(const char *fileName)
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

void TexturePacker::vramToR8G8B8(uint32_t *output)
{
    uint16_t *vram = (uint16_t *)_vram;

    for (int y = 0; y < _h; ++y) {
        for (int x = 0; x < _w; ++x) {
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
    const std::string &name,
    int x, int y, int w, int h,
    TexturePacker::Depth depth
) : _name(name), _x(x), _y(y), _w(w), _h(h), _depth(depth)
{
}
