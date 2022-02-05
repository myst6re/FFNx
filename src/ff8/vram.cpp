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
#include "vram.h"

#include "../ff8.h"
#include "../patch.h"
#include "texture_packer.h"

char nextTextureName[MAX_PATH] = "";
int next_psxvram_x = -1;
int next_psxvram_y = -1;

#define VRAM_SEEK(pointer, x, y) \
    (pointer + VRAM_DEPTH * (x + y * VRAM_WIDTH))

char *psxvram_buffer = (char *)0x1B474F0; // Size: sizeof(int) * 58106 -> 232424
char **psxvram_current_clut_ptr = ((char **)0x1CA8680);
char **psxvram_current_texture_ptr = ((char **)0x1CA86A8);

TexturePacker texturePacker((uint8_t *)psxvram_buffer);

inline char *vram_seek(int x, int y)
{
    return VRAM_SEEK(psxvram_buffer, x, y);
}

inline char *vram_current_texture_seek(int x, int y)
{
    return VRAM_SEEK(*psxvram_current_texture_ptr, x, y);
}

inline char *vram_current_clut_seek(int x, int y)
{
    return VRAM_SEEK(*psxvram_current_clut_ptr, x, y);
}

inline uint16_t color_add(uint16_t color1, uint16_t color2)
{
    unsigned int r = (color1 & 0x1F) + (color2 & 0x1F);

    if (r >= 0x1F)
    {
        r = 0x1F;
    }

    unsigned int g = (color1 & 0x3E0) + (color2 & 0x3E0);

    if (g >= 0x3E0)
    {
        g = 0x3E0;
    }

    unsigned int b = (color1 & 0x7C00) + (color2 & 0x7C00);

    if (b >= 0x7C00)
    {
        b = 0x7C00;
    }

    return r | g | b;
}

inline uint16_t color_sub(uint16_t color1, uint16_t color2)
{
    return ((((color1 & 0x3E0) - (color2 & 0x3E0)) & 0x8000) == 0 ? (color1 & 0x3E0) - (color2 & 0x3E0) : 0)
        | ((((color1 & 0x1F) - (color2 & 0x1F)) & 0x8000) == 0 ? (color1 & 0x1F) - (color2 & 0x1F) : 0)
        | ((((color1 & 0x7C00) - (color2 & 0x7C00)) & 0x8000) == 0 ? (color1 & 0x7C00) - (color2 & 0x7C00) : 0);
}

/* void ff8_intercept_store_vram(int *vram_buffer)
{
    ffnx_trace("%s 0x%X\n", __func__, vram_buffer);

    ((void(*)(int *))0x464210)(vram_buffer);

    // vram_buffer_point_dword_1CB5D04 = (int)vram_buffer;
} */

int ff8_replace_call0(int a1, int header_with_bit_depth, int pos_x6y10, int a4, DWORD *a5)
{
    ffnx_trace("%s\n", __func__);

    return ((int(*)(int, int, int, int, DWORD*))0x462DE0)(a1, header_with_bit_depth, pos_x6y10, a4, a5);
}

int ff8_replace_call1(int a1, int header_with_bit_depth, int16_t pos_x6y9, int a4)
{
    ffnx_trace("%s\n", __func__);

    return ((int(*)(int, int, int16_t, int))0x461A20)(a1, header_with_bit_depth, pos_x6y9, a4);
}

int ff8_replace_call2(int a1, int header_with_bit_depth, int16_t pos_x6y9, int a4)
{
    ffnx_trace("%s\n", __func__);

    return ((int(*)(int, int, int16_t, int))0x461A20)(a1, header_with_bit_depth, pos_x6y9, a4);
}

int ff8_replace_call3(int a1, int header_with_bit_depth, int16_t pos_x6y9, int a4)
{
    ffnx_trace("%s\n", __func__);

    return ((int(*)(int, int, int16_t, int))0x461A20)(a1, header_with_bit_depth, pos_x6y9, a4);
}

int ff8_replace_call4(int a1, int header_with_bit_depth, int16_t pos_x6y9, int a4)
{
    ffnx_trace("%s\n", __func__);

    return ((int(*)(int, int, int16_t, int))0x461A20)(a1, header_with_bit_depth, pos_x6y9, a4);
}

