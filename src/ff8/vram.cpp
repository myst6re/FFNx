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
#include "../image/tim.h"

char next_texture_name[MAX_PATH] = "";
char last_texture_name[MAX_PATH] = "";
uint16_t *next_pal_data = nullptr;
int next_psxvram_x = -1;
int next_psxvram_y = -1;
bool next_is_wm = false;
uint8_t next_bpp = 2;
uint8_t next_scale = 1;
int8_t texl_id_left = -1;
int8_t texl_id_right = -1;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')


void print_struct_50()
{
    for (int bpp = 0; bpp < 3; ++bpp)
    {
        ffnx_info("bpp = %d\n", bpp);

        for (int i = 0; i < 32; ++i)
        {
            const struc_50 &current = ff8_externals.psx_texture_pages[bpp].struc_50_array[i];

            if (current.texture_page_enabled || current.initialized)
            {
                ffnx_info(
                    "texture %d\n"
                    "initialized = %X " BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN "\n"
                    "texture_page_enabled = %X " BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN "\n"
                    "field_328 = %X\n"
                    "vram_needs_reload = %d\n"
                    "field_330 = %X\n"
                    "vram = (%d, %d, %d, %d)\n"
                    "vram_palette_data = %p\n"
                    "vram_palette_pos = %X\n"
                    "==\n",
                    i,
                    current.initialized, BYTE_TO_BINARY(current.initialized >> 24), BYTE_TO_BINARY(current.initialized >> 16), BYTE_TO_BINARY(current.initialized >> 8), BYTE_TO_BINARY(current.initialized >> 0),
                    current.texture_page_enabled, BYTE_TO_BINARY(current.texture_page_enabled >> 24), BYTE_TO_BINARY(current.texture_page_enabled >> 16), BYTE_TO_BINARY(current.texture_page_enabled >> 8), BYTE_TO_BINARY(current.texture_page_enabled >> 0),
                    current.field_328,
                    current.vram_needs_reload,
                    current.field_330,
                    current.vram_x,
                    current.vram_y,
                    current.vram_width,
                    current.vram_height,
                    current.vram_palette_data,
                    current.vram_palette_pos
                );

                for (int j = 0; j < 8; ++j)
                {
                    if (current.texture_page_enabled & (0x1 << j) || current.initialized & (0x1 << j))
                    {
                        const texture_page &tp = current.texture_page[j];

                        ffnx_info(
                            "page %d\n"
                            "field_0 %X\n"
                            "coord = (%d, %d, %d, %d)\n"
                            "color_key = %d\n"
                            "uv = (%f, %f)\n"
                            "field_20 = %X\n"
                            "===\n",
                            j,
                            tp.field_0,
                            tp.x,
                            tp.y,
                            tp.width,
                            tp.height,
                            tp.color_key,
                            tp.u,
                            tp.v,
                            tp.field_20
                        );
                    }
                }
            }
        }
    }
}

void dump_struct_50()
{
    for (int bpp = 0; bpp < 3; ++bpp)
    {
        for (int i = 0; i < 32; ++i)
        {
            const struc_50 &current = ff8_externals.psx_texture_pages[bpp].struc_50_array[i];

            if (current.initialized)
            {
                for (int j = 0; j < 8; ++j)
                {
                    if (current.initialized & (0x1 << j))
                    {
                        const texture_page &tp = current.texture_page[j];

                        uint32_t color_key = tp.color_key == 0 ? 1 : tp.color_key;
                        ff8_tim tim_infos = ff8_tim();
                        tim_infos.img_w = tp.width >> (2 - color_key);
                        tim_infos.img_h = tp.height;
                        tim_infos.img_data = (uint8_t *)tp.image_data;
                        tim_infos.pal_data = (uint16_t *)(tp.tex_header->tex_format.palette_data);

                        char fileName[MAX_PATH];
                        snprintf(fileName, MAX_PATH, "struct_50_bpp_%d_vram_%d_page_%d", bpp, i, j);
                        save_tim(fileName, color_key, &tim_infos, tp.tex_header->palette_index);
                    }
                }
            }
        }
    }
}

