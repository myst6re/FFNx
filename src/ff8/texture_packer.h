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
#include <bimg/bimg.h>

typedef uint32_t ModdedTextureId;

#define VRAM_WIDTH 1024
#define VRAM_HEIGHT 512
#define VRAM_DEPTH int(TexturePacker::R5G5B5)
#define INVALID_TEXTURE 0xFFFFFFFF
#define MAX_SCALE 10

class TexturePacker {
public:
    enum PsDepth {
        Indexed4Bit = 0,
        Indexed8Bit,
        R5G5B5
    };
    enum ColorFormat {
        Format8Bit,
        FormatR5G5B5,
        FormatR5G5B5Hack,
        FormatR8G8B8A8,
        FormatAlphaShift,
        FormatAlphaHeightHack,
    };
    explicit TexturePacker();
    inline void setVram(uint8_t *vram) {
        _vram = vram;
    }
    void setTexture(const char *name, const uint8_t *texture, int x, int y, int w, int h);
    void getRect(uint8_t *target, int x, int y, int w, int h) const;
    inline uint8_t getMaxScale() const {
        return _maxScaleCached;
    }
    inline bool hasModdedTextures() const {
        return ! _moddedTextures.empty();
    }
    void registerTiledTex(uint8_t *target, int x, int y, int w, int h, uint8_t scale);
    void getRect(uint8_t *target, int x, int y, int w, int h, ColorFormat targetFormat, uint8_t scale = 1) const;
    void getColors(uint8_t *target, int x, int y, int size, ColorFormat targetFormat, uint8_t colorShift = 0) const;
    void copyRect(int sourceX, int sourceY, int targetX, int targetY, int w, int h);
    void fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, PsDepth depth = R5G5B5);
    void drawModdedTextures(const uint8_t *texData, uint32_t paletteIndex, uint32_t *target);

    bool saveVram(const char *fileName) const;
private:
    static void scaledRect(uint8_t *sourceAndTarget, int w, int h, ColorFormat format, int scale);
    inline uint8_t *vram_seek(int x, int y) const {
        return _vram + VRAM_DEPTH * (x + y * VRAM_WIDTH);
    }
    void updateMaxScale();
    void drawModdedTextures(uint32_t *target, int x, int y, int w, int h, uint8_t scale);

    void vramToR8G8B8(uint32_t *output) const;
    static inline uint32_t fromR5G5B5Color(uint16_t color) {
        uint8_t r = color & 31,
                g = (color >> 5) & 31,
                b = (color >> 10) & 31;

        return (0xffu << 24) |
            ((((r << 3) + (r >> 2)) & 0xffu) << 16) |
            ((((g << 3) + (g >> 2)) & 0xffu) << 8) |
            (((b << 3) + (b >> 2)) & 0xffu);
    }
    class Texture {
    public:
        Texture();
        Texture(
            const char *name,
            int x, int y, int w, int h
        );
        inline const std::string &name() const {
            return _name;
        }
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
        uint8_t scale() const;
        bool createImage();
        void destroyImage();
        uint32_t getColor(int scaledX, int scaledY) const;
        bool contains(int x, int y, int w, int h) const;
        bool intersect(int x, int y, int w, int h) const;
        bool match(int x, int y, int w, int h) const;
        bool operator==(const Texture &other) const;
        bool operator!=(const Texture &other) const {
            return ! (*this == other);
        }
    private:
        bimg::ImageContainer *_image;
        std::string _name;
        int _x, _y;
        int _w, _h;
    };
    struct TiledTex {
        TiledTex();
        TiledTex(int x, int y, int w, int h, uint8_t scale);
        int x, y;
        int w, h;
        uint8_t scale;
    };

    uint8_t *_vram; // uint16_t[VRAM_WIDTH * VRAM_HEIGHT] aka uint8_t[VRAM_WIDTH * VRAM_HEIGHT * VRAM_DEPTH]
    std::map<const uint8_t *, TiledTex> _tiledTexs;
    ModdedTextureId _vramTextureIds[VRAM_WIDTH * VRAM_HEIGHT];
    std::map<ModdedTextureId, Texture> _moddedTextures;
    uint8_t _maxScaleCached;
};