void ff8_ssigpu_set_t_page(int header_with_bit_depth)
{
    DWORD *header_copy = ((DWORD *)0x1C484F0); // full header
    DWORD *flags = ((DWORD *)0x1CA855C); // Flags
    DWORD *y = ((DWORD *)0x1B474DC); // Y (unused)
    DWORD *x = ((DWORD *)0x1B474D8); // X (unused)
    DWORD *bit_depth = ((DWORD *)0x1B474E4);
    DWORD *size = ((DWORD *)0x1B474EC); // Size (unused)

    if (*header_copy != header_with_bit_depth)
    {
        *header_copy = header_with_bit_depth;
        *flags = (header_with_bit_depth >> 5) & 3;
        *y = 16 * (header_with_bit_depth & 0x10);
        *x = (header_with_bit_depth & 0xF) << 6;
        *bit_depth = (header_with_bit_depth >> 7) & 3;
        *size = (*x / 64) | (16 * (*y / 256));

        *psxvram_current_texture_ptr = vram_seek(*x, *y);

        ffnx_trace("%s %d flags=%d y=%d x=%d bit_depth=%d size=%d\n",
            __func__,
            header_with_bit_depth,
            *flags,
            *y,
            *x,
            *bit_depth,
            *size
        );

        if (((header_with_bit_depth >> 7) & 3) == 3)
        {
            ffnx_warning("%s: bogus texture page: wrong bit depth\n", __func__);

            *bit_depth = 1;
        }
    }
}

void ff8_vram_point_to_texture(int16_t pos_x6y10)
{
    ffnx_trace("%s pos_x6y10=0x%X x=%d y=%d\n", __func__, pos_x6y10, 16 * (pos_x6y10 & 0x3F), (pos_x6y10 & 0x7FFF) >> 6);

    DWORD *vram_pos_xy = ((DWORD *)0x1CA8690);

    *vram_pos_xy = pos_x6y10 & 0x7FFF;
    *psxvram_current_clut_ptr = vram_seek(16 * (pos_x6y10 & 0x3F), *vram_pos_xy >> 6);
}

// Not used?
void ff8_copy_vram2(uint16_t* param)
{
    int vram_x = param[4] & 0x3FF;
    int vram_y = param[5] & 0x1FF;
    int x = param[6] & 0x3FF;
    int y = param[7] & 0x1FF;
    int w = param[8];
    int h = param[9];

    ffnx_trace("%s vram_x=%d vram_y=%d x=%d y=%d w=%d h=%d\n", __func__,
        vram_x, vram_y, x, y, w, h);

    if (vram_x + w > 1024)
    {
        w = 1024 - vram_x;
    }

    if (x + w > 1024)
    {
        w = 1024 - x;
    }

    if (vram_y + h > 512)
    {
        h = 512 - vram_y;
    }

    if (y + h > 512)
    {
        h = 512 - y;
    }

    texturePacker.copyRect(vram_x, vram_y, x, y, w, h);

    ff8_externals.sub_464850(x, y, x + w - 1, h + y - 1);
}

bool first_toto = false;

int ff8_upload_vram(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x=%d y=%d w=%d h=%d buffer=0x%X\n", __func__, x, y, w, h, texture_buffer);

    texturePacker.setTexture(nextTextureName, texture_buffer, x, y, w, h);

    ff8_externals.sub_464850(x, y, x + w - 1, h + y - 1);

    /* if (! first_toto) {
        if (nextTextureName.empty()) {
            nextTextureName = "vram.png";
        } else {
            nextTextureName += ".vram.png";
        }
        texturePacker.saveVram(nextTextureName.c_str());
        first_toto = true;
    } */

    *nextTextureName = '\0';

    return 1;
}

void ff8_copy_vram(int16_t* pos_and_size, int x, int y)
{
    const int vram_x = pos_and_size[0];
    const int vram_y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s vram_x=%d vram_y=%d w=%d h=%d, x=%d y=%d\n", __func__, vram_x, vram_y, w, h, x, y);

    texturePacker.copyRect(vram_x, vram_y, x, y, w, h);

    ff8_externals.sub_464850(x, y, w + x - 1, h + y - 1);
}