void ff8_upload_vram(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];
    bool isPal = next_pal_data != nullptr && (uint8_t *)next_pal_data == texture_buffer;

    if (trace_all || trace_vram) ffnx_trace("%s x=%d y=%d w=%d h=%d bpp=%d isPal=%d\n", __func__, x, y, w, h, next_bpp, isPal);

    texturePacker.setTexture(isPal ? last_texture_name : next_texture_name, texture_buffer, x, y, w, h, next_bpp, isPal);

    ff8_externals.sub_464850(x, y, x + w - 1, h + y - 1);

    strncpy(last_texture_name, next_texture_name, MAX_PATH);
    *next_texture_name = '\0';

    // if (trace_all || trace_vram) ffnx_trace("%s after\n", __func__);

    // print_struct_50();
}

void ff8_wm_texl_palette_upload_vram(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    int *dword_C75DB8 = (int *)0xC75DB8;
    int16_t *word_203688E = (int16_t *)0x203688E;
    int texl_id = (*dword_C75DB8 >> 8) & 0xFF;

    if ((*word_203688E & 1) != 0 && (*dword_C75DB8 == 1284 || *dword_C75DB8 == 1798))
    {
        texl_id += 12;
    }

    bool is_left = pos_and_size[0] == 320;

    if (is_left)
    {
        texl_id_left = texl_id;
    }
    else
    {
        texl_id_right = texl_id;
    }

    if (trace_all || trace_vram) ffnx_trace("%s texl_id=%d\n", __func__, texl_id);

    ff8_upload_vram(pos_and_size, texture_buffer);

    ff8_externals.psx_texture_pages[1].struc_50_array[is_left ? 20 : 22].texture_page_enabled = 1;
}

void ff8_wm_texl_palette_upload_vram2(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    int *dword_C75DB8 = (int *)0xC75DB8;
    int16_t *word_203688E = (int16_t *)0x203688E;
    int texl_id = (*dword_C75DB8 >> 8) & 0xFF;

    if ((*word_203688E & 1) != 0 && (*dword_C75DB8 == 1284 || *dword_C75DB8 == 1798))
    {
        texl_id += 12;
    }

    bool is_left = pos_and_size[0] == 320;

    if (is_left)
    {
        texl_id_left = texl_id;
    }
    else
    {
        texl_id_right = texl_id;
    }

    if (trace_all || trace_vram) ffnx_trace("%s texl_id=%d word_203688E=%d\n", __func__, texl_id, *word_203688E);

    print_struct_50();
    dump_struct_50();

    texturePacker.saveVram(next_texture_name, 0);
    texturePacker.saveVram(next_texture_name, 1);
    texturePacker.saveVram(next_texture_name, 2);

    ff8_upload_vram(pos_and_size, texture_buffer);
}

int ff8_wm_draw_frame(void *a1)
{
    int *dword_1CA8500 = (int *)0x1CA8500;
    int *dword_B7CBF8 = (int *)0xB7CBF8;

    if (trace_all || trace_vram) ffnx_trace("%s a1=%X 1CA8500=%p\n", __func__, a1, *dword_1CA8500);

    ffnx_trace("%s %X\n", __func__, *(int *)a1);

    for (; dword_B7CBF8 <= (int *)0xB7CC24; dword_B7CBF8++)
    {
        ffnx_trace("%s %X=%d\n", __func__, dword_B7CBF8, *dword_B7CBF8);
    }

    return ((int(*)(void*))ff8_externals.sub_45D080)(a1);
}

void ff8_read_buffer_parent_parent()
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    //print_struct_50();

    ((void(*)())ff8_externals.sub_464BD0)();

    print_struct_50();
}

void ff8_unknown_read_vram(void *a1)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);
}

void ff8_read_vram_1(int CLUT, int *target, int size)
{
    if (trace_all || trace_vram) ffnx_trace("%s CLUT=%d size=%d\n", __func__, CLUT, size);


}

int ff8_tx_select_call1(uint32_t a1, uint32_t header_with_bit_depth, int16_t CLUT_pos_x6y9, DWORD *a4, int *a5, DWORD *a6)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    return ((int(*)(uint32_t a1, uint32_t, int16_t, DWORD *, int *, DWORD *))ff8_externals.ssigpu_tx_select_2_sub_465CE0)(a1, header_with_bit_depth, CLUT_pos_x6y9, a4, a5, a6);
}

int ff8_tx_select_call2(uint32_t a1, uint32_t header_with_bit_depth, int16_t CLUT_pos_x6y9, DWORD *a4, int *a5, DWORD *a6)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    return ((int(*)(uint32_t a1, uint32_t, int16_t, DWORD *, int *, DWORD *))ff8_externals.ssigpu_tx_select_2_sub_465CE0)(a1, header_with_bit_depth, CLUT_pos_x6y9, a4, a5, a6);
}

