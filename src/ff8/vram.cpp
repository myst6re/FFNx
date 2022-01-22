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

#define VRAM_DEPTH 2
#define VRAM_WIDTH 1024

#define VRAM_SEEK(pointer, x, y) \
    (pointer + VRAM_DEPTH * (x + y * VRAM_WIDTH))

char *psxvram_buffer = (char *)0x1B474F0;
char **psxvram_current_clut_ptr = ((char **)0x1CA8680);
char **psxvram_current_texture_ptr = ((char **)0x1CA86A8);

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

void ff8_intercept_store_vram(int *vram_buffer)
{
    ffnx_trace("%s 0x%X\n", __func__, vram_buffer);

    ((void(*)(int *))0x464210)(vram_buffer);

    // vram_buffer_point_dword_1CB5D04 = (int)vram_buffer;
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

// Used
void ff8_vram_point_to_texture(int16_t pos_x6y10)
{
    ffnx_trace("%s pos_x6y10=0x%X x=%d y=%d\n", __func__, pos_x6y10, 16 * (pos_x6y10 & 0x3F), (pos_x6y10 & 0x7FFF) >> 6);

    DWORD *vram_pos_xy = ((DWORD *)0x1CA8690);

    *vram_pos_xy = pos_x6y10 & 0x7FFF;
    *psxvram_current_clut_ptr = vram_seek(16 * (pos_x6y10 & 0x3F), *vram_pos_xy >> 6);
}

// Not used?
void ff8_download_vram(DWORD* param)
{
    ffnx_trace("%s %X\n", __func__, param);

    int v1; // edi
    int v2; // eax
    int v3; // esi
    int v4; // edx
    int v5; // ebx
    int v6; // ebp
    char *psxvram_buffer_pointer; // edi
    char *psxvram_buffer_pointer_copy; // [esp+10h] [ebp-Ch]
    int v11; // [esp+14h] [ebp-8h]
    int v12; // [esp+18h] [ebp-4h]
    int v13; // [esp+20h] [ebp+4h]
    char *dest_pointer; // [esp+20h] [ebp+4h]

    v1 = HIWORD(a1[2]) & 0x1FF;
    v2 = HIWORD(a1[3]) & 0x1FF;
    v3 = a1[2] & 0x3FF;
    v4 = (uint16_t)a1[4];
    v5 = a1[3] & 0x3FF;
    v6 = HIWORD(a1[4]);
    v13 = v1;
    v12 = v2;
    if ( v4 + v3 > 1024 )
        v4 = 1024 - v3;
    if ( v4 + v5 > 1024 )
        v4 = 1024 - v5;
    if ( v1 + v6 > 512 )
        v6 = 512 - v1;
    if ( v2 + v6 > 512 )
        v6 = 512 - v2;

    psxvram_buffer_pointer = (char *)(2 * (v3 + (v1 << 10)) + 28603632);// psxvram_buffer
    psxvram_buffer_pointer_copy = psxvram_buffer_pointer;

    dest_pointer = &psxvram_buffer_pointer[2 * v5 + 2 * (((v2 - v13) << 10) - v3)];

    for (int i = 0; i < v6; ++i)
    {
        qmemcpy(dest_pointer, psxvram_buffer_pointer, 2 * v4);
        psxvram_buffer_pointer = psxvram_buffer_pointer_copy + 2048;

        psxvram_buffer_pointer_copy += 2048;
        dest_pointer += 2048;
    }

    v2 = v12;

    ((void(*)(uint32_t, uint32_t, uint32_t, uint32_t))0x464840)(x, y, x + w - 1, h + y - 1);
}

int ff8_upload_vram(int16_t *pos_and_size, char *texture_buffer)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x=%d y=%d w=%d h=%d buffer=0x%X\n", __func__, x, y, w, h, texture_buffer);

    char *psxvram_pointer = vram_seek(x, y);

    for (int i = 0; i < h; ++i)
    {
        memcpy(psxvram_pointer, texture_buffer, VRAM_DEPTH * w);

        texture_buffer += VRAM_DEPTH * w;
        psxvram_pointer += VRAM_WIDTH * VRAM_DEPTH;
    }

    ((void(*)(uint32_t, uint32_t, uint32_t, uint32_t))0x464840)(x, y, x + w - 1, h + y - 1);

    return 1;
}

