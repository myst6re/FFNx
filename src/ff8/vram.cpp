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
#define VRAM_WIDTH 512

#define VRAM_SEEK(pointer, x, y) \
    (pointer + VRAM_DEPTH * (x + y * VRAM_DEPTH * VRAM_WIDTH))

char *ff8_vram_seek(int x, int y)
{
    return VRAM_SEEK((char *)0x1B474F0, x, y);
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
    char **psxvram_buffer_page = ((char **)0x1CA86A8);

    if (*header_copy != header_with_bit_depth)
    {
        *header_copy = header_with_bit_depth;
        *flags = (header_with_bit_depth >> 5) & 3;
        *y = 16 * (header_with_bit_depth & 0x10);
        *x = (header_with_bit_depth & 0xF) << 6;
        *bit_depth = (header_with_bit_depth >> 7) & 3;
        *size = (*x / 64) | (16 * (*y / 256));
        *psxvram_buffer_page = ff8_vram_seek(*x, *y); // psxvram_buffer

        ffnx_trace("%s %d flags=%d height=%d width=%d bit_depth=%d size=%d\n",
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
            ffnx_trace("SSIGPU_SetTPage: WARNING.. bogus texture page: wrong bit depth !!\n",
                       "  current primitive from file: %s, line %i\n",
                       __func__, 0);

            *bit_depth = 1;
        }
    }
}

// Used
void ff8_vram_point_to_palette(int16_t pos_x6y10)
{
    ffnx_trace("%s %X x=%d y=%d\n", __func__, pos_x6y10, 16 * (pos_x6y10 & 0x3F), (pos_x6y10 & 0x7FFF) >> 6);

    DWORD *vram_pos_y = ((DWORD *)0x1CA8690);
    char **psxvram_pointer = ((char **)0x1CA8680);

    *vram_pos_y = pos_x6y10 & 0x7FFF;
    // Maybe a pointer to the palette or the texture without the palette
    *psxvram_pointer = ff8_vram_seek(16 * (pos_x6y10 & 0x3F), *vram_pos_y >> 6);
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

    ffnx_trace("%s x=%d y=%d w=%d h=%d %X\n", __func__, x, y, w, h, texture_buffer);

    if (h != 0)
    {
        const int w_mult_by_2 = 2 * w;
        char *psxvram_buffer = ff8_vram_seek(x, y);

        for (int i = 0; i < h; ++i)
        {
            memcpy(psxvram_buffer, texture_buffer, w_mult_by_2);
            char *texture_buffer_copy = &texture_buffer[w_mult_by_2];
            char *psxvram_buffer_copy = &psxvram_buffer[w_mult_by_2];
            texture_buffer += w_mult_by_2;
            psxvram_buffer += 2048;
            memcpy(psxvram_buffer_copy, texture_buffer_copy, size_t(w_mult_by_2 & 3));
        }
    }

    ((void(*)(uint32_t, uint32_t, uint32_t, uint32_t))0x464840)(x, y, x + w - 1, h + y - 1);

    return 1;
}

// Used
int ff8_upload_vram2(int16_t* pos_and_size, int x, int y)
{
    const int x2 = pos_and_size[0];
    const int y2 = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x2=%d y2=%d w=%d h=%d, x=%d y=%d\n", __func__, x2, y2, w, h, x, y);

    if (h != 0)
    {
        const int w_mult_by_2 = 2 * w;
        char *psxvram_buffer = ff8_vram_seek(x, y);
        char *target_buffer_pos = VRAM_SEEK((char *)psxvram_buffer, x - x2, y - y2);

        for (int i = 0; i < h; ++i)
        {
            char *psxvram_buffer_pointer_copy = psxvram_buffer;
            psxvram_buffer += 2048;
            memcpy(target_buffer_pos, psxvram_buffer_pointer_copy, w_mult_by_2);
            char *target_buffer_pos2 = &target_buffer_pos[w_mult_by_2];
            target_buffer_pos += 2048;
            memcpy(target_buffer_pos2, &psxvram_buffer_pointer_copy[w_mult_by_2], w_mult_by_2 & 3);
        }
    }

    ((void(*)(uint32_t, uint32_t, uint32_t, uint32_t))0x464840)(x, y, w + x - 1, h + y - 1);

    return 1;
}