int ff8_tx_select_call3(uint32_t a1, uint32_t header_with_bit_depth, int16_t CLUT_pos_x6y9, DWORD *a4, int *a5, DWORD *a6)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    return ((int(*)(uint32_t a1, uint32_t, int16_t, DWORD *, int *, DWORD *))ff8_externals.ssigpu_tx_select_2_sub_465CE0)(a1, header_with_bit_depth, CLUT_pos_x6y9, a4, a5, a6);
}

int ff8_tx_select_call4(uint32_t a1, uint32_t header_with_bit_depth, int16_t CLUT_pos_x6y9, DWORD *a4, int *a5, DWORD *a6)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    return ((int(*)(uint32_t a1, uint32_t, int16_t, DWORD *, int *, DWORD *))ff8_externals.ssigpu_tx_select_2_sub_465CE0)(a1, header_with_bit_depth, CLUT_pos_x6y9, a4, a5, a6);
}

int read_vram_to_buffer_parent_call1(int a1, int structure, int x, int y, int w, int h, int bpp, int rel_pos, int a9, uint8_t *target)
{
    if (trace_all || trace_vram) ffnx_trace("%s: x=%d y=%d w=%d h=%d bpp=%d rel_pos=(%d, %d) a9=%d target=%X\n", __func__, x, y, w, h, bpp, rel_pos & 0xF, rel_pos >> 4, a9, target);

    next_psxvram_x = (x >> (2 - bpp)) + ((rel_pos & 0xF) << 6);
    next_psxvram_y = y + (((rel_pos >> 4) & 1) << 8);
    char name[MAX_PATH];

    for (int i = 0; i < 8; ++i)
    {
        snprintf(name, MAX_PATH, "world/dat/wmset/section38/texture%d", i);
        if (texturePacker.hasTexture(name, next_psxvram_x, next_psxvram_y, w, h))
        {
            if (trace_all || trace_vram) ffnx_trace("%s: has wm texture %i\n", __func__, i);

            /* w *= 2;
            h *= 2;
            bpp = 1; */

            next_is_wm = true;

            break;
        }
    }

    int ret = ff8_externals.sub_464F70(a1, structure, x, y, w, h, bpp, rel_pos, a9, target);

    next_psxvram_x = -1;
    next_psxvram_y = -1;
    next_is_wm = false;

    return ret;
}

int read_vram_to_buffer_parent_call2(texture_page *tex_page, int rel_pos, int a3)
{
    if (trace_all || trace_vram) ffnx_trace("%s: x=%d y=%d color_key=%d rel_pos=(%d, %d)\n", __func__, tex_page->x, tex_page->y, tex_page->color_key, rel_pos & 0xF, rel_pos >> 4);

    next_psxvram_x = (tex_page->x >> (2 - tex_page->color_key)) + ((rel_pos & 0xF) << 6);
    next_psxvram_y = tex_page->y + (((rel_pos >> 4) & 1) << 8);

    int ret = ((int(*)(texture_page *, int, int))ff8_externals.sub_4653B0)(tex_page, rel_pos, a3);

    next_psxvram_x = -1;
    next_psxvram_y = -1;

    return ret;
}

int read_vram_to_buffer_with_palette1_parent_call1(texture_page *tex_page, int rel_pos, struc_50 *psxvram_structure)
{
    if (trace_all || trace_vram) ffnx_trace("%s: x=%d y=%d color_key=%d rel_pos=(%d, %d)\n", __func__, tex_page->x, tex_page->y, tex_page->color_key, rel_pos & 0xF, rel_pos >> 4);

    next_psxvram_x = (tex_page->x >> (2 - tex_page->color_key)) + ((rel_pos & 0xF) << 6);
    next_psxvram_y = tex_page->y + (((rel_pos >> 4) & 1) << 8);


	ffnx_info("%s: %d %d\n", __func__, psxvram_structure->vram_palette_pos & 0x3F, psxvram_structure->vram_palette_pos >> 6);

    int ret = ((int(*)(texture_page*,int,struc_50*))ff8_externals.sub_464DB0)(tex_page, rel_pos, psxvram_structure);

    next_psxvram_x = -1;
    next_psxvram_y = -1;

    return ret;
}