// Used
int ff8_copy_vram(int16_t* pos_and_size, int x, int y)
{
    const int x2 = pos_and_size[0];
    const int y2 = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x2=%d y2=%d w=%d h=%d, x=%d y=%d\n", __func__, x2, y2, w, h, x, y);

    if (h != 0)
    {
        const int w_mult_by_2 = VRAM_DEPTH * w;
        char *psxvram_pointer = vram_seek(x, y);
        char *target_buffer_pos = VRAM_SEEK((char *)psxvram_pointer, x - x2, y - y2);

        for (int i = 0; i < h; ++i)
        {
            memcpy(target_buffer_pos, psxvram_pointer, w_mult_by_2);

            target_buffer_pos += VRAM_WIDTH * VRAM_DEPTH;
            psxvram_pointer += VRAM_WIDTH * VRAM_DEPTH;
        }
    }

    ((void(*)(uint32_t, uint32_t, uint32_t, uint32_t))0x464840)(x, y, w + x - 1, h + y - 1);

    return 1;
}

int ff8_download_vram_to_buffer(int16_t* pos_and_size, char* target_buffer)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x=%d y=%d w=%d h=%d buffer=0x%X\n", __func__, x, y, w, h, target_buffer);

    if (h != 0)
    {
        const int w_mult_by_2 = VRAM_DEPTH * w;
        char *psxvram_pointer = vram_seek(x, y);

        for (int i = 0; i < h; ++i)
        {
            memcpy(target_buffer, psxvram_pointer, w_mult_by_2);

            target_buffer += w_mult_by_2;
            psxvram_pointer += VRAM_WIDTH * VRAM_DEPTH;
        }
    }

    return 1;
}
int ff8_fill_vram(int16_t* pos_and_size, unsigned char r, unsigned char g, unsigned char b)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];
    const uint16_t color = (b >> 3) | (32 * ((g >> 3) | (32 * (r >> 3))));

    ffnx_trace("%s x=%d y=%d w=%d h=%d rgb=(%X, %X, %X)\n", __func__, x, y, w, h, r, g, b);

    DWORD *ff8_psxvram_buffer_was_initialized = ((DWORD *)0x1B474E8);

    if (! *ff8_psxvram_buffer_was_initialized)
    {
        ((void(*)(char*))0x464210)(psxvram_buffer);
        *ff8_psxvram_buffer_was_initialized = 1;
    }

    uint16_t *psxvram_buffer_pointer = (uint16_t *)vram_seek(x, y);

    if (h != 0)
    {
        for (int i = 0; i < h; ++i)
        {
            if (w != 0)
            {
                std::fill_n(psxvram_buffer_pointer, w, color);
            }

            psxvram_buffer_pointer += VRAM_WIDTH;
        }
    }

    ((void(*)(uint32_t, uint32_t, uint32_t, uint32_t))0x464840)(x, y, w + x - 1, h + y - 1);

    return 1;
}

