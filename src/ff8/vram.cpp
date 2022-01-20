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
    *psxvram_current_clut_ptr = vram_seek(16 * (pos_x6y10 & 0x3F), *vram_pos_y >> 6);
}

// Not used?
void ff8_download_vram(DWORD* param)
{
    ffnx_trace("%s %X\n", __func__, param);
}

int ff8_upload_vram(int16_t *pos_and_size, char *texture_buffer)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x=%d y=%d w=%d h=%d buffer=0x%X\n", __func__, x, y, w, h, texture_buffer);

    if (h != 0)
    {
        const int w_mult_by_2 = VRAM_DEPTH * w;
        char *psxvram_pointer = vram_seek(x, y);

        for (int i = 0; i < h; ++i)
        {
            memcpy(psxvram_pointer, texture_buffer, w_mult_by_2);

            texture_buffer += w_mult_by_2;
            psxvram_pointer += VRAM_WIDTH * VRAM_DEPTH;
        }
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

    uint16_t *psxvram_buffer_pointer = vram_seek(x, y);

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

    uint16_t *cur = pointer + VRAM_DEPTH * y;
    uint16_t *max = pointer + VRAM_DEPTH * h;
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

    uint16_t *cur = pointer + VRAM_DEPTH * y;
    uint16_t *max = pointer + VRAM_DEPTH * h;
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

    uint16_t *cur = pointer + VRAM_DEPTH * y;
    uint16_t *max = pointer + VRAM_DEPTH * h;
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

    if ((cur & 2) != 0) // Not multiple of 4
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

    for (; cur < (max & 0xFFFFFFFC); ++cur)
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

    if (cur < size)
    {
        uint16_t color = *(uint16_t *)(
            *psxvram_current_clut_ptr + VRAM_DEPTH * *(uint8_t *)(
                ((x2 + (y & 0x1FE00000)) / VRAM_WIDTH) + v7
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
    ffnx_trace("%s pointer=%X x1=%d w1=%d x2=%d w2=%d y2=%d h2=%d\n", __func__, pointer, x1, w1, x2, w2, y2, h2);

    int16_t *cur = pointer + VRAM_DEPTH * x1;
    int16_t *max = pointer + VRAM_DEPTH * w1;
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

void ff8_vram_op_add_indexed8(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);

    size_t size = a1 + VRAM_DEPTH * a3;
    int16_t *v9 = (int16_t *)(a1 + VRAM_DEPTH * a2);
    int v10 = a6 * VRAM_WIDTH;
    int i = a7 * VRAM_WIDTH;

    for (; size_t(cur) < size; ++cur)
    {
        int16_t color = *(uint8_t *)(
            *psxvram_current_texture_ptr + (
                (a4 + (v10 & 0xFF00000)) / VRAM_WIDTH
            )
        );

        if (color != 0)
        {
            *cur = color_add(color, *cur);
        }

        a4 += a5;
        v10 += i;
    }
}

void ff8_vram_op_sub_indexed8(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);

    size_t size = a1 + VRAM_DEPTH * a3;
    uint16_t *cur = (uint16_t *)(a1 + VRAM_DEPTH * a2);
    int v7 = a6 * VRAM_WIDTH;
    int i = a7 * VRAM_WIDTH;

    for (; size_t(cur) <= size; ++cur)
    {
        // FIXME: why int8_t cast???
        uint16_t color = *(int8_t *)(
            *psxvram_current_texture_ptr + (
                (a4 + (v7 & 0xFF00000)) / VRAM_WIDTH
            )
        );

        if (color != 0)
        {
            *cur = color_sub(*cur, color);
        }

        a4 += a5;
        v7 += i;
    }
}

// TODO: check signature
void ff8_test14(int a1, int y, int h)
{
    ffnx_trace("%s unused=%d y=%d h=%d\n", __func__, a1, y, h);

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
}

void ff8_test15(int a1)
{
    ffnx_trace("%s %d\n", __func__, a1);
}

void ff8_test16(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);
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
}

void ff8_test22(int a1, int a2, int a3)
{
    ffnx_trace("%s %d %d %d\n", __func__, a1, a2, a3);
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
    /* replace_function(0x4633B0, ff8_vram_op_set16);
    replace_function(0x463440, ff8_vram_op_add16);
    replace_function(0x463540, ff8_vram_op_sub16);
    replace_function(0x463640, ff8_vram_op_set32);
    replace_function(0x463790, ff8_vram_op_add16b);
    replace_function(0x463880, ff8_vram_op_sub16b);
    replace_function(0x463970, ff8_vram_op_set_indexed8);
    replace_function(0x4639E0, ff8_vram_op_add_indexed8);
    replace_function(0x463AC0, ff8_vram_op_sub_indexed8);

    replace_function(0x463D40, ff8_test14);
    replace_function(0x466EE0, ff8_test15);

    replace_function(0x464990, ff8_test16);
    replace_function(0x464DA0, ff8_test17); */
    replace_function(0x464F60, ff8_test18);
    /* replace_function(0x4653A0, ff8_test19);
    replace_function(0x465710, ff8_test20); */

    // GPU read
    /* replace_function(0x467360, ff8_test21);
    replace_function(0x467460, ff8_test22); */
}