int read_vram_to_buffer_with_palette1_parent_call2(texture_page *tex_page, int rel_pos, struc_50 *psxvram_structure)
{
    if (trace_all || trace_vram) ffnx_trace("%s: x=%d y=%d color_key=%d rel_pos=(%d, %d)\n", __func__, tex_page->x, tex_page->y, tex_page->color_key, rel_pos & 0xF, rel_pos >> 4);

    next_psxvram_x = (tex_page->x >> (2 - tex_page->color_key)) + ((rel_pos & 0xF) << 6);
    next_psxvram_y = tex_page->y + (((rel_pos >> 4) & 1) << 8);

	ffnx_info("%s: %d %d\n", __func__, psxvram_structure->vram_palette_pos & 0x3F, psxvram_structure->vram_palette_pos >> 6);

    int ret = ((int(*)(texture_page*,int,struc_50*))ff8_externals.sub_465720)(tex_page, rel_pos, psxvram_structure);

    next_psxvram_x = -1;
    next_psxvram_y = -1;

    return ret;
}

void read_vram_to_buffer(uint8_t *vram, int vram_w_2048, uint8_t *target, int target_w, signed int w, int h, int bpp)
{
    if (trace_all || trace_vram) ffnx_trace("%s: vram_pos=(%d, %d) target=%X target_w=%d w=%d h=%d bpp=%d\n", __func__, next_psxvram_x, next_psxvram_y, int(target), target_w, w, h, bpp);

    if (next_psxvram_x == -1)
    {
        ffnx_warning("%s: cannot detect VRAM position\n", __func__);
    }
    else
    {
        texturePacker.registerTiledTex(target, next_psxvram_x, next_psxvram_y, bpp);
    }

    if (w == 512 && h == 512)
    {
        ffnx_trace("%s: WM hack\n", __func__);
        vram = (uint8_t *)ff8_externals.psxvram_buffer + VRAM_DEPTH * (256 + 256 * VRAM_WIDTH);
        ff8_externals.read_vram_1(vram, vram_w_2048, target, target_w, 256, 256, bpp);
        vram = (uint8_t *)ff8_externals.psxvram_buffer + VRAM_DEPTH * (512 + 256 * VRAM_WIDTH);
        ff8_externals.read_vram_1(vram, vram_w_2048, target + target_w * 256, target_w, 256, 256, bpp);
    }
    else
    {
        ff8_externals.read_vram_1(vram, vram_w_2048, target, target_w, w, h, bpp);
    }
}

void read_vram_to_buffer_with_palette1(uint8_t *vram, int vram_w_2048, uint8_t *target, int target_w, int w, int h, int bpp, uint16_t *vram_palette)
{
    int *dword_1CA8690 = (int *)0x1CA8690;

    if (trace_all || trace_vram) ffnx_trace("%s: vram_pos=(%d, %d) target=%X target_w=%d w=%d h=%d bpp=%d vram_palette=%X pal=(%d, %d)\n", __func__, next_psxvram_x, next_psxvram_y, int(target), target_w, w, h, bpp, int(vram_palette), *dword_1CA8690 & 0x3F, *dword_1CA8690 >> 6);

    if (next_psxvram_x == -1)
    {
        ffnx_warning("%s: cannot detect VRAM position\n", __func__);
    }
    else
    {
        texturePacker.registerTiledTex(target, next_psxvram_x, next_psxvram_y, bpp);
    }

    ff8_externals.read_vram_2_paletted(vram, vram_w_2048, target, target_w, w, h, bpp, vram_palette);
}

void read_vram_to_buffer_with_palette2(uint8_t *vram, uint8_t *target, int w, int h, int bpp, uint16_t *vram_palette)
{
    if (trace_all || trace_vram) ffnx_trace("%s: vram_pos=(%d, %d) target=%X w=%d h=%d bpp=%d vram_palette=%X\n", __func__, next_psxvram_x, next_psxvram_y, int(target), w, h, bpp, int(vram_palette));

    if (next_psxvram_x == -1)
    {
        ffnx_warning("%s: cannot detect VRAM position\n", __func__);
    }
    else
    {
        texturePacker.registerTiledTex(target, next_psxvram_x, next_psxvram_y, bpp);
    }

	int *dword_1CA8690 = (int *)0x1CA8690;

	ffnx_info("%s: %d %d\n", __func__, *dword_1CA8690 & 0x3F, *dword_1CA8690 >> 6);

    ff8_externals.read_vram_3_paletted(vram, target, w, h, bpp, vram_palette);
}

