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
    struc_50 *struct_50 = (struc_50 *)0x1CB5D18;

    for (int i = 0; i < 32; ++i)
    {
        const struc_50 &current = struct_50[i];

        if (current.vram_page_enabled || current.initialized)
        {
            ffnx_info(
                "texture %d\n"
                "initialized = %X " BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN "\n"
                "vram_page_enabled = %X " BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN "\n"
                "field_328 = %X\n"
                "vram_needs_reload = %d\n"
                "field_330 = %X\n"
                "vram_x = %d\n"
                "vram_y = %d\n"
                "vram_width = %d\n"
                "vram_height = %d\n"
                "vram_palette_data = %p\n"
                "vram_palette_pos = %X\n"
                "==\n",
                i,
                current.initialized, BYTE_TO_BINARY(current.initialized >> 24), BYTE_TO_BINARY(current.initialized >> 16), BYTE_TO_BINARY(current.initialized >> 8), BYTE_TO_BINARY(current.initialized >> 0),
                current.vram_page_enabled, BYTE_TO_BINARY(current.vram_page_enabled >> 24), BYTE_TO_BINARY(current.vram_page_enabled >> 16), BYTE_TO_BINARY(current.vram_page_enabled >> 8), BYTE_TO_BINARY(current.vram_page_enabled >> 0),
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

            /* for (int j = 0; j < 8; ++j)
            {
                const texture_page &tp = current.texture_page[j];

                ffnx_info(
                    "page %d\n"
                    "field_0 %X\n"
                    "x = %d\n"
                    "y = %d\n"
                    "width = %d\n"
                    "height = %d\n"
                    "color_key = %d\n"
                    "u = %f\n"
                    "v = %f\n"
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
            } */
        }
    }
}

int ff8_upload_vram(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    const int x = pos_and_size[0];
    const int y = pos_and_size[1];
    const int w = pos_and_size[2];
    const int h = pos_and_size[3];
    bool isPal = next_pal_data != nullptr && (uint8_t *)next_pal_data == texture_buffer;

    if (trace_all || trace_vram) ffnx_trace("%s x=%d y=%d w=%d h=%d bpp=%d isPal=%d\n", __func__, x, y, w, h, next_bpp, isPal);

    print_struct_50();

    texturePacker.setTexture(isPal ? last_texture_name : next_texture_name, texture_buffer, x, y, w, h, next_bpp, isPal);

    ff8_externals.sub_464850(x, y, x + w - 1, h + y - 1);

    strncpy(last_texture_name, next_texture_name, MAX_PATH);
    *next_texture_name = '\0';

    if (trace_all || trace_vram) ffnx_trace("%s after\n", __func__);

    print_struct_50();

    return 1;
}

int ff8_wm_texl_palette_upload_vram(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    int *dword_C75DB8 = (int *)0xC75DB8;
    int texl_id = (*dword_C75DB8 >> 8) & 0xFF;

    if (trace_all || trace_vram) ffnx_trace("%s texl_id=%d\n", __func__, texl_id);

    return ff8_upload_vram(pos_and_size, texture_buffer);
}

int ff8_wm_texl_palette_upload_vram2(int16_t *pos_and_size, uint8_t *texture_buffer)
{
    int *dword_C75DB8 = (int *)0xC75DB8;
    int texl_id = (*dword_C75DB8 >> 8) & 0xFF;

    if (trace_all || trace_vram) ffnx_trace("%s texl_id=%d\n", __func__, texl_id);

    return ff8_upload_vram(pos_and_size, texture_buffer);
}

int ff8_wm_draw_frame(void *a1)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    return ((int(*)(void*))ff8_externals.sub_45D080)(a1);
}

void ff8_read_buffer_parent_parent()
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    //print_struct_50();

    ((void(*)())ff8_externals.sub_464BD0)();

    ffnx_info("%s after\n", __func__);
    //print_struct_50();
}

void ff8_unknown_read_vram(void *a1)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);
}

void ff8_read_vram_1(int CLUT, int *target, int size)
{
    if (trace_all || trace_vram) ffnx_trace("%s CLUT=%d size=%d\n", __func__, CLUT, size);


}