void ff8_download_vram_to_buffer(int16_t* pos_and_size, uint8_t* target_buffer)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x=%d y=%d w=%d h=%d buffer=0x%X\n", __func__, x, y, w, h, target_buffer);

    texturePacker.getRect(target_buffer, x, y, w, h);
}

int ff8_fill_vram(int16_t* pos_and_size, unsigned char b, unsigned char g, unsigned char r)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x=%d y=%d w=%d h=%d rgb=(%X, %X, %X)\n", __func__, x, y, w, h, r, g, b);

    DWORD *ff8_psxvram_buffer_was_initialized = ((DWORD *)0x1B474E8);

    if (! *ff8_psxvram_buffer_was_initialized)
    {
        ((void(*)(char*))0x464210)(psxvram_buffer);
        *ff8_psxvram_buffer_was_initialized = 1;
    }

    texturePacker.fillRect(x, y, w, h, r, g, b);

    ff8_externals.sub_464850(x, y, w + x - 1, h + y - 1);

    return 1;
}

// Not used? Maybe wm related
void ff8_fill_vram2(struc_color_texture *color_texture)
{
    ffnx_trace("%s x=%d, y=%d, w=%d, h=%d\n", __func__,
        color_texture->x, color_texture->y, color_texture->w, color_texture->h);

    int16_t x = color_texture->x,
            y = color_texture->y,
            w = color_texture->w,
            h = color_texture->h;

    if (x >= 640 && y >= 240 && x + w <= VRAM_WIDTH && y + h <= VRAM_HEIGHT)
    {
        texturePacker.fillRect(x, y, w, h, color_texture->r, color_texture->g, color_texture->b);
    }
}

void ff8_fill_vram3(struc_color_texture *color_texture)
{
    ffnx_trace("%s x=%d, y=%d, w=%d, h=%d\n", __func__,
        color_texture->x, color_texture->y, color_texture->w, color_texture->h);

    int16_t x = color_texture->x,
            y = color_texture->y,
            w = color_texture->w,
            h = color_texture->h;

    if (x >= 0 && y >= 0 && x + w <= VRAM_WIDTH && y + h <= VRAM_HEIGHT)
    {
        texturePacker.fillRect(x, y, w, h, color_texture->r, color_texture->g, color_texture->b);
    }
}

void ff8_vram_op_set16(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH * VRAM_DEPTH * 2;
    int h = h2 * VRAM_WIDTH * VRAM_DEPTH * 2;

    for (; cur <= max; ++cur)
    {
        uint16_t color = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * (
                (*(
                    *psxvram_current_texture_ptr + (
                        (x2 + (y & 0x3FC00000)) / (VRAM_WIDTH * 2)
                    )
                ) >> (x2 & 4)) & 0xF
            )
        );

        if (color != 0)
        {
            *cur = color;
        }

        x2 += w2;
        y += h;
    }
}

void ff8_vram_op_add16(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH * VRAM_DEPTH * 2;
    int h = h2 * VRAM_WIDTH * VRAM_DEPTH * 2;

    for (; cur < max; ++cur)
    {
        uint16_t color = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * ((*(
                *psxvram_current_texture_ptr + (
                    (x2 + (y & 0x3FC00000)) / (VRAM_WIDTH * 2)
                )
            ) >> (x2 & 4)) & 0xF)
        );

        if (color != 0)
        {
            *cur = color_add(color, *cur);
        }

        x2 += w2;
        y += h;
    }
}

void ff8_vram_op_sub16(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH * VRAM_DEPTH * 2;
    int h = h2 * VRAM_WIDTH * VRAM_DEPTH * 2;

    for (; cur <= max; ++cur)
    {
        uint16_t color = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * ((*(uint8_t *)(
                *psxvram_current_texture_ptr + (
                    (x2 + (y & 0x3FC00000)) / (VRAM_WIDTH * 2)
                )
            ) >> (x2 & 4)) & 0xF)
        );

        if (color)
        {
            *cur = color_sub(*cur, color);
        }

        x2 += w2;
        y += h;
    }
}