uint32_t ff8_credits_open_texture(char *fileName, char *buffer)
{
    if (trace_all || trace_vram) ffnx_trace("%s: %s\n", __func__, fileName);

    // {name}.lzs
    strncpy(next_texture_name, strrchr(fileName, '\\') + 1, MAX_PATH);
    next_bpp = 2;

    uint32_t ret = ff8_externals.credits_open_file(fileName, buffer);

    if (save_textures) save_lzs(next_texture_name, (uint8_t *)buffer);

    return ret;
}

void ff8_cdcheck_error_upload_vram(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    strncpy(next_texture_name, "discerr.lzs", MAX_PATH);
    next_bpp = 2;

    if (save_textures) save_lzs(next_texture_name, texture_buffer - 8);

    ff8_upload_vram(pos_and_size, texture_buffer);
}

void ff8_upload_vram_triple_triad_1(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    if (trace_all || trace_vram) ffnx_trace("%s %p\n", __func__, texture_buffer);

    if (texture_buffer == (uint8_t *)0xC4ACC0)
    {
        strncpy(next_texture_name, "cardgame/intro", MAX_PATH);
        next_bpp = 2;
    }
    else if (texture_buffer == (uint8_t *)0xC20CAC)
    {
        strncpy(next_texture_name, "cardgame/game", MAX_PATH);
        next_bpp = 2;
    }

    if (save_textures && *next_texture_name != '\0')
    {
        ff8_tim tim = ff8_tim();
        tim.img_w = pos_and_size[2];
        tim.img_h = pos_and_size[3];
        tim.img_data = texture_buffer;
        save_tim(next_texture_name, next_bpp, &tim);
    }

    ff8_upload_vram(pos_and_size, texture_buffer);
}

void ff8_upload_vram_triple_triad_2(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    if (trace_all || trace_vram) ffnx_trace("%s %p\n", __func__, texture_buffer);

    ff8_upload_vram(pos_and_size, texture_buffer);
}

void ff8_wm_open_pal1(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    if (trace_all || trace_vram) ffnx_trace("%s %p\n", __func__, texture_buffer);

    next_pal_data = (uint16_t *)texture_buffer;

    ff8_upload_vram(pos_and_size, texture_buffer);

    next_pal_data = nullptr;
}

uint32_t ff8_wm_open_texture1(uint8_t *tim_file_data, ff8_tim *tim_infos)
{
    uint8_t bpp = tim_file_data[4] & 0x3;
    int *wm_section_38_textures_pos = *((int **)0x2040014);
    int searching_value = int(tim_file_data - (uint8_t *)wm_section_38_textures_pos);
    int timId = -1;
    int *dword_C75DB8 = (int *)0xC75DB8;

    if (trace_all || trace_vram) ffnx_trace("%s C75DB8=%d\n", __func__, *dword_C75DB8);

    // Find tim id relative to the start of section 38
    for (int *cur = wm_section_38_textures_pos; *cur != 0; ++cur) {
        if (*cur == searching_value) {
            timId = int(cur - wm_section_38_textures_pos);
            break;
        }
    }
    //*dword_C75DB8 = 0x0000; // Force upload texl textures

    if (timId == 0) // TODO: temp if
    {
        snprintf(next_texture_name, MAX_PATH, "world/dat/wmset/section38/texture%d", timId);
    }

    next_bpp = bpp;

    uint32_t ret = ((uint32_t(*)(uint8_t*,ff8_tim*))0x541740)(tim_file_data, tim_infos);

    next_pal_data = tim_infos->pal_data;

    if (save_textures) save_tim(next_texture_name, bpp, tim_infos);

    return ret;
}

uint32_t ff8_wm_open_texture2(uint8_t *pointer, ff8_tim *texture_infos)
{
    int *dword_C75DB8 = (int *)0xC75DB8;

    if (trace_all || trace_vram) ffnx_trace("%s C75DB8=%d\n", __func__, *dword_C75DB8);
    //*dword_C75DB8 = 0x0000; // Force upload texl textures

    return ((uint32_t(*)(uint8_t*,ff8_tim*))0x541740)(pointer, texture_infos);
}

uint32_t ff8_wm_open_texture3(uint8_t *pointer, ff8_tim *texture_infos)
{
    int *dword_C75DB8 = (int *)0xC75DB8;

    if (trace_all || trace_vram) ffnx_trace("%s C75DB8=%d\n", __func__, *dword_C75DB8);
    //*dword_C75DB8 = 0x0000; // Force upload texl textures

    return ((uint32_t(*)(uint8_t*,ff8_tim*))0x541740)(pointer, texture_infos);
}