// Not used? Maybe wm related
void ff8_test1(int a1)
{
    ffnx_trace("%s %d\n", __func__, a1);
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
        int8_t color = *(int8_t *)(
            *psxvram_current_texture_ptr + (
                (x2 + (y & 0xFF00000)) / VRAM_WIDTH
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

void ff8_vram_op_add_indexed8(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH;
    int h = h2 * VRAM_WIDTH;

    for (; cur < max; ++cur)
    {
        int16_t color = *(uint8_t *)(
            *psxvram_current_texture_ptr + (
                (x2 + (y & 0xFF00000)) / VRAM_WIDTH
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

void ff8_vram_op_sub_indexed8(uint16_t *pointer, int x1, int w1, int x2, int w2, int y2, int h2)
{
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    uint16_t *cur = pointer + VRAM_DEPTH * x1;
    uint16_t *max = pointer + VRAM_DEPTH * w1;
    int y = y2 * VRAM_WIDTH;
    int h = h2 * VRAM_WIDTH;

    for (; cur <= max; ++cur)
    {
        // FIXME: why int8_t cast???
        uint16_t color = *(int8_t *)(
            *psxvram_current_texture_ptr + (
                (x2 + (y & 0xFF00000)) / VRAM_WIDTH
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
/*
void ff8_test14(int y, int h)
{
    ffnx_trace("%s y=%d h=%d\n", __func__, y, h);

    uint16_t *psxvram_buffer_pointer = (uint16_t *)(psxvram_buffer + VRAM_DEPTH * VRAM_WIDTH * y);
    int *v5 = &dword_1CA9F88;
    int *v6 = &dword_1CA9FB0;

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

        if (size > 0)
        {
            std::fill_n(psxvram_buffer_pointer + 2 * v8, 4 * size, dword_1CA9FA8);
        }

        psxvram_buffer_pointer += VRAM_WIDTH;
        dword_1CA9F88 += dword_1CACEE0;
        dword_1CA9FB0 += dword_1CA9FD8;
    }
} */

void ff8_test15(int a1)
{
    ffnx_trace("%s %d\n", __func__, a1);

    int result = *(uint8_t *)(a1 + 4) >> 3;
    int v2 = ((result | (4 * (*(int8_t *)(a1 + 5) & 0xF8 | (32 * (*(int8_t *)(a1 + 6) & 0xF8))))) << 16) | result | (4 * (*(int8_t *)(a1 + 5) & 0xF8 | (32 * (*(int8_t *)(a1 + 6) & 0xF8))));
    int16_t v3 = *(int16_t *)(a1 + 8);

    if (v3 >= 0)
    {
        int16_t v4 = *(int16_t *)(a1 + 10);

        if (v4 >= 0)
        {
            int16_t v5 = *(int16_t *)(a1 + 12);

            if (v3 + v5 <= 1024 && v4 + *(int16_t *)(a1 + 14) <= 512)
            {
                int16_t *psxvram_buffer_pointer = (int16_t *)(2 * (v3 + (v4 << 10)) + 28603632);// psxvram_buffer
                int v10 = *(int16_t *)(a1 + 14);

                if (v10)
                {
                    int v7 = v5 / 2;
                    int16_t v8 = v5 & 1;

                    do
                    {
                        int16_t *psxvram_buffer_pointer_copy = psxvram_buffer_pointer;

                        if (v7)
                        {
                            memset32(psxvram_buffer_pointer, v2, v7);
                            psxvram_buffer_pointer_copy = &psxvram_buffer_pointer[2 * v7];
                        }
                        if (v8) {
                            *psxvram_buffer_pointer_copy = v2;
                        }

                        psxvram_buffer_pointer += 1024;
                        --v10;
                    }
                    while (v10);
                }
            }
        }
    }
}

void ff8_test16(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);

    unsigned int v7 = a2;
    int v8 = (a2 >> 7) & 3;
    int v9 = a2 & 0x1F;
    uint16_t v10 = a3 & 0x7FFF;
    int result = 1100 * (v9 + 32 * v8) + 30104856;

    if ( !dword_1CCFA68 || v8 == 2 )
    {
        dword_1CB603C[8800 * v8 + 275 * v9] |= 1u;

        if ( (a2 & 0x60) == 64 )
        {
            *((int32_t *)&unk_1CB6040 + 8800 * v8 + 275 * v9) |= 1u;
            *((int16_t *)&unk_1CB6160 + 17600 * ((a2 >> 7) & 3) + 550 * v9) = v10;
        }
    }
    else
    {
        char *v12 = (char *)&unk_1CACF00 + 1136 * v9;
        int32_t *v16 = v12;
        if ( !*((int32_t *)v12 + 283) )
        {
            *((int32_t *)v12 + 283) = ssigpu_garbage_mem_sub_467150(0x20000u);
        }
        int *v13 = (int *)&v12[64 * (a5 >> 4) + 100 + 4 * (a4 >> 4)];
        int v14 = v10 ^ HIWORD(*v13);
        *v13 = v14 | (v10 << 16);
        if ( v14 )
        {
            op_on_psxvram_sub_467A00(
                vram_buffer_point_dword_1CB5D04
                    + 2 * ((a4 >> (2 - v8)) + (((v7 & 0xF) + 16 * (a5 + (((v7 >> 4) & 1) << 8))) << 6)),
                v16[283] + 2 * (a4 + (a5 << 8)),
                a6,
                a7,
                v8,
                vram_buffer_point_dword_1CB5D04 + 32 * ((a3 & 0x3F) + ((uint16_t)(a3 & 0x7FFF) >> 6 << 6))
            );
            int v15 = v16[282];
            LOBYTE(v15) = v15 | 1;
            v16[282] = v15;
        }

        result = v16[281];
        LOBYTE(result) = result | 1;
        v16[281] = result;
    }

    return result;
}

void ff8_test17(int a1, int a2, int a3)
{
    ffnx_trace("%s %d %d %d\n", __func__, a1, a2, a3);
}

void ff8_test18(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10)
{
    ffnx_trace("%s %d %d %d %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

void ff8_test19(int a1, int a2, int a3)
{
    ffnx_trace("%s %d %d %d\n", __func__, a1, a2, a3);
}

void ff8_test20(int a1, int a2, int a3)
{
    ffnx_trace("%s %d %d %d\n", __func__, a1, a2, a3);
}

void ff8_test21(int a1, int a2, int a3)
{
    ffnx_trace("%s %d %d %d\n", __func__, a1, a2, a3);

    /* uint16_t *psxvram_buffer_pointer; // ebp
  unsigned int psxvram_buffer_pointer_copy; // ecx
  int v6; // edx

  psxvram_buffer_pointer = (uint16_t *)((char *)&psxvram_buffer + 2048 * ((a1 >> 6) & 0x1FF) + 32 * (a1 & 0x3F));
  if ( a3 <= 0 )
    return 1;
  do
  {
    psxvram_buffer_pointer_copy = *psxvram_buffer_pointer++;
    if ( (int16_t)psxvram_buffer_pointer_copy )
    {
      a2[3] = 127;
      if ( psxvram_buffer_pointer_copy == 0x8000 )
      {
        a2[2] = 0;
        a2[1] = 0;
        *a2 = 8;
      }
      else
      {
        a2[2] = 255 * (psxvram_buffer_pointer_copy & 0x1F) / 0x1F;
        v6 = (138547333 * (unsigned __int64)(255 * ((psxvram_buffer_pointer_copy >> 5) & 0x1F))) >> 32;
        a2[1] = (v6 + ((255 * ((psxvram_buffer_pointer_copy >> 5) & 0x1F) - v6) >> 1)) >> 4;
        *a2 = 255 * ((psxvram_buffer_pointer_copy >> 10) & 0x1F) / 0x1F;
      }
    }
    else
    {
      a2[3] = 0;
      a2[2] = 0;
      a2[1] = 0;
      *a2 = 1;
    }
    a2 += 4;
    --a3;
  }
  while ( a3 );
  return 1; */
}

void ff8_test22(int a1, int a2, int a3)
{
    ffnx_trace("%s %d %d %d\n", __func__, a1, a2, a3);
/*
    uint16_t *psxvram_buffer_pointer; // ebp
    uint16_t psxvram_buffer_pointer_copy; // cx
    unsigned int v6; // edi
    unsigned int v7; // esi
    unsigned int v8; // ecx
    unsigned int v9; // ebx
    char v10; // cl
    unsigned int v11; // ebx

    psxvram_buffer_pointer = (uint16_t *)((char *)&psxvram_buffer + 2048 * ((a1 >> 6) & 0x1FF) + 32 * (a1 & 0x3F));
    if ( a3 <= 0 )
        return 1;
    do
    {
        psxvram_buffer_pointer_copy = *psxvram_buffer_pointer++;
        v6 = psxvram_buffer_pointer_copy & 0x1F;
        v7 = (psxvram_buffer_pointer_copy >> 5) & 0x1F;
        v8 = (psxvram_buffer_pointer_copy >> 10) & 0x1F;
        if ( dword_1CCFA58 )
        {
        v9 = v6 + v8 + v7;
        v10 = 1 - dword_1CA86C0;
        a2[2] = 0;
        v11 = v9 << v10;
        a2[1] = 0;
        *a2 = 0;
        if ( v11 > 0xFF )
            LOBYTE(v11) = -1;
        a2[3] = v11;
        }
        else if ( v6 >= 8 || v7 >= 8 || v8 >= 8 )
        {
        a2[2] = 0;
        a2[1] = 0;
        *a2 = 8;
        a2[3] = -1;
        }
        else
        {
        a2[2] = 0;
        a2[1] = 0;
        *a2 = 0;
        a2[3] = 0;
        }
        a2 += 4;
        --a3;
    }
    while ( a3 ); */
}

int op_on_psxvram_sub_4675B0(int8_t *a1, int a2, char *a3, int a4, signed int a5, int a6, int a7)
{
    if (a7 == 1)
    {
        for (int i = 0; i < a6; ++i)
        {
            qmemcpy(a3, a1, 4 * ((unsigned int)a5 >> 2));
            qmemcpy(&a3[4 * ((unsigned int)a5 >> 2)], &a1[4 * ((unsigned int)a5 >> 2)], a5 & 3);
            a3 += a4;
            a1 += a2;
        }
    }
    else if (a7 == 2)
    {
        if (dword_B7DB44 <= 0)
        {
            for (int i = 0; i < a6; ++i)
            {
                unsigned int *v19 = (unsigned int *)a1;
                int *v20 = (int *)a3;

                for (int j = 0; j < a5 / 2; ++j)
                {
                    unsigned int v21 = *v19++;
                    *v20++ = v21 & 0x3E003E0 | ((v21 & 0x1F001F) << 10) | (v21 >> 10) & 0x1F001F;
                }

                if ( (a5 & 1) != 0 ) {
                    *(int16_t *)v20 = *(int16_t *)v19 & 0x3E0 | ((*(int16_t *)v19 & 0x1F) << 10) | (*(int16_t *)v19 >> 10) & 0x1F;
                }

                a1 += a2;
                a3 += a4;
            }
        }
        else if (a6 != 0)
        {
            for (int i = 0; i < a6; ++i)
            {
                unsigned int *v15 = (unsigned int *)a1;
                int16_t *v16 = a3;

                for (int j = 0; j < a5 / 2; ++j)
                {
                    unsigned int v17 = *v15++;
                    v16 += 2;
                    *((int32_t *)v16 - 1) = (v17 >> 10) & 0x1F001F | (2 * (v17 & 0x3E003E0 | ((v17 & 0xFFFF001F) << 10)));
                }

                if ( (a5 & 1) != 0 ) {
                    *v16 = (*(int16_t *)v15 >> 10) & 0x1F | (2 * ((*(int16_t *)v15 << 10) | *(int16_t *)v15 & 0x3E0));
                }

                a1 += a2;
                a3 += a4;
            }
        }
    }
    else if (a7 == 0)
    {
        for (int i = 0; i < a6; ++i)
        {
            for (int j = 0; j < a5 / 2; ++j)
            {
                a3 += 2;
                *(a3 - 2) = *a1 & 0xF;
                *(a3 - 1) = char(*a1 >> 4);
                ++a1;
            }
            a1 += a2 - a5 / 2;
        }
    }
}

int op_on_psxvram_sub_4677C0(uint8_t *a1, int a2, int16_t *a3, int a4, int a5, int a6, int bpp, int a8)
{
    char v9 = dword_1CA86C0 + 3;

    if (bpp == 1)
    {
        for (int i = 0; i < a6; ++i)
        {
            uint8_t *v12 = a1;
            uint8_t *v13 = a3;

            for (int j = 0; j < a5; ++j)
            {
                uint16_t v14 = *(int16_t *)(a8 + 2 * *v12);
                int v15 = ((v14 & 0x1F) + (((int)v14 >> 5) & 0x1F) + (((int)v14 >> 10) & 0x1F)) >> v9;

                if (v15 > 15)
                {
                    LOWORD(v15) = 15;
                }

                *v13 = (int16_t)v15 << 12;
                ++v12;
                ++v13;
            }

            a3 = (int16_t *)((char *)a3 + a4);
            a1 += a2;
        }
    }
    else if (bpp == 2)
    {
        uint8_t *v11 = a1;

        for (int i = 0; i < a6; ++i)
        {
            for (int j = 0; j < a5; ++j)
            {
                int v21 = ((*(int16_t *)v11 & 0x1F)
                    + (((int)*(uint16_t *)v11 >> 5) & 0x1F)
                    + (((int)*(uint16_t *)v11 >> 10) & 0x1F)) >> v9;
                if (v21 > 15) {
                    LOWORD(v21) = 15;
                }
                *(int16_t *)&v11[(char *)a3 - (char *)a1] = (int16_t)v21 << 12;
                v11 += 2;
            }

            v11 = &a1[a2];
            a3 = (int16_t *)((char *)a3 + a4);
            a1 += a2;
        }
    }
    else if (bpp == 0)
    {
        result = a5 / 2;
        int v22 = a2 - result;

        for (int i = 0; i < a6; ++i)
        {
            for (int j = 0; j < result; ++j)
            {
                unsigned int v17 = *a1;
                int v18 = ((*(int16_t *)(a8 + 2 * (v17 & 0xF)) & 0x1F)
                    + (((int)*(uint16_t *)(a8 + 2 * (v17 & 0xF)) >> 5) & 0x1F)
                    + (((int)*(uint16_t *)(a8 + 2 * (v17 & 0xF)) >> 10) & 0x1F)) >> v9;
                if (v18 > 15) {
                    LOWORD(v18) = 15;
                }
                uint16_t v19 = *(int16_t *)(a8 + 2 * (v17 >> 4));
                *a3 = (int16_t)v18 << 12;
                int v20 = ((v19 & 0x1F) + (((int)v19 >> 5) & 0x1F) + (((int)v19 >> 10) & 0x1F)) >> v9;
                if (v20 > 15) {
                    LOWORD(v20) = 15;
                }
                a3[1] = (int16_t)v20 << 12;
                a3 += 2;
                ++a1;
            }

            a1 += v22;
        }
    }

    return result;
}

void op_on_psxvram_sub_467A00(int8_t *vram_buffer_point, int8_t *target_buffer, int w, int h, int bpp, int8_t *vram_palette)
{
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
    }
}

void vram_init()
{
    /* replace_call(0x45B460 + 0x26, ff8_intercept_store_vram);
    replace_function(0x45B910, ff8_vram_point_to_texture);
    replace_function(0x45B950, ff8_ssigpu_set_t_page);
    replace_function(0x45BAF0, ff8_download_vram);
    replace_function(0x45BD20, ff8_upload_vram);
    replace_function(0x45BDC0, ff8_copy_vram);
    replace_function(0x45BE60, ff8_download_vram_to_buffer);
    replace_function(0x45BED0, ff8_fill_vram);
    replace_function(0x462390, ff8_test1); */

    // Operations
    replace_function(0x4633B0, ff8_vram_op_set16);
    replace_function(0x463440, ff8_vram_op_add16);
    replace_function(0x463540, ff8_vram_op_sub16);
    replace_function(0x463640, ff8_vram_op_set32);
    replace_function(0x463790, ff8_vram_op_add16b);
    replace_function(0x463880, ff8_vram_op_sub16b);
    replace_function(0x463970, ff8_vram_op_set_indexed8);
    replace_function(0x4639E0, ff8_vram_op_add_indexed8);
    replace_function(0x463AC0, ff8_vram_op_sub_indexed8);

    /* replace_function(0x463D40, ff8_test14);

    replace_function(0x464990, ff8_test16);
    replace_function(0x464DA0, ff8_test17); */
    //replace_function(0x464F60, ff8_test18);
    /* replace_function(0x4653A0, ff8_test19);
    replace_function(0x465710, ff8_test20);
    replace_function(0x466EE0, ff8_test15); */

    // GPU read
    /* replace_function(0x467360, ff8_test21);
    replace_function(0x467460, ff8_test22); */
}
