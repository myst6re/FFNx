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
#include <list>

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
        FormatR8G8B8A8
    };
    TexturePacker(uint8_t *vram, int w, int h, PsDepth depth = R5G5B5);
    void setTexture(const char *name, const uint8_t *texture, int x, int y, int w, int h, PsDepth depth = R5G5B5);
    void getTexture(uint8_t *target, int x, int y, int w, int h, PsDepth depth = R5G5B5) const;
    void getTexture(uint8_t *target, int x, int y, int w, int h, PsDepth sourceDepth, ColorFormat targetFormat) const;
    void getColors(uint8_t *target, int x, int y, int size, ColorFormat targetFormat) const;
    void copyTexture(int sourceX, int sourceY, int targetX, int targetY, int w, int h);
    void fill(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, PsDepth depth = R5G5B5);

    std::string textureNameFromInfos(int x, int y, int w, int h) const;

    bool saveVram(const char *fileName) const;
private:
    void setTextureName(const char *name, int x, int y, int w, int h, PsDepth depth);
    inline uint8_t *vram_seek(int x, int y) const {
        return _vram + _depth * (x + y * _w);
    }

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
            int x, int y, int w, int h,
            TexturePacker::PsDepth depth
        );
        inline const std::string &name() const {
            return _name;
        }
        bool contains(int x, int y, int w, int h) const;
        bool intersect(int x, int y, int w, int h) const;
        bool match(int x, int y, int w, int h) const;
        bool operator==(const Texture &other) const;
        bool operator!=(const Texture &other) const {
            return ! (*this == other);
        }
    private:
        std::string _name;
        int _x, _y;
        int _w, _h;
        TexturePacker::PsDepth _depth;
    };

    uint8_t *_vram;
    int _w, _h;
    PsDepth _depth;
    std::list<Texture> _textures;
};