uint32_t ff8_wm_open_texture4(uint8_t *pointer, ff8_tim *texture_infos)
{
    int *dword_C75DB8 = (int *)0xC75DB8;

    if (trace_all || trace_vram) ffnx_trace("%s C75DB8=%d\n", __func__, *dword_C75DB8);
    //*dword_C75DB8 = 0x0000; // Force upload texl textures

    return ((uint32_t(*)(uint8_t*,ff8_tim*))0x541740)(pointer, texture_infos);
}

// Used
uint32_t ff8_wm_open_texture5(uint8_t *pointer, ff8_tim *texture_infos)
{
    int *dword_C75DB8 = (int *)0xC75DB8;

    if (trace_all || trace_vram) ffnx_trace("%s C75DB8=%d\n", __func__, *dword_C75DB8);

    //*dword_C75DB8 = 0x0000; // Force upload texl textures

    return ((uint32_t(*)(uint8_t*,ff8_tim*))0x541740)(pointer, texture_infos);
}

// Used
uint32_t ff8_wm_open_texture6(uint8_t *pointer, ff8_tim *texture_infos)
{
    int *dword_C75DB8 = (int *)0xC75DB8;

    if (trace_all || trace_vram) ffnx_trace("%s C75DB8=%d\n", __func__, *dword_C75DB8);
    //*dword_C75DB8 = 0x0000; // Force upload texl textures

    return ((uint32_t(*)(uint8_t*,ff8_tim*))0x541740)(pointer, texture_infos);
}

uint32_t ff8_wm_open_texture7(uint8_t *pointer, ff8_tim *texture_infos)
{
    int *dword_C75DB8 = (int *)0xC75DB8;

    if (trace_all || trace_vram) ffnx_trace("%s C75DB8=%d\n", __func__, *dword_C75DB8);
    //*dword_C75DB8 = 0x0000; // Force upload texl textures

    return ((uint32_t(*)(uint8_t*,ff8_tim*))0x541740)(pointer, texture_infos);
}

void set_texture_page_enabled2_sub_464B50(int a1, int header_with_bit_depth, uint16_t pos_x6y9, int a4, int a5, int a6)
{
    ffnx_trace("%s a1=%d header_with_bit_depth=%d (depth=%d ???=%d page=%d x=%d y=%d) pos_x6y9=(%d, %d) a4=%d a5=%d a6=%d\n", __func__, a1,
        header_with_bit_depth, (((unsigned __int16)header_with_bit_depth >> 7) & 3), header_with_bit_depth & 0x60, header_with_bit_depth & 0x1F, (header_with_bit_depth & 0xF) << 6, 16 * (header_with_bit_depth & 0x10),
        pos_x6y9 & 0x3F, (pos_x6y9 & 0x7FFF) >> 6, a4, a5, a6);

    ((void(*)(int,int,uint16_t))0x464B50)(a1, header_with_bit_depth, pos_x6y9);
}

void crash(int a1, int a2, int a3, int a4, int a5)
{
    ffnx_trace("%s a1=%d a2=%d a3=%d a4=%d a5=%d", __func__, a1, a2, a3, a4, a5);
    *((int *)0xFFFFFFFFF) = 42;
}