void ff8_vram_op_set32(uint32_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint32_t *max = pointer + VRAM_DEPTH * x1;
    uint32_t *cur = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH * VRAM_DEPTH;
    int h = h2 * VRAM_WIDTH * VRAM_DEPTH;

    if ((uint32_t(cur) & 2) != 0) // Not multiple of 4
    {
        uint16_t color = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * *(uint8_t *)(
                *psxvram_current_texture_ptr + ((x2 + (y & 0x1FE00000)) / VRAM_WIDTH)
            )
        );

        x2 += w2;
        y += h;

        if (color != 0)
        {
            *(uint16_t *)cur = color;
            cur = (uint32_t *)(((uint16_t *)cur) + 1);
        }
    }

    for (; cur < (uint32_t *)(uint32_t(max) & 0xFFFFFFFC); ++cur)
    {
        uint16_t color1 = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * *(uint8_t *)(
                *psxvram_current_texture_ptr + ((x2 + (y & 0x1FE00000)) / VRAM_WIDTH)
            )
        );

        x2 += w2;
        y += h;

        int color2 = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * *(uint8_t *)(
                *psxvram_current_texture_ptr + ((x2 + (y & 0x1FE00000)) / VRAM_WIDTH)
            )
        );

        if ((uint16_t(color2) & color1) != 0)
        {
            *cur = color1 | (color2 << 16);
        }
        else
        {
            if (color1 != 0)
            {
                *(int16_t *)cur = color1;
            }

            if (color2 != 0)
            {
                *(((int16_t *)cur) + 1) = color2;
            }
        }

        x2 += w2;
        y += h;
    }

    if (cur < max)
    {
        uint16_t color = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * *(uint8_t *)(
                *psxvram_current_texture_ptr + ((x2 + (y & 0x1FE00000)) / VRAM_WIDTH)
            )
        );

        if (color != 0)
        {
            *(uint16_t *)cur = color;
        }
    }
}

void ff8_vram_op_add16b(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH * VRAM_DEPTH;
    int h = h2 * VRAM_WIDTH * VRAM_DEPTH;

    for (; cur < max; ++cur)
    {
        uint16_t color = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * *(uint8_t *)(
                *psxvram_current_texture_ptr + (
                    (x2 + (y & 0x1FE00000)) / VRAM_WIDTH
                )
            )
        );

        if (color != 0)
        {
            *cur = color_add(color, *cur);
        }

        x2 += w2;
        y += h;
    }
}

void ff8_vram_op_sub16b(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH * VRAM_DEPTH;
    int h = h2 * VRAM_WIDTH * VRAM_DEPTH;

    for (; cur <= max; ++cur)
    {
        uint16_t color = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * *(uint8_t *)(
                *psxvram_current_texture_ptr + (
                    (x2 + (y & 0x1FE00000)) / VRAM_WIDTH
                )
            )
        );

        if (color != 0)
        {
            *cur = color_sub(*cur, color);
        }

        x2 += w2;
        y += h;
    }
}