int ff8_tx_select_call1(unsigned int a1, unsigned int header_with_bit_depth, int16_t CLUT_pos_x6y9, DWORD *a4, int *a5, DWORD *a6)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    return ((int(*)(unsigned int a1, unsigned int, int16_t, DWORD *, int *, DWORD *))ff8_externals.ssigpu_tx_select_2_sub_465CE0)(a1, header_with_bit_depth, CLUT_pos_x6y9, a4, a5, a6);
}

int ff8_tx_select_call2(unsigned int a1, unsigned int header_with_bit_depth, int16_t CLUT_pos_x6y9, DWORD *a4, int *a5, DWORD *a6)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    return ((int(*)(unsigned int a1, unsigned int, int16_t, DWORD *, int *, DWORD *))ff8_externals.ssigpu_tx_select_2_sub_465CE0)(a1, header_with_bit_depth, CLUT_pos_x6y9, a4, a5, a6);
}

int ff8_tx_select_call3(unsigned int a1, unsigned int header_with_bit_depth, int16_t CLUT_pos_x6y9, DWORD *a4, int *a5, DWORD *a6)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    return ((int(*)(unsigned int a1, unsigned int, int16_t, DWORD *, int *, DWORD *))ff8_externals.ssigpu_tx_select_2_sub_465CE0)(a1, header_with_bit_depth, CLUT_pos_x6y9, a4, a5, a6);
}

int ff8_tx_select_call4(unsigned int a1, unsigned int header_with_bit_depth, int16_t CLUT_pos_x6y9, DWORD *a4, int *a5, DWORD *a6)
{
    if (trace_all || trace_vram) ffnx_trace("%s\n", __func__);

    return ((int(*)(unsigned int a1, unsigned int, int16_t, DWORD *, int *, DWORD *))ff8_externals.ssigpu_tx_select_2_sub_465CE0)(a1, header_with_bit_depth, CLUT_pos_x6y9, a4, a5, a6);
}

int read_vram_to_buffer_parent_call1(int a1, int structure, int x, int y, int w, int h, int bpp, int rel_pos, int a9, uint8_t *target)
{
    if (trace_all || trace_vram) ffnx_trace("%s: x=%d y=%d w=%d h=%d bpp=%d rel_pos=(%d, %d) a9=%d target=%X\n", __func__, x, y, w, h, bpp, rel_pos & 0xF, rel_pos >> 4, a9, target);

    next_psxvram_x = (x >> (2 - bpp)) + ((rel_pos & 0xF) << 6);
    next_psxvram_y = y + (((rel_pos >> 4) & 1) << 8);

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

void set_vram_enabled2_sub_464B50(int a1, int header_with_bit_depth, uint16_t pos_x6y9, int a4, int a5, int a6)
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

void vram_init()
{
    texturePacker.setVram((uint8_t *)ff8_externals.psxvram_buffer);

    // pubintro
    replace_call(ff8_externals.open_lzs_image + 0x72, ff8_credits_open_texture);
    // cdcheck
    replace_call(ff8_externals.cdcheck_sub_52F9E0 + 0x1DC, ff8_cdcheck_error_upload_vram);
    // worldmap
    replace_call(0x53F0EC, ff8_wm_open_texture1); // Section 38
    /* replace_call(0x5404E9, ff8_wm_open_texture2); // Section 38
    replace_call(0x540592, ff8_wm_open_texture3); // Section 39
    replace_call(0x540654, ff8_wm_open_texture4); // Section 40
    replace_call(0x54151D, ff8_wm_open_texture5); // Multi sections
    replace_call(0x5416AD, ff8_wm_open_texture6); // Section 17
    replace_call(0x541BBD, ff8_wm_open_texture7); // ???
    */

    replace_function(ff8_externals.upload_psx_vram, ff8_upload_vram);

    // wm texl project
    replace_call(0x5533C0 + 0x2EC, ff8_wm_texl_palette_upload_vram);
    replace_call(0x5533C0 + 0x3F0, ff8_wm_texl_palette_upload_vram2);

    replace_call(0x559889, ff8_wm_draw_frame);
    replace_call(ff8_externals.sub_45D080 + 0x208, ff8_read_buffer_parent_parent);
    replace_function(0x466EE0, ff8_unknown_read_vram);

    // replace_call(0x466180 + 0x2C, ff8_read_vram_1);

    replace_call(0x462FB7, set_vram_enabled2_sub_464B50);
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