/*
typedef void FuncParam;

int __cdecl sub_45D040(unsigned int a1)
{
    int v1 = 0;
    unsigned int *v2 = (unsigned int *)&unk_1CA8590;

    while (a1 < *(v2 - 1) || a1 > *v2)
    {
        v2 += 11;
        ++v1;

        if ((int)v2 >= (int)&unk_1CA8640)
        {
            return 0;
        }
    }

    return 44 * v1 + 0x1CA8568;
}

void __cdecl display_texture_related_2_sub_45D070(FuncParam *funct_param0)
{
    void (__cdecl *func1)(int8_t *); // eax
    void (__cdecl *func3)(int8_t *); // eax
    void (__cdecl *func0)(int8_t *); // eax
    void (__cdecl *func2)(int8_t *); // eax

    if ((dword_1CA8500 - 0x1C48500) / 24 < 0x4000)
    {
        dword_1CA8558 = 0;

        if (funct_param0 == (void *)-1)
        {
            funct_param0 = previous_funct_param_dword_1B474E0;
        }

        previous_funct_param_dword_1B474E0 = funct_param0;

        FuncParam *funct_param_cur = funct_param0;

        do
        {
            while (funct_param_cur < (FuncParam *)0x1C48500 || funct_param_cur >= (FuncParam *)0x1CA8500)
            {
                int32_t *sub_45D040_return_value = sub_45D040(funct_param_cur);

                if (sub_45D040_return_value == nullptr)
                {
                    return;
                }

                sub_45D040_return_value[1] = 1;

                funct_param_cur = *(FuncParam **)funct_param_cur;

                if (funct_param_cur == nullptr || (uint32_t(funct_param_cur) & 0xFFFFFF) == 0xFFFFFF)
                {
                    break 2;
                }
            }

            uint8_t *funct_param_cur4 = funct_param_cur;
            int32_t *funct_param_deref1 = *(int32_t **)funct_param_cur;
            uint8_t *funct_param_deref1_cur = *(int8_t **)funct_param_cur;
            max_size_dword_1CA8688 = funct_param_deref1_cur[1];

            /*
            funct_param_deref1
            u8 field0;
            u8 size;
            u8 field2;
            u8 field3;

            u8 field4;
            u8 field5;
            u8 field6;
            u8 func_id;
            */
/*
            while (max_size_dword_1CA8688 > 0)
            {
                int op_size = 1;
                int func_id = funct_param_deref1_cur[7];

                if (func_id != 0)
                {
                    if ((funct_param_cur4[20] & 1) != 0)
                    {
                        dword_B7CC00 = dword_B7CC0C;
                        dword_B7CC04 = dword_B7CC10;
                        dword_1CA86D4 = 1;
                        dword_B7CC1C = 16;
                    }
                    else
                    {
                        dword_B7CC00 = dword_B7CC14;
                        dword_B7CC04 = dword_B7CC18;
                        dword_1CA86D4 = 0;
                        dword_B7CC1C = 13;
                    }

                    if ((dword_1CA86E4 & 2) != 0)
                    {
                        func1 = (void (__cdecl *)(int8_t *))pointer_to_func_dword_B7DC18[func_id];

                        if (func1 != nullptr)
                        {
                            func1(funct_param_deref1_cur);
                        }
                    }

                    func3 = nullptr;

                    if (ssigpu_callback_enabled_byte_B7CD28[func_id])
                    {
                        if ((dword_1CA86E4 & 1) != 0)
                        {
                            if (dword_B7CC24 != 0)
                            {
                                func0 = (void (__cdecl *)(int8_t *))pointer_to_func2_dword_B7D708[func_id];

                                if (func0 != nullptr)
                                {
                                    func0(funct_param_deref1_cur);
                                }
                            }

                            if (dword_1CA8528 != 0)
                            {
                                func2 = (void (__cdecl *)(int8_t *))pointer_to_func_dword_B7DC18[func_id];

                                if (func2 != nullptr)
                                {
                                    func2(funct_param_deref1_cur);
                                }

                                // Do it twice?
                                if (dword_1CA8528 != 0)
                                {
                                    func3 = (void (__cdecl *)(int8_t *))pointer_to_func_dword_B7DC18[func_id];

                                    if (func3 != nullptr)
                                    {
                                        func3(funct_param_deref1_cur);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        func3 = (void (__cdecl *)(int8_t *))pointer_to_func2_dword_B7D708[func_id];

                        if (func3 != nullptr)
                        {
                            func3(funct_param_deref1_cur);
                        }
                    }

                    op_size = (uint8_t)ssigpu_op_size_byte_B7CC28[func_id];

                    if (op_size == 0)
                    {
                        op_size = 1;
                    }
                }

                max_size_dword_1CA8688 -= op_size;
                funct_param_deref1_cur += 4 * op_size;
            }

            if ((*funct_param_deref1 & 0xFFFFFF) == 0xFFFFFF)
            {
                break;
            }

            funct_param_cur = (void *)((*funct_param_deref1 & 0xFFFFFF) | (uint8_t(funct_param_cur[22]) << 24));
        }
        while (funct_param_cur != nullptr);

        if (dword_B7CC24 != 0)
        {
            ff8_read_buffer_parent_parent();
            dword_B7CC20 = 1;
            sub_460C10();
            sub_45D300(funct_param0, ssigpu_callbacks_dword_B7CF08);
            sub_460CD0();
            dword_B7CC20 = 0;
            sub_460C10();
            sub_45D300(funct_param0, pointer_to_func_off_B7D308);
            sub_460CD0();
            dword_B7CC20 = 1;
        }
    }

    dword_1CA8500 = 0x1C48500;

    if (dword_1CA867C != 0)
    {
        dword_1CA867C = 0;
    }

    dword_1CA8528 = 0;
    dword_1CA868C = 0;
    dword_1CA8684 = 0;
} */