void ff8_vram_op_set_indexed8(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
/*
arg_0= dword ptr  4
arg_4= dword ptr  8
arg_8= dword ptr  0Ch
arg_C= dword ptr  10h
arg_10= dword ptr  14h
arg_14= dword ptr  18h
arg_18= dword ptr  1Ch

mov     eax, [esp+arg_0]
mov     ecx, [esp+arg_8]
mov     edx, [esp+arg_4]
push    ebp
mov     ebp, psxvram_buffer_pointer_page_dword_1CA86A8
push    esi
mov     esi, [esp+8+arg_18]
push    edi
lea     edi, [eax+ecx*2]
mov     ecx, [esp+0Ch+arg_14]
lea     eax, [eax+edx*2]
shl     ecx, 0Ah
shl     esi, 0Ah
cmp     eax, edi
mov     [esp+0Ch+arg_18], esi
ja      short loc_4639D5

mov     esi, [esp+0Ch+arg_C]
push    ebx

loc_4639A6:
mov     edx, ecx
xor     ebx, ebx
and     edx, 0FF00000h
add     edx, esi
sar     edx, 0Ah
mov     bl, [edx+ebp]
mov     edx, ebx
test    edx, edx
jz      short loc_4639C1

mov     [eax], dx

loc_4639C1:
mov     edx, [esp+10h+arg_10]
mov     ebx, [esp+10h+arg_18]
add     eax, 2
add     esi, edx
add     ecx, ebx
cmp     eax, edi
jbe     short loc_4639A6

pop     ebx

loc_4639D5:
pop     edi
pop     esi
pop     ebp
retn
*/
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH;
    int h = h2 * VRAM_WIDTH;

    for (; cur <= max; ++cur)
    {
        uint8_t color = *(uint8_t *)(
            *psxvram_current_texture_ptr + (
                (x2 + (y & 0xFF00000)) / VRAM_WIDTH
            )
        );

        if (color != 0)
        {
            *cur = uint16_t(color);
        }

        x2 += w2;
        y += h;
    }
}

void ff8_vram_op_add_indexed8(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH;
    int h = h2 * VRAM_WIDTH;

    for (; cur < max; ++cur)
    {
        uint8_t color = *(uint8_t *)(
            *psxvram_current_texture_ptr + (
                (x2 + (y & 0xFF00000)) / VRAM_WIDTH
            )
        );

        if (color != 0)
        {
            *cur = color_add(uint16_t(color), *cur);
        }

        x2 += w2;
        y += h;
    }
}

void ff8_vram_op_sub_indexed8(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH;
    int h = h2 * VRAM_WIDTH;

    for (; cur <= max; ++cur)
    {
        uint8_t color = *(uint8_t *)(
            *psxvram_current_texture_ptr + (
                (x2 + (y & 0xFF00000)) / VRAM_WIDTH
            )
        );

        if (color != 0)
        {
            *cur = color_sub(*cur, uint16_t(color));
        }

        x2 += w2;
        y += h;
    }
}

void ff8_vram_op_related(int y, int h)
{
    ffnx_trace("%s y=%d h=%d\n", __func__, y, h);

    uint16_t *psxvram_buffer_pointer = (uint16_t *)vram_seek(0, y);
    int *v5 = (int *)0x1CA9F88;
    int *v6 = (int *)0x1CA9FB0;
    uint16_t *dword_1CA9FA8 = (uint16_t *)0x1CA9FA8;
    int *dword_1CACEE0 = (int *)0x1CACEE0;
    int *dword_1CA9FD8 = (int *)0x1CA9FD8;

    for (int i = 0; i < h; ++i)
    {
        if (*v5 > *v6)
        {
            int *v7 = v5;
            v5 = v6;
            v6 = v7;
        }

        int v8 = *v5 >> 16;
        size_t size = (*v6 >> 16) - v8;

        std::fill_n(psxvram_buffer_pointer + 2 * v8, 4 * size, *dword_1CA9FA8);

        psxvram_buffer_pointer += VRAM_WIDTH;
        *v5 += *dword_1CACEE0;
        *v6 += *dword_1CA9FD8;
    }
}

// bit_depth = 0 -> size = 16
// bit_depth = 1 -> size = 256
void ff8_gpu_read_palette(int clut, uint8_t rgba[4], int size)
{
    const int x = 16 * (clut & 0x3F);
    const int y = (uint16_t(clut) >> 6) & 0x1FF;

    ffnx_trace("%s x=%d y=%d size=%d (bit depth=%d)\n", __func__, x, y, size, size == 16 ? 0 : (size == 256 ? 1 : 2));

    texturePacker.getColors(rgba, x, y, size, TexturePacker::FormatR8G8B8A8);
}

void ff8_gpu_read_palette_alpha(int clut, uint8_t rgba[4], int size)
{
    const int x = 16 * (clut & 0x3F);
    const int y = (uint16_t(clut) >> 6) & 0x1FF;
    int is_alpha_shift = *((int *)0x1CCFA58);
    int shift_value = *((int *)0x1CA86C0);

    ffnx_trace("%s x=%d y=%d size=%d is_alpha_shift=%d shift_value=%d\n", __func__, x, y, size, is_alpha_shift, shift_value);

    TexturePacker::ColorFormat format = is_alpha_shift ? TexturePacker::FormatAlphaShift : TexturePacker::FormatAlphaHeightHack;
    texturePacker.getColors(rgba, x, y, size, format, shift_value);
}

