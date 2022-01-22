
#include "texture_packer.h"

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

}

std::string TexturePacker::textureNameFromInfos(int x, int y, int w, int h, Depth depth)
{

}

bool TexturePacker::saveVram(const char *fileName)
{
    int y;

    FILE *fp = fopen(fileName, "wb");
    if(fp == nullptr) {
        return false;
    }

    png_struct *png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == nullptr) {
        return false;
    }

    png_info *info = png_create_info_struct(png);
    if (info == nullptr) {
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        return false;
    }

    png_init_io(png, fp);

    png_set_IHDR(
        png,
        info,
        _w, _h,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    uint32_t *vram = new uint32_t[_w * _h];
    vramToR8G8B8(vram);

    png_write_image(png, (uint8_t **)&vram);
    png_write_end(png, nullptr);

    fclose(fp);

    png_destroy_write_struct(&png, &info);
}

void TexturePacker::vramToR8G8B8(uint32_t *output)
{
    uint16_t *vram = (uint16_t *)_vram;

    for (int y = 0; y < _h; ++y) {
        for (int x = 0; x < _w; ++x) {
            *output = fromR5G5B5Color(*vram);

            ++output;
        }
    }
}