void vram_init()
{
    texturePacker.setVram((uint8_t *)ff8_externals.psxvram_buffer);

    // pubintro
    replace_call(ff8_externals.open_lzs_image + 0x72, ff8_credits_open_texture);
    // cdcheck
    replace_call(ff8_externals.cdcheck_sub_52F9E0 + 0x1DC, ff8_cdcheck_error_upload_vram);
    // worldmap
    replace_call(0x53F0EC, ff8_wm_open_texture1); // Section 38
    replace_call(0x53F165, ff8_wm_open_pal1); // Section 38
    /* replace_call(0x5404E9, ff8_wm_open_texture2); // Section 38
    replace_call(0x540592, ff8_wm_open_texture3); // Section 39
    replace_call(0x540654, ff8_wm_open_texture4); // Section 40
    replace_call(0x54151D, ff8_wm_open_texture5); // Multi sections
    replace_call(0x5416AD, ff8_wm_open_texture6); // Section 17
    replace_call(0x541BBD, ff8_wm_open_texture7); // ???
    */
    // Triple Triad
    replace_call(0x538BA9, ff8_upload_vram_triple_triad_1);
    replace_call(0x538D2C, ff8_upload_vram_triple_triad_2);

    replace_function(ff8_externals.upload_psx_vram, ff8_upload_vram);

    // wm texl project
    //replace_call(0x5533C0 + 0x2EC, ff8_wm_texl_palette_upload_vram);
    //replace_call(0x5533C0 + 0x3F0, ff8_wm_texl_palette_upload_vram2);

    //replace_call(0x559889, ff8_wm_draw_frame);
    //replace_call(ff8_externals.sub_45D080 + 0x208, ff8_read_buffer_parent_parent);
    //replace_function(0x466EE0, ff8_unknown_read_vram);

    // replace_call(0x466180 + 0x2C, ff8_read_vram_1);

    //replace_call(0x462FB7, set_texture_page_enabled2_sub_464B50);
    //replace_function(0x464B6E, crash);

    /* replace_call(0x4610C1, ff8_tx_select_call1);
    replace_call(0x461A4C, ff8_tx_select_call2);
    replace_call(0x462E13, ff8_tx_select_call3);
    replace_call(0x465CB8, ff8_tx_select_call4); */


    // read_vram_to_buffer_parent_calls
    replace_call(ff8_externals.sub_464BD0 + 0x53, read_vram_to_buffer_parent_call1);
    replace_call(ff8_externals.sub_464BD0 + 0xED, read_vram_to_buffer_parent_call1);
    replace_call(ff8_externals.sub_464BD0 + 0x1A1, read_vram_to_buffer_parent_call1);
    replace_call(ff8_externals.ssigpu_tx_select_2_sub_465CE0 + 0x281, read_vram_to_buffer_parent_call1);

    replace_call(ff8_externals.sub_464BD0 + 0x79, read_vram_to_buffer_parent_call2);
    replace_call(ff8_externals.sub_464BD0 + 0x1B5, read_vram_to_buffer_parent_call2);

    // read_vram_to_buffer_with_palette1_parent_calls
    replace_call(ff8_externals.sub_464BD0 + 0xFC, read_vram_to_buffer_with_palette1_parent_call1);
    replace_call(ff8_externals.ssigpu_tx_select_2_sub_465CE0 + 0x2CF, read_vram_to_buffer_with_palette1_parent_call1);

    replace_call(ff8_externals.sub_464BD0 + 0xAF, read_vram_to_buffer_with_palette1_parent_call2);

    replace_call(uint32_t(ff8_externals.sub_464F70) + 0x2C5, read_vram_to_buffer);
    replace_call(ff8_externals.sub_4653B0 + 0x9D, read_vram_to_buffer);

    replace_call(ff8_externals.sub_464DB0 + 0xEC, read_vram_to_buffer_with_palette1);
    replace_call(ff8_externals.sub_465720 + 0xA5, read_vram_to_buffer_with_palette1);

    // Not used?
    replace_call(ff8_externals.sub_4649A0 + 0x13F, read_vram_to_buffer_with_palette2);
}