int next_scale = 1;

int op_on_psxvram_sub_4675B0_parent_call1(int a1, int structure, int x, int y, int w, int h, int bpp, int rel_pos, int a9, uint8_t *target)
{
    ffnx_trace("%s: x=%d y=%d w=%d h=%d bpp=%d rel_pos=(%d, %d) a9=%d target=%X\n", __func__, x, y, w, h, bpp, rel_pos & 0xF, rel_pos >> 4, a9, target);

    next_psxvram_x = (x >> (2 - bpp)) + ((rel_pos & 0xF) << 6);
    next_psxvram_y = y + (((rel_pos >> 4) & 1) << 8);

    next_scale = texturePacker.getCurrentScale();

    if (next_scale > 1) {
        w *= next_scale;
        h *= next_scale;
    }

    int ret = ((int(*)(int, int, int, int, int, int, int, int, int, uint8_t *))0x464F60)(a1, structure, x, y, w, h, bpp, rel_pos, a9, target);

    next_psxvram_x = -1;
    next_psxvram_y = -1;

    next_scale = 1;

    return ret;
}

int op_on_psxvram_sub_4675B0_parent_call2(texture_page *tex_page, int rel_pos, int a3)
{
    ffnx_trace("%s: x=%d y=%d color_key=%d rel_pos=(%d, %d)\n", __func__, tex_page->x, tex_page->y, tex_page->color_key, rel_pos & 0xF, rel_pos >> 4);

    next_psxvram_x = (tex_page->x >> (2 - tex_page->color_key)) + ((rel_pos & 0xF) << 6);
    next_psxvram_y = tex_page->y + (((rel_pos >> 4) & 1) << 8);

    next_scale = texturePacker.getCurrentScale();

    int ret = ((int(*)(texture_page *, int, int))0x4653A0)(tex_page, rel_pos, a3);

    next_psxvram_x = -1;
    next_psxvram_y = -1;

    next_scale = 1;

    return ret;
}

bool is_foo = true;

void read_vram_to_buffer1(uint8_t *vram, int vram_w_2048, uint8_t *target, int target_w, signed int w, int h, int bpp)
{
    int dword_B7DB44 = *((int *)0xB7DB44);
    int vram_start = int(psxvram_buffer);
    int vram_y = (int(vram) - vram_start) / vram_w_2048;
    int vram_x = ((int(vram) - vram_start) % vram_w_2048) / 2;
    ffnx_trace("%s: vram=0x%X (x=%d, y=%d) vram_w=%d target=%X target_w=%d w=%d h=%d bpp=%d dword_B7DB44=%d\n", __func__, vram, vram_x, vram_y, vram_w_2048, target, target_w, w, h, bpp, dword_B7DB44);

    if (next_psxvram_x == -1) {
        ffnx_error("%s: cannot detect VRAM position\n", __func__);

        return;
    }

    TexturePacker::ColorFormat color_format;

    if (bpp == 2 && dword_B7DB44 > 0)
    {
        color_format = TexturePacker::FormatR5G5B5Hack;
    }
    else
    {
        color_format = bpp == 2 ? TexturePacker::FormatR5G5B5 : TexturePacker::Format8Bit;
    }

    // FIXME: cat we ignore target_w? it is taken from tex header and should be always w * depth or something like this
    texturePacker.getRect(target, next_psxvram_x, next_psxvram_y, w / next_scale, h / next_scale, color_format, next_scale);

    is_foo = false;
}

