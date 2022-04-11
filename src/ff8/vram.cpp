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
uint8_t next_bpp = 2;
uint8_t next_scale = 1;
int8_t texl_id_left = -1;
int8_t texl_id_right = -1;

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
}

int read_vram_to_buffer_parent_call1(int a1, int structure, int x, int y, int w, int h, int bpp, int rel_pos, int a9, uint8_t *target)
{
    if (trace_all || trace_vram) ffnx_trace("%s: x=%d y=%d w=%d h=%d bpp=%d rel_pos=(%d, %d) a9=%d target=%X\n", __func__, x, y, w, h, bpp, rel_pos & 0xF, rel_pos >> 4, a9, target);

    next_psxvram_x = (x >> (2 - bpp)) + ((rel_pos & 0xF) << 6);
    next_psxvram_y = y + (((rel_pos >> 4) & 1) << 8);
    char name[MAX_PATH];

    int ret = ff8_externals.sub_464F70(a1, structure, x, y, w, h, bpp, rel_pos, a9, target);

    next_psxvram_x = -1;
    next_psxvram_y = -1;

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

    ff8_externals.read_vram_1(vram, vram_w_2048, target, target_w, w, h, bpp);
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

void vram_init()
{
    texturePacker.setVram((uint8_t *)ff8_externals.psxvram_buffer);

    // pubintro
    replace_call(ff8_externals.open_lzs_image + 0x72, ff8_credits_open_texture);
    // cdcheck
    replace_call(ff8_externals.cdcheck_sub_52F9E0 + 0x1DC, ff8_cdcheck_error_upload_vram);
    // Triple Triad
    replace_call(0x538BA9, ff8_upload_vram_triple_triad_1);
    replace_call(0x538D2C, ff8_upload_vram_triple_triad_2);

    replace_function(ff8_externals.upload_psx_vram, ff8_upload_vram);

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