int ff8_upload_vram_to_buffer(int16_t* pos_and_size, char* target_buffer)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x=%d y=%d w=%d h=%d\n", __func__, x, y, w, h);

    if (h != 0)
    {
        const int w_mult_by_2 = 2 * w;
        char *psxvram_buffer_pointer = ff8_vram_seek(x, y);

        for (int i = 0; i < h; ++i)
        {
            memcpy(target_buffer, psxvram_buffer_pointer, w_mult_by_2);
            char *v6 = &psxvram_buffer_pointer[w_mult_by_2];
            char *v5 = &target_buffer[w_mult_by_2];
            target_buffer += w_mult_by_2;
            psxvram_buffer_pointer += 2048;
            memcpy(v5, v6, w_mult_by_2 & 3);
        }
    }

    return 1;
}
int ff8_upload_vram3(int16_t* pos_and_size, unsigned char a3, unsigned char a4, unsigned char a5)
{

    DWORD *ff8_upload_vram2_flag = ((DWORD *)0x1B474E8);
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];

    ffnx_trace("%s x=%d y=%d w=%d h=%d %d %d %d\n", __func__, x, y, w, h, a3, a4, a5);

    if (! *ff8_upload_vram2_flag)
    {
        ((void(*)(int))0x464210)((int)ff8_vram_seek(0, 0));
        *ff8_upload_vram2_flag = 1;
    }

    int16_t v6 = (a5 >> 3) | (32 * ((a4 >> 3) | (32 * (a3 >> 3))));

    char *psxvram_buffer_pointer = ff8_vram_seek(x, y);

    if (h)
    {
        for (int i = 0; i < h; ++i)
        {
            if (w)
            {
                size_t size = w * 2;
                memset(psxvram_buffer_pointer, v6 | (v6 << 16), size);

                char *psxvram_buffer_pointer_copy = &psxvram_buffer_pointer[size];
                for (int j = w & 1; j != 0; --j)
                {
                    *(short *)psxvram_buffer_pointer_copy = v6;
                    psxvram_buffer_pointer_copy += 2;
                }
            }
            psxvram_buffer_pointer += 2048;
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

void ff8_vram_op1(int a1, int a2, int h, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d h=%d %d %d %d %d\n", __func__, a1, a2, h, a4, a5, a6, a7);

    char **psxvram_pointer = ((char **)0x1CA8680);
    char **psxvram_buffer_page = ((char **)0x1CA86A8);

    size_t size = a1 + VRAM_DEPTH * h;
    uint16_t *v9 = (uint16_t *)(a1 + VRAM_DEPTH * a2);
    int v10 = a6 << 12;
    for (int i = a7 << 12; size_t(v9) <= size; v10 += i)
    {
        uint16_t result = *(uint16_t *)(
            *psxvram_pointer + VRAM_DEPTH * (
                (*(*psxvram_buffer_page + ((a4 + (v10 & 0x3FC00000)) >> 11)) >> (a4 & 4)) & 0xF
            )
        );
        if (result)
        {
            *v9 = result;
        }

        ++v9;
        a4 += a5;
    }

}

void ff8_vram_op2(int a1, int a2, int h, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d h=%d %d %d %d %d\n", __func__, a1, a2, h, a4, a5, a6, a7);

    char **psxvram_pointer = ((char **)0x1CA8680);
    char **psxvram_buffer_page = ((char **)0x1CA86A8);

    unsigned int v7 = a1 + VRAM_DEPTH * h;
    int16_t *v8 = (int16_t *)(a1 + VRAM_DEPTH * a2);
    int v18 = a6 << 12;

    for (int i = a7 << 12; (unsigned int)v8 < v7; v18 += i)
    {
        int16_t v11 = *(int16_t *)(
            *psxvram_pointer + VRAM_DEPTH * (
                (*(*psxvram_buffer_page + ((a4 + (v18 & 0x3FC00000)) >> 11)) >> (a4 & 4)) & 0xF
            )
        );
        if (v11)
        {
            unsigned int v13 = (v11 & 0x1F) + (*v8 & 0x1F);
            if (v13 >= 0x1F)
            {
                v13 = 0x1F;
            }
            unsigned int v14 = (v11 & 0x3E0) + (*v8 & 0x3E0);
            if (v14 >= 0x3E0)
            {
                v14 = 0x3E0;
            }
            unsigned int v15 = (v11 & 0x7C00) + (*v8 & 0x7C00);
            if (v15 >= 0x7C00)
            {
                v15 = 0x7C00;
            }
            *v8 = int16_t(v14 | v13) | v15;
        }
        ++v8;
        a4 += a5;
    }
}/*

void ff8_vram_op3(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);
    unsigned int v7; // esi
    _WORD *v8; // edi
    int result; // eax
    __int16 v11; // dx
    unsigned int v12; // [esp+Ch] [ebp+4h]
    int v13; // [esp+20h] [ebp+18h]
    int i; // [esp+24h] [ebp+1Ch]

    v7 = a1 + 2 * a3;
    v8 = (_WORD *)(a1 + 2 * a2);
    v12 = v7;
    result = a6 << 12;
    v13 = a6 << 12;
    for ( i = a7 << 12; (unsigned int)v8 <= v7; v13 += i )
    {
        v11 = *(_WORD *)(psxvram_pointer_dword_1CA8680
                    + 2
                    * ((*(unsigned __int8 *)(((a4 + (result & 0x3FC00000)) >> 11)
                                            + psxvram_buffer_pointer_page_dword_1CA86A8) >> (BYTE1(a4) & 4)) & 0xF));
        if ( v11 )
        {
        v7 = v12;
        *v8 = ((((*v8 & 0x3E0) - (v11 & 0x3E0)) & 0x8000) == 0 ? (*v8 & 0x3E0) - (v11 & 0x3E0) : 0) | ((((*v8 & 0x1F) - (v11 & 0x1F)) & 0x8000) == 0 ? (*v8 & 0x1F) - (v11 & 0x1F) : 0) | ((((*v8 & 0x7C00) - (v11 & 0x7C00)) & 0x8000) == 0 ? (*v8 & 0x7C00) - (v11 & 0x7C00) : 0);
        }
        ++v8;
        a4 += a5;
        result = i + v13;
    }
    return result;
}

void ff8_vram_op4(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);

    int v7; // edx
    int psxvram_buffer_pointer_copy; // ebp
    unsigned int v9; // ebx
    unsigned int v10; // ecx
    int v11; // eax
    __int16 v12; // si
    int v13; // edi
    int v14; // ebx
    int v15; // eax
    int v16; // esi
    int v17; // edi
    int v18; // ecx
    unsigned int v19; // ebx
    unsigned int v21; // [esp+10h] [ebp-4h]
    unsigned int v22; // [esp+18h] [ebp+4h]
    int v23; // [esp+30h] [ebp+1Ch]

    v7 = psxvram_buffer_pointer_page_dword_1CA86A8;
    psxvram_buffer_pointer_copy = psxvram_pointer_dword_1CA8680;
    v9 = a1 + 2 * a2;
    v21 = a1 + 2 * a3;
    v10 = v21 & 0xFFFFFFFC;
    v22 = v9;
    v11 = a6 << 11;
    v23 = a7 << 11;
    if ( (v9 & 2) != 0 )
    {
        v12 = *(_WORD *)(psxvram_pointer_dword_1CA8680
                    + 2
                    * *(unsigned __int8 *)(((a4 + (v11 & 0x1FE00000)) >> 10) + psxvram_buffer_pointer_page_dword_1CA86A8));
        v13 = a5 + a4;
        v11 += v23;
        if ( v12 )
        {
        *(_WORD *)v9 = v12;
        v9 += 2;
        v22 = v9;
        }
        v10 = v21 & 0xFFFFFFFC;
    }
    else
    {
        v13 = a4;
    }
    if ( v9 < v10 )
    {
        do
        {
        HIWORD(v16) = 0;
        v14 = *(unsigned __int8 *)(((v13 + (v11 & 0x1FE00000)) >> 10) + v7);
        v15 = v23 + v11;
        LOWORD(v16) = *(_WORD *)(psxvram_buffer_pointer_copy + 2 * v14);
        v17 = a5 + v13;
        v18 = *(unsigned __int16 *)(psxvram_buffer_pointer_copy
                                    + 2 * *(unsigned __int8 *)(((v17 + (v15 & 0x1FE00000)) >> 10) + v7));
        v13 = a5 + v17;
        v11 = v23 + v15;
        if ( ((unsigned __int16)v18 & (unsigned __int16)v16) != 0 )
        {
            *(_DWORD *)v22 = v16 | (v18 << 16);
        }
        else
        {
            v19 = v22;
            if ( v16 )
            *(_WORD *)v22 = v16;
            if ( !v18 )
            goto LABEL_14;
            *(_WORD *)(v22 + 2) = v18;
        }
        v19 = v22;
    LABEL_14:
        v9 = v19 + 4;
        v22 = v9;
        }
        while ( v9 < (v21 & 0xFFFFFFFC) );
    }
    if ( v9 < v21 )
    {
        LOWORD(v11) = *(_WORD *)(psxvram_buffer_pointer_copy
                            + 2 * *(unsigned __int8 *)(((v13 + (v11 & 0x1FE00000)) >> 10) + v7));
        if ( (_WORD)v11 )
        *(_WORD *)v9 = v11;
    }
    return v11;
}

void ff8_vram_op5(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);

    int result; // eax
    unsigned int v8; // esi
    __int16 *v9; // edi
    int v10; // ebx
    __int16 v11; // ax
    __int16 v12; // cx
    unsigned int v13; // edx
    unsigned int v14; // esi
    unsigned int v15; // ecx
    __int16 v16; // dx
    unsigned int v17; // [esp+10h] [ebp+4h]
    int i; // [esp+28h] [ebp+1Ch]

    result = a1;
    v9 = (__int16 *)(a1 + 2 * a2);
    v10 = a6 << 11;
    v17 = a1 + 2 * a3;
    v8 = v17;
    for ( i = a7 << 11; (unsigned int)v9 < v8; a4 += a5 )
    {
        v11 = *(_WORD *)(psxvram_pointer_dword_1CA8680
                    + 2
                    * *(unsigned __int8 *)(((a4 + (v10 & 0x1FE00000)) >> 10) + psxvram_buffer_pointer_page_dword_1CA86A8));
        if ( v11 )
        {
        v12 = *v9;
        v13 = (v11 & 0x1F) + (*v9 & 0x1F);
        if ( v13 >= 0x1F )
            LOWORD(v13) = 31;
        v14 = (v11 & 0x3E0) + (v12 & 0x3E0);
        if ( v14 >= 0x3E0 )
            LOWORD(v14) = 992;
        v15 = (v11 & 0x7C00) + (v12 & 0x7C00);
        v16 = v14 | v13;
        if ( v15 >= 0x7C00 )
            LOWORD(v15) = 31744;
        v8 = v17;
        *v9 = v16 | v15;
        }
        result = a5;
        ++v9;
        v10 += i;
    }
    return result;
}

void ff8_vram_op6(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);

    int result; // eax
    int v8; // ebx
    unsigned int v9; // esi
    _WORD *v10; // edi
    __int16 v11; // dx
    unsigned int v12; // [esp+10h] [ebp+4h]
    int i; // [esp+28h] [ebp+1Ch]

    result = a3;
    v8 = a6 << 11;
    v9 = a1 + 2 * a3;
    v10 = (_WORD *)(a1 + 2 * a2);
    v12 = v9;
    for ( i = a7 << 11; (unsigned int)v10 <= v9; a4 += a5 )
    {
        v11 = *(_WORD *)(psxvram_pointer_dword_1CA8680
                    + 2
                    * *(unsigned __int8 *)(((a4 + (v8 & 0x1FE00000)) >> 10) + psxvram_buffer_pointer_page_dword_1CA86A8));
        if ( v11 )
        {
        v9 = v12;
        *v10 = ((((*v10 & 0x3E0) - (v11 & 0x3E0)) & 0x8000) == 0 ? (*v10 & 0x3E0) - (v11 & 0x3E0) : 0) | ((((*v10 & 0x1F) - (v11 & 0x1F)) & 0x8000) == 0 ? (*v10 & 0x1F) - (v11 & 0x1F) : 0) | ((((*v10 & 0x7C00) - (v11 & 0x7C00)) & 0x8000) == 0 ? (*v10 & 0x7C00) - (v11 & 0x7C00) : 0);
        }
        result = a5;
        ++v10;
        v8 += i;
    }
    return result;
}

void ff8_vram_op7(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);

    int psxvram_buffer_pointer_copy; // ebp
    unsigned int v8; // edi
    _WORD *result; // eax
    int v10; // ecx
    int i; // [esp+28h] [ebp+1Ch]

    psxvram_buffer_pointer_copy = psxvram_buffer_pointer_page_dword_1CA86A8;
    v8 = a1 + 2 * a3;
    result = (_WORD *)(a1 + 2 * a2);
    v10 = a6 << 10;
    for ( i = a7 << 10; (unsigned int)result <= v8; v10 += i )
    {
        if ( *(_BYTE *)(((a4 + (v10 & 0xFF00000)) >> 10) + psxvram_buffer_pointer_copy) )
        *result = *(unsigned __int8 *)(((a4 + (v10 & 0xFF00000)) >> 10) + psxvram_buffer_pointer_copy);
        ++result;
        a4 += a5;
    }
    return result;
}

void ff8_vram_op8(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);

    int result; // eax
    unsigned int v8; // edx
    __int16 *v9; // edi
    int v10; // ebx
    __int16 v11; // cx
    __int16 v12; // ax
    unsigned int v13; // edx
    unsigned int v14; // esi
    unsigned int v15; // eax
    int v16; // edx
    unsigned int v17; // [esp+Ch] [ebp+4h]
    int i; // [esp+24h] [ebp+1Ch]

    result = a1;
    v8 = a1 + 2 * a3;
    v17 = v8;
    v9 = (__int16 *)(result + 2 * a2);
    v10 = a6 << 10;
    for ( i = a7 << 10; (unsigned int)v9 < v8; a4 += a5 )
    {
        result = (a4 + (v10 & 0xFF00000)) >> 10;
        v11 = *(unsigned __int8 *)(result + psxvram_buffer_pointer_page_dword_1CA86A8);
        if ( *(_BYTE *)(result + psxvram_buffer_pointer_page_dword_1CA86A8) )
        {
        v12 = *v9;
        v13 = (v11 & 0x1F) + (*v9 & 0x1F);
        if ( v13 >= 0x1F )
            v13 = 31;
        v14 = (v11 & 0x3E0) + (v12 & 0x3E0);
        if ( v14 >= 0x3E0 )
            v14 = 992;
        v15 = (v11 & 0x7C00) + (v12 & 0x7C00);
        v16 = v14 | v13;
        if ( v15 >= 0x7C00 )
            v15 = 31744;
        result = v16 | v15;
        v8 = v17;
        *v9 = result;
        }
        ++v9;
        v10 += i;
    }
    return result;
}

void ff8_vram_op9(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    ffnx_trace("%s %d %d %d %d %d %d %d\n", __func__, a1, a2, a3, a4, a5, a6, a7);

    int v7; // ebx
    unsigned int result; // eax
    _WORD *v9; // edi
    __int16 v10; // dx
    unsigned int v11; // [esp+Ch] [ebp+4h]
    int i; // [esp+24h] [ebp+1Ch]

    v7 = a6 << 10;
    v9 = (_WORD *)(a1 + 2 * a2);
    v11 = a1 + 2 * a3;
    result = v11;
    for ( i = a7 << 10; (unsigned int)v9 <= result; a4 += a5 )
    {
        if ( *(_BYTE *)(((a4 + (v7 & 0xFF00000)) >> 10) + psxvram_buffer_pointer_page_dword_1CA86A8) )
        {
        result = v11;
        v10 = *(unsigned __int8 *)(((a4 + (v7 & 0xFF00000)) >> 10) + psxvram_buffer_pointer_page_dword_1CA86A8);
        *v9 = ((((*v9 & 0x3E0) - (v10 & 0x3E0)) & 0x8000) == 0 ? (*v9 & 0x3E0) - (v10 & 0x3E0) : 0) | ((((*v9 & 0x1F) - (v10 & 0x1F)) & 0x8000) == 0 ? (*v9 & 0x1F) - (v10 & 0x1F) : 0) | ((((*v9 & 0x7C00) - (v10 & 0x7C00)) & 0x8000) == 0 ? (*v9 & 0x7C00) - (v10 & 0x7C00) : 0);
        }
        ++v9;
        v7 += i;
    }
    return result;
}

void ff8_test14(int a1, int a2)
{
    ffnx_trace("%s %d %d\n", __func__, a1, a2);

    char *psxvram_buffer_pointer; // edx
    int result; // eax
    int *v5; // esi
    int *v6; // ebx
    int *v7; // eax
    int v8; // ecx
    char *psxvram_buffer_pointer_copy; // edi
    unsigned int v10; // ecx
    int v11; // ebp
    char v12; // cf
    char *v13; // edi
    int i; // ecx
    bool v15; // zf
    int v16; // [esp+Ch] [ebp+4h]

    psxvram_buffer_pointer = (char *)&psxvram_buffer + 2048 * a2;
    result = a3 - 1;
    v5 = &dword_1CA9F88;
    v6 = &dword_1CA9FB0;
    if ( a3 - 1 >= 0 )
    {
        v16 = a3;
        do
        {
        if ( *v5 > *v6 )
        {
            v7 = v5;
            v5 = v6;
            v6 = v7;
        }
        v8 = *v5 >> 16;
        if ( (*v6 >> 16) - v8 > 0 )
        {
            psxvram_buffer_pointer_copy = &psxvram_buffer_pointer[2 * v8];
            v10 = (*v6 >> 16) - v8;
            LOWORD(a1) = dword_1CA9FA8;
            v11 = a1 << 16;
            LOWORD(v11) = dword_1CA9FA8;
            v12 = v10 & 1;
            v10 >>= 1;
            memset32(psxvram_buffer_pointer_copy, v11, v10);
            v13 = &psxvram_buffer_pointer_copy[4 * v10];
            for ( i = v12; i; --i )
            {
            *(_WORD *)v13 = v11;
            v13 += 2;
            }
        }
        a1 = dword_1CACEE0 + dword_1CA9F88;
        psxvram_buffer_pointer += 2048;
        result = v16 - 1;
        v15 = v16 == 1;
        dword_1CA9F88 += dword_1CACEE0;
        dword_1CA9FB0 += dword_1CA9FD8;
        --v16;
        }
        while ( !v15 );
    }
    return result;
}
*/
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
    replace_function(0x45B910, ff8_vram_point_to_palette);
    replace_function(0x45B950, ff8_ssigpu_set_t_page);
    replace_function(0x45BAF0, ff8_download_vram);
    replace_function(0x45BD20, ff8_upload_vram);
    replace_function(0x45BDC0, ff8_upload_vram2);
    replace_function(0x45BE60, ff8_upload_vram_to_buffer);
    replace_function(0x45BED0, ff8_upload_vram3);
    replace_function(0x462390, ff8_test1); */

    // Operations
    /* replace_function(0x4633B0, ff8_vram_op1);
    replace_function(0x463440, ff8_vram_op2);
    replace_function(0x463540, ff8_vram_op3);
    replace_function(0x463640, ff8_vram_op4);
    replace_function(0x463790, ff8_vram_op5);
    replace_function(0x463880, ff8_vram_op6);
    replace_function(0x463970, ff8_vram_op7);
    replace_function(0x4639E0, ff8_vram_op8);
    replace_function(0x463AC0, ff8_vram_op9);

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