void read_vram_to_buffer_with_palette(uint8_t *vram, int vram_w_2048, uint8_t *target, int target_w, int w, int h, int bpp, uint8_t *vram_palette)
{
    ffnx_trace("%s\n", __func__);
    /*
    int *color_shift = (int *)0x1CA86C0;

    char color_shift2 = *color_shift + 3;

    if (bpp == 1)
    {
        for (int i = 0; i < h; ++i)
        {
            uint8_t *v12 = vram;
            uint8_t *v13 = target;

            for (int j = 0; j < w; ++j)
            {
                uint16_t v14 = *(int16_t *)(vram_palette + 2 * *v12);
                uint16_t v15 = ((v14 & 0x1F) + (((int)v14 >> 5) & 0x1F) + (((int)v14 >> 10) & 0x1F)) >> color_shift2;
                *v13 = (v15 > 15 ? 15 : v15) << 12;

                ++v12;
                ++v13;
            }

            target += target_w;
            vram += vram_w_2048;
        }
    }
    else if (bpp == 2)
    {
        for (int i = 0; i < h; ++i)
        {
            uint16_t *vram16 = (uint16_t *)vram;

            for (int j = 0; j < w; ++j)
            {
                uint16_t color = ((*vram16 & 0x1F) + ((*vram16 >> 5) & 0x1F) + ((*vram16 >> 10) & 0x1F)) >> color_shift2;
                vram16[target - vram] = (color > 15 ? 15 : color) << 12;
                vram16++;
            }

            target += target_w;
            vram += vram_w_2048;
        }
    }
    else if (bpp == 0)
    {
        int v22 = vram_w_2048 - w / 2;

        for (int i = 0; i < h; ++i)
        {
            for (int j = 0; j < w / 2; ++j)
            {
                unsigned int v17 = *vram;
                int v18 = ((*(int16_t *)(vram_palette + 2 * (v17 & 0xF)) & 0x1F)
                    + (((int)*(uint16_t *)(vram_palette + 2 * (v17 & 0xF)) >> 5) & 0x1F)
                    + (((int)*(uint16_t *)(vram_palette + 2 * (v17 & 0xF)) >> 10) & 0x1F)) >> color_shift2;
                if (v18 > 15) {
                    LOWORD(v18) = 15;
                }
                uint16_t v19 = *(int16_t *)(vram_palette + 2 * (v17 >> 4));
                *target = (int16_t)v18 << 12;
                int v20 = ((v19 & 0x1F) + (((int)v19 >> 5) & 0x1F) + (((int)v19 >> 10) & 0x1F)) >> color_shift2;
                if (v20 > 15) {
                    LOWORD(v20) = 15;
                }
                target[1] = (int16_t)v20 << 12;
                target += 4;
                ++vram;
            }

            vram += v22;
        }
    } */
}

void read_vram_to_buffer3(int8_t *vram_buffer_point, int8_t *target_buffer, int w, int h, int bpp, int8_t *vram_palette)
{
    ffnx_trace("%s\n", __func__);
    /*
    int w_divided_by_2 = w / 2;

    if (bpp == 1)
    {
        for (int i = 0; i < h; ++i)
        {
            if (w > 2)
            {
                for (int j = 0; j < w_divided_by_2; ++j)
                {
                    int16_t v12 = *(int16_t *)(vram_palette + 2 * (uint8_t)*vram_buffer_point);
                    int v13 = *(uint16_t *)(vram_palette + 2 * (uint8_t)*(vram_buffer_point + 1)) << 16;
                    *(int32_t *)target_buffer = v12 & 0x3E0 | uint(v13 & 0x3E00000 | ((v12 & 0x1F | v13 & 0x1F0000) << 10) | ((v12 & 0x7C00 | v13 & 0x7C000000u) >> 10));

                    target_buffer += 4;
                    vram_buffer_point += 2;
                }
            }

            if ((w & 1) != 0)
            {
                uint16_t v15 = *(int16_t *)(vram_palette + 2 * (uint8_t)*vram_buffer_point);
                *(int16_t *)target_buffer = v15 & 0x3E0 | ((v15 & 0x1F) << 10) | (v15 >> 10) & 0x1F;

                target_buffer += 2;
                vram_buffer_point += 1;
            }

            vram_buffer_point += 2048 - w;
            a2 += 2 * (256 - w);
        }
    }
    else if (bpp == 0)
    {
        for (int i = 0; i < h; ++i)
        {
            for (int j = 0; j < w_divided_by_2; ++j)
            {
                int v17 = *(uint16_t *)(vram_palette + 2 * ((uint8_t)*vram_buffer_point >> 4)) << 16;
                *(int32_t *)target_buffer = int(*(int16_t *)(vram_palette + 2 * (*vram_buffer_point & 0xF)) & 0x3E0)
                    | v17 & 0x3E00000
                    | int((*(int16_t *)(vram_palette + 2 * (*vram_buffer_point & 0xF)) & 0x1F | v17 & 0x1F0000) << 10)
                    | (uint(*(int16_t *)(vram_palette + 2 * (*vram_buffer_point & 0xF)) & 0x7C00 | v17 & 0x7C000000) >> 10);

                target_buffer += 4;
                vram_buffer_point += 1;
            }

            vram_buffer_point += 2048 - w_divided_by_2;
            target_buffer += 2 * (256 - w);
        }
    }*/
}

uint32_t ff8_credits_open_texture(char *fileName, char *buffer)
{
    ffnx_trace("%s: %s\n", __func__, fileName);

    // {name}.lzs
    strncpy(nextTextureName, strrchr(fileName, '\\') + 1, MAX_PATH);

    return ((uint32_t(*)(char*,char*))0x52CED0)(fileName, buffer);
}

void vram_init()
{
    ffnx_info("%s: sizeof(struc_50) = %d\n", __func__, sizeof(struc_50));

    replace_call(0x52F9D2, ff8_credits_open_texture);
    /* replace_call(0x462B22, ff8_replace_call0);
    replace_call(0x461802, ff8_replace_call1);
    replace_call(0x461AA2, ff8_replace_call2);
    replace_call(0x461E10, ff8_replace_call3);
    replace_call(0x462042, ff8_replace_call4); */
    /* replace_call(0x45B460 + 0x26, ff8_intercept_store_vram); */
    // replace_function(0x45B910, ff8_vram_point_to_texture);
    // replace_function(0x45B950, ff8_ssigpu_set_t_page);
    replace_function(0x45BAF0, ff8_copy_vram2);
    replace_function(0x45BD20, ff8_upload_vram);
    replace_function(0x45BDC0, ff8_copy_vram);
    replace_function(0x45BE60, ff8_download_vram_to_buffer);
    replace_function(0x45BED0, ff8_fill_vram);
    replace_function(0x462390, ff8_fill_vram2);
    replace_function(0x466EE0, ff8_fill_vram3);

    // Operations
    /* replace_function(0x4633B0, ff8_vram_op_set16);
    replace_function(0x463440, ff8_vram_op_add16);
    replace_function(0x463540, ff8_vram_op_sub16);
    replace_function(0x463640, ff8_vram_op_set32);
    replace_function(0x463790, ff8_vram_op_add16b);
    replace_function(0x463880, ff8_vram_op_sub16b);
    replace_function(0x463970, ff8_vram_op_set_indexed8);
    replace_function(0x4639E0, ff8_vram_op_add_indexed8);
    replace_function(0x463AC0, ff8_vram_op_sub_indexed8);

    replace_function(0x463D40, ff8_vram_op_related); */

    // GPU read
    replace_function(0x467360, ff8_gpu_read_palette);
    replace_function(0x467460, ff8_gpu_read_palette_alpha);

    replace_call(0x464C13, op_on_psxvram_sub_4675B0_parent_call1);
    replace_call(0x464CAD, op_on_psxvram_sub_4675B0_parent_call1);
    replace_call(0x464D61, op_on_psxvram_sub_4675B0_parent_call1);
    replace_call(0x465F51, op_on_psxvram_sub_4675B0_parent_call1);

    replace_call(0x464C39, op_on_psxvram_sub_4675B0_parent_call2);
    replace_call(0x464D75, op_on_psxvram_sub_4675B0_parent_call2);

    replace_function(0x4675B0, read_vram_to_buffer1);
    replace_function(0x4677C0, read_vram_to_buffer_with_palette);
    replace_function(0x467A00, read_vram_to_buffer3);
}
