/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2025 myst6re                                            //
//    Copyright (C) 2025 Julian Xhokaxhiu                                   //
//    Copyright (C) 2023 Tang-Tang Zhou                                     //
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

#include <stdint.h>
#include "./file.h"
#include "../globals.h"
#include "../patch.h"
#include "../log.h"

bool fonts_initialized = false;
ff8_font *fonts_fieldtdw_even = nullptr;
ff8_font *fonts_fieldtdw_odd = nullptr;
ff8_font *fonts_sysevn = nullptr;
ff8_font *fonts_sysodd = nullptr;
ff8_graphics_object *graphic_object_font8_even = nullptr;
ff8_graphics_object *graphic_object_font8_odd = nullptr;
uint8_t font_character_width_local_field[452];

static constexpr uint8_t ff8_remastered_font_alignment_data[] = {
    0x85, 0x86, 0x88, 0x88, 0x88, 0xB8, 0x47, 0x74, 0x7A, 0x77, 0x89, 0x77, 0x55, 0x44, 0x84, 0x77,
    0x74, 0x47, 0x89, 0x87, 0x68, 0x86, 0x38, 0x76, 0x96, 0x87, 0x86, 0x66, 0x88, 0x98, 0x88, 0x68,
    0x66, 0x66, 0x65, 0x36, 0x54, 0x93, 0x66, 0x66, 0x54, 0x64, 0x96, 0x66, 0x76, 0x66, 0x77, 0x55,
    0x55, 0x34, 0x44, 0x76, 0x77, 0x67, 0x66, 0xA6, 0x66, 0x66, 0x66, 0x66, 0x66, 0x34, 0x44, 0x66,
    0x66, 0x66, 0x66, 0xA6, 0x5D, 0x95, 0x99, 0x66, 0xA9, 0x77, 0x49, 0x9A, 0xA7, 0x74, 0x35, 0xD7,
    0x88, 0x97, 0x74, 0x79, 0x93, 0xAA, 0x89, 0x8E, 0x8C, 0x8A, 0x88, 0x88, 0x8A, 0x8F, 0x88, 0x8C,
    0xC8, 0x09, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00,
    0x00, 0x00, 0xE0, 0x01, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x52, 0xCA, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xEF, 0xBD, 0xAD, 0xB5, 0x4A, 0xA9, 0x08, 0xA1, 0x00, 0x00, 0xE7, 0x9C, 0xFF, 0x83, 0xFF, 0x83,
    0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83,
    0x4A, 0xA9, 0xAD, 0xB5, 0x10, 0xC2, 0x94, 0xD2, 0x00, 0x00, 0xA5, 0x94, 0xFF, 0x83, 0xFF, 0x83,
    0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83,
    0xA5, 0x94, 0x31, 0x86, 0xD6, 0x86, 0x7B, 0x87, 0x00, 0x00, 0xA5, 0x94, 0xFF, 0x83, 0xFF, 0x83,
    0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83,
    0xA9, 0x94, 0x73, 0x8C, 0x5A, 0x88, 0x1D, 0x80, 0x00, 0x00, 0xA5, 0x94, 0xFF, 0x83, 0xFF, 0x83,
    0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83,
    0xE4, 0x90, 0x22, 0x8A, 0xC2, 0x8A, 0xA0, 0x83, 0x00, 0x00, 0xA5, 0x94, 0xFF, 0x83, 0xFF, 0x83,
    0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83,
    0x06, 0xA1, 0xE8, 0xD9, 0x2A, 0xE2, 0xCD, 0xF6, 0x00, 0x00, 0xC6, 0x98, 0xFF, 0x83, 0xFF, 0x83,
    0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83,
    0x88, 0xA0, 0x92, 0xC8, 0x58, 0xE0, 0x1D, 0xF4, 0x00, 0x00, 0xE7, 0x9C, 0xFF, 0x83, 0xFF, 0x83,
    0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83, 0xFF, 0x83,
    0x4A, 0xA9, 0x10, 0x42, 0xB5, 0x56, 0x9C, 0x73, 0x0C, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static constexpr uint8_t ff8_remastered_font_jp_alignment_data[] = {
    0xBC, 0xCB, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xBC, 0xBC, 0xBB, 0xCC, 0x9B, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xBC, 0xB9, 0xCB, 0xBC, 0xCC, 0xBC, 0xCC, 0x9C, 0x86, 0x98, 0x88, 0x99, 0x58, 0x86,
    0xBB, 0xC9, 0xCA, 0xCB, 0xBB, 0xCA, 0xBB, 0x9A, 0xBB, 0xAA, 0xBB, 0x9B, 0xBB, 0xBB, 0xBA, 0xBB,
    0xBB, 0xBB, 0xBB, 0xA8, 0x9A, 0xBB, 0xBA, 0xBB, 0xCB, 0xCB, 0xBB, 0xCA, 0xCB, 0xBA, 0xBB, 0xCA,
    0xBB, 0xBB, 0xBB, 0xAB, 0x98, 0xBC, 0xCA, 0xAA, 0xBB, 0xBB, 0xBA, 0xCA, 0xCA, 0xBA, 0xA9, 0xAA,
    0x99, 0x99, 0x99, 0x99, 0x89, 0xAA, 0xAA, 0x9A, 0x99, 0x88, 0x9A, 0x76, 0x79, 0x9B, 0x9A, 0x9A,
    0x99, 0xA9, 0xAD, 0x9A, 0x95, 0x9B, 0x99, 0xC9, 0xAC, 0x55, 0x55, 0xCC, 0xCC, 0x56, 0x55, 0xBA,
    0xC8, 0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB,
    0xCC, 0xCC, 0xCC, 0xBC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCA, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCA, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCA, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB,
    0xBC, 0xCC, 0xCC, 0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB,
    0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xBC, 0xCC, 0xCB, 0xCC,
    0xBC, 0xBC, 0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xBC, 0xBC, 0xCC, 0xAC,
    0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCC, 0xBC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0x8C, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x66, 0xCC, 0xCC, 0xBC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xBC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xAC, 0xCC, 0xCC, 0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC,
    0xAC, 0xCC, 0xCC, 0xCC, 0xBC, 0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC,
    0xCC, 0xBC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xBC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x09,
};

static_assert(sizeof(ff8_remastered_font_alignment_data) == 0x1B9);
static_assert(sizeof(ff8_remastered_font_jp_alignment_data) == 0x1B9);

static uint32_t ff8_load_fonts_hardcoded_tdw(uint32_t font_id, uint32_t unknown_arg2, uint32_t unknown_arg3)
{
    const auto load_fonts = reinterpret_cast<uint32_t (*)(uint32_t, uint32_t, uint32_t)>(ff8_externals.load_fonts);
    const uint32_t result = load_fonts(font_id, unknown_arg2, unknown_arg3);

    if (font_id == 0 && ff8_is_remastered_font_asset())
    {
        uint8_t *font_alignment_data = reinterpret_cast<uint8_t *>(ff8_externals.dword_1D2B808) + 0x10;
        memcpy(font_alignment_data, JP_VERSION ? ff8_remastered_font_jp_alignment_data : ff8_remastered_font_alignment_data, sizeof(JP_VERSION ? ff8_remastered_font_jp_alignment_data : ff8_remastered_font_alignment_data));
    }

    return result;
}

static uint32_t ff8_get_character_width_hardcoded_tdw(uint32_t character_id)
{
    if (character_id == 173) return 9;
    if (character_id == 174) return 10;

    const uint8_t *font_alignment_data = ff8_is_remastered_font_asset()
        ? (JP_VERSION ? ff8_remastered_font_jp_alignment_data : ff8_remastered_font_alignment_data)
        : reinterpret_cast<uint8_t *>(ff8_externals.dword_1D2B808) + 0x10;
    const uint8_t packed_widths = font_alignment_data[character_id >> 1];
    return ((character_id & 1) != 0 ? packed_widths >> 4 : packed_widths) & 0xF;
}

ff8_font *malloc_ff8_font_structure()
{
    ff8_font *font = (ff8_font *)external_malloc(sizeof(ff8_font));
    font->graphics_object48 = nullptr;
    font->graphics_object4C = nullptr;
    font->graphics_object50 = nullptr;
    font->graphics_object54 = nullptr;

    return font;
}

ff8_graphics_object *ff8_create_font_graphic_object(const char *path, ff8_create_graphic_object *create_graphics_object_infos, bool isTmp = false)
{
    char buffer[MAX_PATH] = {};

    if (!isTmp) {
        if (create_graphics_object_infos->file_container != nullptr) {
            sprintf(buffer, "c:%s%s", ff8_externals.archive_path_prefix_menu, path);
        } else {
            sprintf(buffer, "%s%s", ff8_externals.archive_path_prefix_menu, path);
        }
    } else {
        strcpy(buffer, path);
    }

    return ((ff8_graphics_object*(*)(int,int,ff8_create_graphic_object*,char*,void*))(ff8_externals._load_texture))(1, 12, create_graphics_object_infos, buffer, *ff8_externals.dword_1D2A284);
}

void free_font_graphics_object(ff8_font *font)
{
    if (font->graphics_object48 != nullptr) {
        ff8_externals.free_graphics_object(font->graphics_object48);
        font->graphics_object48 = nullptr;
    }
    if (font->graphics_object4C != nullptr) {
        ff8_externals.free_graphics_object(font->graphics_object4C);
        font->graphics_object4C = nullptr;
    }
    if (font->graphics_object50 != nullptr) {
        ff8_externals.free_graphics_object(font->graphics_object50);
        font->graphics_object50 = nullptr;
    }
    if (font->graphics_object54 != nullptr) {
        ff8_externals.free_graphics_object(font->graphics_object54);
        font->graphics_object54 = nullptr;
    }
}

void fill_font_structure(ff8_font *font, int width, int height, int ratio)
{
    if (font->graphics_object48 == nullptr) {
        return;
    }

    font->field_4 = float(width * ratio);
    font->field_8 = float(height * ratio);
    font->field_38 = font->field_34;
    font->field_3D = 0;

    font->field_0 = 0;
    font->field_14 = 4;
    font->field_3E = ((ff8_tex_header *)(((ff8_texture_set *)(font->graphics_object48->hundred_data->texture_set))->tex_header))->field_DC;
    font->field_40 = ((ff8_tex_header *)(((ff8_texture_set *)(font->graphics_object48->hundred_data->texture_set))->tex_header))->field_E0;
    font->field_C = 1.0 / font->field_4;
    font->field_10 = 1.0 / font->field_8;
    font->field_18 = float(width);
    font->field_1C = float(height);
    font->field_20 = 1.0 / font->field_18;
    font->field_24 = 1.0 / font->field_1C;
    font->field_28 = font->field_4 / font->field_18;
    font->field_2C = font->field_8 / font->field_1C;
}

void create_graphics_object_info_structure_for_font(ff8_create_graphic_object *create_graphics_object_infos)
{
    ff8_externals.create_graphics_object_info_structure(4, create_graphics_object_infos);
    create_graphics_object_infos->field_7C |= 0x80u;
    create_graphics_object_infos->flags |= 0x11u;
    create_graphics_object_infos->field_18 = *ff8_externals.dword_1D2A288;
}

void ff8_load_fonts_field(char *tdw_tim_data, char *path)
{
    ffnx_trace("%s %s\n", __func__, path);
    size_t element_count;
    ff8_create_graphic_object create_graphics_object_infos;
    int *tim = (int *)tdw_tim_data;
    char *tim_img_header;

    if ((tdw_tim_data[4] & 8) != 0) { // paletted
        tim_img_header = &tdw_tim_data[*((DWORD *)tdw_tim_data + 2) + 8];
    } else {
        tim_img_header = tdw_tim_data + 8;
    }
    int16_t width = *((int16_t *)tim_img_header + 4), height = *((int16_t *)tim_img_header + 5);

    if (*(DWORD *)tim_img_header == 12) {
        return;
    }

    create_graphics_object_info_structure_for_font(&create_graphics_object_infos);
    create_graphics_object_infos.field_8 = 1;

    if (fonts_fieldtdw_even == nullptr) {
        fonts_fieldtdw_even = malloc_ff8_font_structure();
    }
    free_font_graphics_object(fonts_fieldtdw_even);
    if (fonts_fieldtdw_odd == nullptr) {
        fonts_fieldtdw_odd = malloc_ff8_font_structure();
    }
    free_font_graphics_object(fonts_fieldtdw_odd);
    bool use_low_res = true;

    if (*ff8_externals.config_highres_font_multiplier == 2 && *ff8_externals.config_use_highres_font) { // high res
        ff8_file_container *file_container = ff8_externals.get_file_container_sub_51B410("\\MENU\\");
        // Remove extension
        path[strlen(path) - 4] = '\0';
        // Get filename
        const char *field_name = strrchr(path, '\\');
        if (field_name == nullptr) {
            field_name = path;
        } else {
            field_name += 1;
        }
        char filename[MAX_PATH] = {};
        snprintf(filename, sizeof(filename), "%shires\\fieldtdw\\%s00.dat", ff8_externals.archive_path_prefix_menu, field_name);
        void *buffer = nullptr;
        ff8_externals.open_file_menu_sub_4B9530(&buffer, filename);
        if (buffer) {
            void *buffer2 = nullptr;
            ff8_externals.tdw_malloc_sub_4B98F0((int *)buffer, (int **)&buffer2, &element_count);
            ff8_externals.write_tdw_tmp_sub_4B9640(buffer, "temp_evn.tim", element_count);
            ff8_externals.write_tdw_tmp_sub_4B9640(buffer2, "temp_odd.tim", element_count);
            external_free(buffer);
            external_free(buffer2);
            buffer = nullptr;
            fonts_fieldtdw_even->graphics_object48 = ff8_create_font_graphic_object("temp_evn.tim", &create_graphics_object_infos, true);
            fonts_fieldtdw_odd->graphics_object48 = ff8_create_font_graphic_object("temp_odd.tim", &create_graphics_object_infos, true);
            use_low_res = false;
        }
        snprintf(filename, sizeof(filename), "%shires\\fieldtdw\\%s01.dat", ff8_externals.archive_path_prefix_menu, field_name);
        ff8_externals.open_file_menu_sub_4B9530(&buffer, filename);
        if (buffer) {
            void *buffer2 = nullptr;
            ff8_externals.tdw_malloc_sub_4B98F0((int *)buffer, (int **)&buffer2, &element_count);
            ff8_externals.write_tdw_tmp_sub_4B9640(buffer, "temp_evn1.tim", element_count);
            ff8_externals.write_tdw_tmp_sub_4B9640(buffer2, "temp_odd1.tim", element_count);
            external_free(buffer);
            external_free(buffer2);
            buffer = nullptr;
            fonts_fieldtdw_even->graphics_object4C = ff8_create_font_graphic_object("temp_evn1.tim", &create_graphics_object_infos, true);
            fonts_fieldtdw_odd->graphics_object4C = ff8_create_font_graphic_object("temp_odd1.tim", &create_graphics_object_infos, true);
        }
        ff8_externals.free_file_container(file_container);
        fonts_fieldtdw_even->field_1 = 1;
        fonts_fieldtdw_even->field_30 = 2;
        fonts_fieldtdw_even->field_34 = 1;
        fonts_fieldtdw_even->field_3C = 0;
        fonts_fieldtdw_odd->field_1 = 1;
        fonts_fieldtdw_odd->field_30 = 2;
        fonts_fieldtdw_odd->field_34 = 1;
        fonts_fieldtdw_odd->field_3C = 0;
    }

    if (use_low_res) {
        void *buffer2 = nullptr;
        ff8_externals.tdw_malloc_sub_4B98F0(tim, (int **)&buffer2, &element_count);
        ff8_externals.write_tdw_tmp_sub_4B9640(tim, "temp_evn.tim", element_count);
        ff8_externals.write_tdw_tmp_sub_4B9640(buffer2, "temp_odd.tim", element_count);
        external_free(buffer2);

        fonts_fieldtdw_even->graphics_object48 = ff8_create_font_graphic_object("temp_evn.tim", &create_graphics_object_infos, true);
        fonts_fieldtdw_even->field_1 = 0;
        fonts_fieldtdw_even->field_30 = 1;
        fonts_fieldtdw_even->field_34 = 1;
        fonts_fieldtdw_even->field_3C = 0;
        fonts_fieldtdw_odd->graphics_object48 = ff8_create_font_graphic_object("temp_odd.tim", &create_graphics_object_infos, true);
        fonts_fieldtdw_odd->field_1 = 0;
        fonts_fieldtdw_odd->field_30 = 1;
        fonts_fieldtdw_odd->field_34 = 1;
        fonts_fieldtdw_odd->field_3C = 0;
    }

    fill_font_structure(fonts_fieldtdw_even, width, height, fonts_fieldtdw_even->field_30);
    fill_font_structure(fonts_fieldtdw_odd, width, height, fonts_fieldtdw_odd->field_30);
}

void reset_graphics_object_field_58(ff8_graphics_object *graphic_object)
{
    if (graphic_object != nullptr) graphic_object->field_58 = 0;
}

void reset_font_graphics_object_field_58(ff8_font *font)
{
    reset_graphics_object_field_58(font->graphics_object48);
    reset_graphics_object_field_58(font->graphics_object4C);
    reset_graphics_object_field_58(font->graphics_object50);
    reset_graphics_object_field_58(font->graphics_object54);
}

void ff8_fonts_jp_rendering_reset_field_58()
{
    ((void(*)())ff8_externals.sub_4B3710)();

    reset_font_graphics_object_field_58(fonts_fieldtdw_even);
    reset_font_graphics_object_field_58(fonts_fieldtdw_odd);

    reset_font_graphics_object_field_58(fonts_sysevn);
    reset_font_graphics_object_field_58(fonts_sysodd);

    reset_graphics_object_field_58(graphic_object_font8_even);
    reset_graphics_object_field_58(graphic_object_font8_odd);
}

void draw_graphics_object(ff8_graphics_object *graphic_object, game_obj *game_object)
{
    if (graphic_object != nullptr) ((void(*)(ff8_graphics_object*,game_obj*))ff8_externals.graphics_setrendererstate_draw_sub_4178D7)(graphic_object, game_object);
}

void draw_font(ff8_font *font, game_obj *game_object)
{
    draw_graphics_object(font->graphics_object48, game_object);
    draw_graphics_object(font->graphics_object4C, game_object);
    draw_graphics_object(font->graphics_object50, game_object);
    draw_graphics_object(font->graphics_object54, game_object);
}

void ff8_fonts_jp_draw()
{
    game_obj *game_object = common_externals.get_game_object();

    draw_font(fonts_fieldtdw_even, game_object);
    draw_font(fonts_fieldtdw_odd, game_object);

    draw_font(fonts_sysevn, game_object);
    draw_font(fonts_sysodd, game_object);

    draw_graphics_object(graphic_object_font8_even, game_object);
    draw_graphics_object(graphic_object_font8_odd, game_object);

    ((void(*)())ff8_externals.sub_4B3690)();
}

uint8_t *pointer_to_iterate_to = nullptr;
int last_x = -1;

void ff8_fonts_jp_render_kernel_menus_before_loops(uint32_t a1, uint8_t *a2, int8_t a3)
{
    ffnx_trace("%s\n", __func__);

    ((void(*)(uint32_t,uint8_t*,int8_t))ff8_externals.sub_4B87A0)(a1, a2, a3);

    for (int i = 0; i < 9; ++i) {
        if (*a2 != 1) {
            break;
        }
        ++a2;
    }

    pointer_to_iterate_to = a2;
}

int call_draw_icon_sub_4B7210(int *a1, int a2, int x, uint16_t y, uint16_t a5, int a6)
{
    ffnx_trace("%s: xy=(%d, %d)\n", __func__, x, y);
    last_x = x - 7 + 154;

    uint8_t *pointer_to_iterate_to2 = pointer_to_iterate_to;
    for (;;) {
        uint8_t v22 = *pointer_to_iterate_to2++;
        if (!v22) {
            break;
        }
        last_x -= ff8_externals.kernel_bin_sysfont[v22].x_field_0 & 0xF;
    }
    ffnx_trace("%s: last_x=%d\n", __func__, last_x);

    return ((int(*)(int*,int,int,uint16_t,uint16_t,int))ff8_externals.ff8_draw_icon_or_key1)(a1, a2, x, y, a5, a6);
}

void ff8_fonts_jp_render_kernel_menus_after_loops(int *a1, void *a2)
{
    ffnx_trace("%s\n", __func__);
    last_x = -1;
}

void *load_save_render_entry_icon(int *a1, void *draw_infos, int icon_id, uint16_t x, uint16_t y, int a6)
{
    return draw_infos;
}

void *load_save_render_entry_icon2(int *a1, void *draw_infos, uint16_t x, int y, char *a5, int a6)
{
    char *text = ((char*(*)(int,int,int,int))0x4BD630)(1, 5, 30, 0); // Get text

    ffnx_trace("%s: %s\n", __func__, text);

    char text2[256] = {};

    strncpy(text2, text, sizeof(text2));

    char *v10 = &text2[strlen(text2)];
    v10[0] = a5[0];
    v10[1] = a5[1];

    return ((void*(*)(int*,void*,uint16_t,int,char*,int))0x49F850)(a1, draw_infos, x + 196 - 228, y, text2, a6);
}

void jp_fonts_with_font8c(ff8_draw_menu_sprite_texture_infos_short *texture_infos_short)
{
    int v4 = (texture_infos_short->palID >> 6) - fonts_sysevn->field_40;

    texture_infos_short->palID = (fonts_sysevn->field_3E >> 4) & 0x3F | ((fonts_sysevn->field_40 + uint16_t(v4 / 2)) << 6);
    ff8_font *fonts = (v4 & 1) != 0 ? fonts_sysodd : fonts_sysevn;

    // font8 support
    /* if (16 * (texture_infos_short->palID & 0x3F) != fonts->field_3E
            && texture_infos_short->u >= 128u && texture_infos_short->v >= 152u && texture_infos_short->v < 200u) {
        ff8_graphics_object *graphic_object = (v4 & 1) != 0 ? graphic_object_font8_odd : graphic_object_font8_even;
        if (graphic_object != nullptr) {
            texture_infos_short->u += 128; // u >= 256
            texture_infos_short->v += 104; // v >= 256 && v < 304
            texture_infos_short->palID = (((ff8_tex_header *)(((ff8_texture_set *)(graphic_object->hundred_data->texture_set))->tex_header))->field_DC >> 4) & 0x3F | ((LOWORD(((ff8_tex_header *)(((ff8_texture_set *)(graphic_object->hundred_data->texture_set))->tex_header))->field_E0) + uint16_t(v4 / 2)) << 6);

            return ((void(*)(ff8_draw_menu_sprite_texture_infos_short*,ff8_graphics_object*))0x49B300)(texture_infos_short, graphic_object); // TODO
        }
    } */

    ((void(*)(ff8_draw_menu_sprite_texture_infos_short*,ff8_font*))ff8_externals.sub_49D6F0)(texture_infos_short, fonts);

    if (fonts->graphics_object48 != nullptr && fonts->graphics_object48->vertices != nullptr) {
        ffnx_info("%s: 48 (%lf, %lf, u=%lf, v=%lf) (%lf, %lf, u=%lf, v=%lf) (%lf, %lf, u=%lf, v=%lf)\n", __func__,
            fonts->graphics_object48->vertices[0].x, fonts->graphics_object48->vertices[0].y,
            fonts->graphics_object48->vertices[0].u, fonts->graphics_object48->vertices[0].v,
            fonts->graphics_object48->vertices[1].x, fonts->graphics_object48->vertices[1].y,
            fonts->graphics_object48->vertices[1].u, fonts->graphics_object48->vertices[1].v,
            fonts->graphics_object48->vertices[2].x, fonts->graphics_object48->vertices[2].y,
            fonts->graphics_object48->vertices[2].u, fonts->graphics_object48->vertices[2].v
        );
    }

    if (fonts->graphics_object4C != nullptr && fonts->graphics_object4C->vertices != nullptr) {
        ffnx_info("%s: 4C (%lf, %lf, u=%lf, v=%lf) (%lf, %lf, u=%lf, v=%lf) (%lf, %lf, u=%lf, v=%lf)\n", __func__,
            fonts->graphics_object4C->vertices[0].x, fonts->graphics_object4C->vertices[0].y,
            fonts->graphics_object4C->vertices[0].u, fonts->graphics_object4C->vertices[0].v,
            fonts->graphics_object4C->vertices[1].x, fonts->graphics_object4C->vertices[1].y,
            fonts->graphics_object4C->vertices[1].u, fonts->graphics_object4C->vertices[1].v,
            fonts->graphics_object4C->vertices[2].x, fonts->graphics_object4C->vertices[2].y,
            fonts->graphics_object4C->vertices[2].u, fonts->graphics_object4C->vertices[2].v
        );
    }

    if (fonts->graphics_object50 != nullptr && fonts->graphics_object50->vertices != nullptr) {
        ffnx_info("%s: 50 (%lf, %lf, u=%lf, v=%lf) (%lf, %lf, u=%lf, v=%lf) (%lf, %lf, u=%lf, v=%lf)\n", __func__,
            fonts->graphics_object50->vertices[0].x, fonts->graphics_object50->vertices[0].y,
            fonts->graphics_object50->vertices[0].u, fonts->graphics_object50->vertices[0].v,
            fonts->graphics_object50->vertices[1].x, fonts->graphics_object50->vertices[1].y,
            fonts->graphics_object50->vertices[1].u, fonts->graphics_object50->vertices[1].v,
            fonts->graphics_object50->vertices[2].x, fonts->graphics_object50->vertices[2].y,
            fonts->graphics_object50->vertices[2].u, fonts->graphics_object50->vertices[2].v
        );
    }

    if (fonts->graphics_object54 != nullptr && fonts->graphics_object54->vertices != nullptr) {
        ffnx_info("%s: 54 (%lf, %lf, u=%lf, v=%lf) (%lf, %lf, u=%lf, v=%lf) (%lf, %lf, u=%lf, v=%lf)\n", __func__,
            fonts->graphics_object54->vertices[0].x, fonts->graphics_object54->vertices[0].y,
            fonts->graphics_object54->vertices[0].u, fonts->graphics_object54->vertices[0].v,
            fonts->graphics_object54->vertices[1].x, fonts->graphics_object54->vertices[1].y,
            fonts->graphics_object54->vertices[1].u, fonts->graphics_object54->vertices[1].v,
            fonts->graphics_object54->vertices[2].x, fonts->graphics_object54->vertices[2].y,
            fonts->graphics_object54->vertices[2].u, fonts->graphics_object54->vertices[2].v
        );
    }

    /* if (fonts->graphics_object48 != nullptr && fonts->graphics_object48->field_74 != nullptr) {
        fonts->graphics_object48->field_74[0].field_0 += 0.5;
        fonts->graphics_object48->field_74[0].field_4 += 0.5;
        fonts->graphics_object48->field_74[1].field_0 += 0.5;
        fonts->graphics_object48->field_74[1].field_4 += 0.5;
        fonts->graphics_object48->field_74[2].field_0 += 0.5;
        fonts->graphics_object48->field_74[2].field_4 += 0.5;
    }
    if (fonts->graphics_object4C != nullptr && fonts->graphics_object4C->field_74 != nullptr) {
        fonts->graphics_object4C->field_74[0].field_0 += 0.5;
        fonts->graphics_object4C->field_74[0].field_4 += 0.5;
        fonts->graphics_object4C->field_74[1].field_0 += 0.5;
        fonts->graphics_object4C->field_74[1].field_4 += 0.5;
        fonts->graphics_object4C->field_74[2].field_0 += 0.5;
        fonts->graphics_object4C->field_74[2].field_4 += 0.5;
    }
    if (fonts->graphics_object50 != nullptr && fonts->graphics_object50->field_74 != nullptr) {
        fonts->graphics_object50->field_74[0].field_0 += 0.5;
        fonts->graphics_object50->field_74[0].field_4 += 0.5;
        fonts->graphics_object50->field_74[1].field_0 += 0.5;
        fonts->graphics_object50->field_74[1].field_4 += 0.5;
        fonts->graphics_object50->field_74[2].field_0 += 0.5;
        fonts->graphics_object50->field_74[2].field_4 += 0.5;
    }
    if (fonts->graphics_object54 != nullptr && fonts->graphics_object54->field_74 != nullptr) {
        fonts->graphics_object54->field_74[0].field_0 += 0.5;
        fonts->graphics_object54->field_74[0].field_4 += 0.5;
        fonts->graphics_object54->field_74[1].field_0 += 0.5;
        fonts->graphics_object54->field_74[1].field_4 += 0.5;
        fonts->graphics_object54->field_74[2].field_0 += 0.5;
        fonts->graphics_object54->field_74[2].field_4 += 0.5;
    } */
}

void build_icon_graphic_object_font8(ff8_draw_menu_sprite_texture_infos_short *texture_infos_short, ff8_graphics_object *graphic_object)
{
    ffnx_trace("%s: uv=(%d, %d)\n", __func__, texture_infos_short->u, texture_infos_short->v);

    int v4 = (texture_infos_short->palID >> 6) - fonts_sysevn->field_40;

    if (16 * (texture_infos_short->palID & 0x3F) != ((ff8_tex_header *)(((ff8_texture_set *)(graphic_object->hundred_data->texture_set))->tex_header))->field_DC
            && texture_infos_short->u >= 128u && texture_infos_short->v >= 152u && texture_infos_short->v < 200u) {
        ff8_graphics_object *graphic_object = (v4 & 1) != 0 ? graphic_object_font8_odd : graphic_object_font8_even;
        if (graphic_object != nullptr) {
            texture_infos_short->u -= 128; // u >= 0
            texture_infos_short->v -= 152; // v >= 0 && v < 48
            texture_infos_short->palID = (((ff8_tex_header *)(((ff8_texture_set *)(graphic_object->hundred_data->texture_set))->tex_header))->field_DC >> 4) & 0x3F | ((LOWORD(((ff8_tex_header *)(((ff8_texture_set *)(graphic_object->hundred_data->texture_set))->tex_header))->field_E0) + uint16_t(v4 / 2)) << 6);

            ((void(*)(ff8_draw_menu_sprite_texture_infos_short*,ff8_graphics_object*))0x49B300)(texture_infos_short, graphic_object); // TODO

            *(int *)(graphic_object->field_7C) = (*(int *)graphic_object->field_7C - 16) / 2;
            graphic_object->field_80 = (graphic_object->field_80 - 16) / 2;

            return;
        }
    }

    ((void(*)(ff8_draw_menu_sprite_texture_infos_short*,ff8_graphics_object*))0x49B300)(texture_infos_short, graphic_object);
}

void ff8_fonts_jp_render_kernel_menus(ff8_draw_menu_sprite_texture_infos_short *texture_infos, ff8_font *fonts)
{
    ffnx_trace("%s last_x=%d\n", __func__, last_x);
    int8_t i = 0;

    if (last_x != -1) {
        int x = last_x;
        for (;;) {
            i = *pointer_to_iterate_to++;
            if (i == 0 || x == texture_infos->x) {
                break;
            }

            x += ff8_externals.kernel_bin_sysfont[i].x_field_0 & 0xF;
        }
    } else {
        i = *pointer_to_iterate_to++;
    }

    // Add missing information to texture_infos
    texture_infos->palID = ff8_externals.kernel_bin_sysfont[i].pal_id_field_1 + ((texture_infos->palID - 0x3812) << 1) + 0x3812;

    return jp_fonts_with_font8c(texture_infos);
}

/*
 * This method should be not used at all, since it cannot recreate the original "character" value
 */
void ff8_fonts_jp_render_simple_menus(ff8_draw_menu_sprite_texture_infos_short *texture_infos, ff8_font *fonts)
{
    ffnx_trace("%s\n", __func__);

    uint16_t field_c_divided_by_12 = *(uint16_t *)&texture_infos->u / 12;
    // FIXME: Buggy when original character value > 5461
    int character = (field_c_divided_by_12 >> 8) * 21 + (field_c_divided_by_12 & 0xFF);

    *(uint16_t *)&texture_infos->u = 12 * (((character >> 1) % 21) | (((character >> 1) / 21) << 8));

    texture_infos->palID = ((character & 1) ? 14418 : 14354) + ((texture_infos->palID - 14354) << 1);

    ffnx_trace("%s: uv=(%d, %d) palID=%X character=%d\n", __func__, texture_infos->u, texture_infos->v, texture_infos->palID, character);

    return jp_fonts_with_font8c(texture_infos);
}

int get_character_width(int character)
{
    const uint8_t *font_char_width = ff8_remastered_edition && ff8_is_remastered_font_asset()
        ? ff8_remastered_font_jp_alignment_data
        : reinterpret_cast<uint8_t *>(ff8_externals.dword_1D2B808 + 0x10);

    if ((character & 0x400) != 0) {
        character &= 0x3FF;
        font_char_width = font_character_width_local_field;
    }
    uint8_t width = font_char_width[character >> 1];
    if ((character & 1) != 0) {
        width >>= 4;
    }

    ffnx_trace("%s: %X %d\n", __func__, character, width & 0xF);

    return width & 0xF;
}

uint8_t *ff8_fonts_jp_kernel_bin_get_section(int section_id)
{
    ffnx_trace("%s\n", __func__);

    struc_kernel_sysfont *kernel_sysfont = ff8_externals.kernel_bin_sysfont + 1;
    for (int i = 0; i < 10; ++i) {
        int character = ((uint8_t*(*)(int))ff8_externals.kernel_bin_get_section_sub_47EC70)(section_id)[1] + i - 32;
        int space = get_character_width(character);
        kernel_sysfont->x_field_0 ^= (space ^ kernel_sysfont->x_field_0) & 0xF;
        if (space <= 8) {
            kernel_sysfont->x_field_0 = kernel_sysfont->x_field_0 & 0xF | (16 * ((8 - space) / 2));
        } else {
            kernel_sysfont->x_field_0 = kernel_sysfont->x_field_0 & 0xF;
        }
        kernel_sysfont->uv_field_2 = (3072 * (character / 2 / 21)) | uint8_t(12 * (character / 2 % 21));
        ++kernel_sysfont;
    }

    return ((uint8_t*(*)(int))ff8_externals.kernel_bin_get_section_sub_47EC70)(section_id);
}

void fill_texture_infos_for_font(ff8_draw_menu_sprite_texture_infos *texture_infos, int x, int y, int character, int current_color, uint32_t *field8, uint32_t command = 0x5000000)
{
    ffnx_trace("%s character=%X xy=(%d, %d)\n", __func__, character, x, y);

    bool is_extended_font = (character & 0x400) != 0;

    texture_infos->command = command;
    // << 7 only in jp version + 14418 only in jp version
    texture_infos->inner.palID = ((current_color & 7) << 7) + ((character & 1) ? 14418 : 14354);
    texture_infos->inner.color = (current_color & 0xFFFFFFF8) == 0 ? *field8 : *(field8 + 1);
    texture_infos->inner.texID = is_extended_font ? 0xE100041D : 0xE100041F;
    texture_infos->inner.x = uint16_t(x);
    texture_infos->inner.y = uint16_t(y);
    int character2 = (is_extended_font ? character & 0x3FF : character) >> 1; // >> 1 only in jp version
    texture_infos->inner.w = 12;
    texture_infos->inner.h = 12;
    texture_infos->inner.u = 12 * (character2 % 21);
    texture_infos->inner.v = 12 * (character2 / 21);

    ffnx_trace("%s uv=(%d, %d) character2=%X is_extended_font=%d\n", __func__, texture_infos->inner.u, texture_infos->inner.v, character2, is_extended_font);

    // [jp]fonts_field_sub_4A0EE0
    if (is_extended_font && fonts_fieldtdw_odd->graphics_object48 != nullptr && fonts_fieldtdw_even->graphics_object48 != nullptr) {
        int is_odd = (texture_infos->inner.palID >> 6) - fonts_fieldtdw_even->field_40;
        texture_infos->inner.palID = ((texture_infos->inner.palID >> 6) << 6) | ((fonts_fieldtdw_even->field_3E >> 4) & 0x3F);

        ((void(*)(ff8_draw_menu_sprite_texture_infos_short*,ff8_font*))ff8_externals.sub_49D6F0)(&texture_infos->inner, (is_odd & 1) != 0 ? fonts_fieldtdw_odd : fonts_fieldtdw_even);
    } else { // [jp]fonts_sysoddeven_sub_4A0E00
        jp_fonts_with_font8c(&texture_infos->inner);
    }
}

int get_icon_id(uint8_t icon_param, bool bound_icon_param_to_63 = false)
{
    int icon_id = 0;
    if (icon_param >= 64) {
        icon_id = ff8_externals.word_B86D84[icon_param];
    } else if (icon_param < 32 || icon_param > 47) {
        if (icon_param >= 48 && (!bound_icon_param_to_63 || icon_param <= 63)) {
            icon_id = icon_param + 80;
        }
    } else {
        int key_from_key_id = ((int(*)(int))ff8_externals.sub_4A2DF0)(icon_param - 32);
        if (key_from_key_id >= 0) {
            icon_id = key_from_key_id + 128;
        }
    }

    return icon_id;
}

ff8_draw_menu_sprite_texture_infos *fill_texture_infos_for_icon(int *a1, ff8_draw_menu_sprite_texture_infos *texture_infos, int &x, int y, int icon_id)
{
    if (a1 != nullptr || texture_infos != nullptr) {
        int dword_1D2B514 = *(int *)ff8_externals.dword_1D2B514;
        texture_infos = ((ff8_draw_menu_sprite_texture_infos*(*)(int*,ff8_draw_menu_sprite_texture_infos*,void*,int,uint16_t,uint16_t,int))ff8_externals.sub_4B75B0)(a1, texture_infos, ((void*(*)())ff8_externals.get_icon_sp1_data)(), icon_id, x, y, dword_1D2B514);
    }
    x += uint8_t(((uint16_t(*)(void*,int))ff8_externals.sub_4B73F0)(((void*(*)())ff8_externals.get_icon_sp1_data)(), icon_id)) + 1;

    return texture_infos;
}

int ff8_fonts_get_text_dimensions(uint8_t *text_data, bool continue_on_new_line)
{
    ffnx_trace("%s\n", __func__);

    int x = 0, y = 12, max_x = 0, max_y = 12;

    for (;;) {
        uint8_t current_byte = *text_data++;

        if (current_byte == 0) {
            break;
        }

        if (current_byte == 2 || current_byte == 1 || current_byte == 7) { // New line, New page, ???
            if (current_byte == 2) { // New line
                y += 16;
            } else {
                y = 12;
            }
            if (max_y < y) {
                max_y = y;
            }
            if (max_x < x) {
                max_x = x;
            }
            x = 0;
            if (!continue_on_new_line) {
                return max_x | (max_y << 16);
            }
        } else if (current_byte == 5) { // Icon
            int next_byte = *text_data++;
            fill_texture_infos_for_icon(nullptr, nullptr, x, 0, get_icon_id(next_byte));
        } else if (current_byte <= 15) {
            ++text_data;
        } else if (current_byte >= 24) {
            int character;
            if (current_byte < 32) { // two-bytes
                if (current_byte > 27) { // Field extended font (JP version only)
                    character = int(*text_data + 224 * current_byte - 6304) | 0x400;
                } else {
                    character = *text_data + 224 * current_byte - 5408;
                }
                text_data++;
            } else {
                character = current_byte - 32;
            }
            x += get_character_width(character);
        }
    }
    if (max_x < x) {
        max_x = x;
    }
    if (max_y < y) {
        max_y = y;
    }
    return max_x | (max_y << 16);
}

int sub_4BDD60(int a1, ff8_draw_menu_sprite_texture_infos *texture_infos, int character, int current_color, int xy)
{
    fill_texture_infos_for_font(texture_infos, xy & 0xFFFF, xy >> 16, character, current_color, ff8_externals.dword_1D2B100, 0x4000000);

    return a1;
}

ff8_draw_menu_sprite_texture_infos *ff8_fonts_parse_and_render_menu_texts_1(
    int *a1,
    ff8_draw_menu_sprite_texture_infos *texture_infos,
    int x,
    int y,
    uint8_t *text_data,
    int current_color
) {
    ffnx_trace("%s\n", __func__);

    const int x_orig = x;

    if (text_data == nullptr || y > 256 || y < -8) {
        return texture_infos;
    }

    for (;;) {
        uint8_t current_byte = *text_data++;

        if (current_byte == 2) { // new line
            x = x_orig;
            y += 16;

            continue;
        }

        if (current_byte <= 24 || x > 384) {
            break;
        }

        int character;
        if (current_byte < 32) { // two-bytes
            if (current_byte > 27) { // Field extended font
                character = int(*text_data + 224 * current_byte - 6304) | 0x400;
            } else {
                character = *text_data + 224 * current_byte - 5408;
            }
            text_data++;
        } else {
            character = current_byte - 32;
        }
        fill_texture_infos_for_font(texture_infos, x, y, character, current_color, ff8_externals.dword_1D2B100);
        x += get_character_width(character);
        ++texture_infos;
    }

    return texture_infos;
}

void ff8_fonts_parse_and_render_menu_texts_2(int *a1, int x, int y, uint8_t *text_data)
{
    ffnx_trace("%s\n", __func__);

    const int x_orig = x;
    int *dword_1D76608 = (int *)ff8_externals.dword_1D76608;
    int dword_227C6F0_orig = *dword_1D76608;
    int some_struct = ((int(*)(int))ff8_externals.sub_403E00)(0);
    uint8_t *output = (uint8_t *)(some_struct + 768);
    *dword_1D76608 = some_struct + 896;
    int current_color = 7;
    uint8_t *text_it = text_data;
    ff8_draw_menu_sprite_texture_infos *texture_infos = ((ff8_draw_menu_sprite_texture_infos*(*)())ff8_externals.sub_49AB40)();
    uint8_t *last_color_iterate_text_byte_1D762E4 = (uint8_t *)ff8_externals.dword_1D7660C;

    while (text_it) {
        ((void(*)(uint8_t*,uint8_t*,int))ff8_externals.sub_4B8B30)(text_it, output, -1); // Expand text with names
        *last_color_iterate_text_byte_1D762E4 = current_color;
        text_it = ((uint8_t*(*)(uint8_t*))ff8_externals.sub_4B8AC0)(text_it); // To next line
        text_data = output;
        for (;;) {
            int current_byte = *text_data++;

            if (current_byte == 0 || current_byte == 1 || current_byte == 7) { // end of text, New page, ???
                text_it = nullptr; // Stop
                break;
            }

            if (current_byte == 2) { // New line
                x = x_orig;
                y += 16;
                break;
            }

            if (current_byte == 5) { // Icons
                // x is modified
                texture_infos = fill_texture_infos_for_icon(a1, texture_infos, x, y, get_icon_id(*text_data++));
            } else if (current_byte == 6) { // Color
                current_color = (*text_data++) & 0xF;
            } else if (current_byte <= 15) {
                ++text_data;
            } else if (current_byte > 24) {
                int character;
                if (current_byte < 32) { // two-bytes
                    if (current_byte > 27) { // Field extended font
                        character = int(*text_data + 224 * current_byte - 6304) | 0x400;
                    } else {
                        character = *text_data + 224 * current_byte - 5408;
                    }
                    text_data++;
                } else {
                    character = current_byte - 32;
                }
                fill_texture_infos_for_font(texture_infos, x, y, character, current_color, (uint32_t *)ff8_externals.dword_1D2B514); // jp: dword_22314BC
                x += get_character_width(character);
                ++texture_infos;
            }
        }
    }

    ((void(*)(ff8_draw_menu_sprite_texture_infos*))ff8_externals.sub_49AB60)(texture_infos);
    *dword_1D76608 = dword_227C6F0_orig;
}

void ff8_fonts_parse_and_render_field_texts(int *a1, ff8_win_obj *win)
{
    ffnx_trace("%s\n", __func__);

    ((void(*)())ff8_externals.sub_49B0B0)();
    uint8_t **dword_1D76608 = (uint8_t **)ff8_externals.dword_1D76608;
    uint8_t *output = *dword_1D76608;
    uint8_t *text_it = (uint8_t *)win->text_data1;
    *dword_1D76608 += 128;
    int first_answer_line = win->first_question, last_anwser_line = win->last_question;
    ff8_draw_menu_sprite_texture_infos *texture_infos = ((ff8_draw_menu_sprite_texture_infos*(*)())ff8_externals.sub_49AB40)();
    int x = win->field_30 + 2, y = win->field_32 - win->field_12 + 2;
    int current_line = 0;
    int current_color = win->current_color >> 4;
    if (first_answer_line <= 0) {
        x += 34;
    }
    uint8_t *last_color_iterate_text_byte_1D762E4 = (uint8_t *)ff8_externals.dword_1D7660C;
    *last_color_iterate_text_byte_1D762E4 = current_color;

    while (y < -16) {
        if (text_it == nullptr) {
            ((void(*)(ff8_draw_menu_sprite_texture_infos*))ff8_externals.sub_49AB60)(texture_infos);
            *dword_1D76608 -= 128;
            ((void(*)())ff8_externals.sub_49B0D0)();

            return;
        }
        text_it = ((uint8_t*(*)(uint8_t*))ff8_externals.sub_4B8AC0)(text_it); // To next line
        current_color = *last_color_iterate_text_byte_1D762E4 & 0xF;
        y += 16;
        ++current_line;
    }

    while (text_it != nullptr) {
        // Expand text with names
        ((void(*)(uint8_t*,uint8_t*,int))ff8_externals.sub_4B8B30)(text_it, output, current_line >= win->text_data1_line ? win->text_data1_offset : -1);
        *last_color_iterate_text_byte_1D762E4 = current_color;
        text_it = ((uint8_t*(*)(uint8_t*))ff8_externals.sub_4B8AC0)(text_it); // To next line
        ++current_line;
        uint8_t *text_data = output;
        for (;;) {
            int current_byte = *text_data++;

            if (current_byte == 0 || current_byte == 1 || current_byte == 7) { // end of text, New page, ???
                text_it = nullptr; // Stop
                break;
            }

            if (current_byte == 2) { // New line
                x = win->field_30 + 2;
                if (first_answer_line <= current_line && last_anwser_line >= current_line) {
                    x += 34;
                }
                y += 16;
                break;
            }

            if (current_byte == 5) { // Icons
                // x is modified
                texture_infos = fill_texture_infos_for_icon(a1, texture_infos, x, y, get_icon_id(*text_data++, true));
            } else if (current_byte == 6) { // Color
                current_color = (*text_data++) & 0xF;
            } else if (current_byte <= 15) {
                ++text_data;
            } else if (current_byte > 24) {
                int character;
                if (current_byte < 32) { // two-bytes
                    if (current_byte > 27) { // Field extended font
                        character = int(*text_data + 224 * current_byte - 6304) | 0x400;
                    } else {
                        character = *text_data + 224 * current_byte - 5408;
                    }
                    text_data++;
                } else {
                    character = current_byte - 32;
                }
                fill_texture_infos_for_font(texture_infos, x, y, character, current_color, (uint32_t *)ff8_externals.dword_1D2B514);
                x += get_character_width(character);
            }
        }
    }

    ((void(*)(ff8_draw_menu_sprite_texture_infos*))ff8_externals.sub_49AB60)(texture_infos);
    *dword_1D76608 -= 128;
    ((void(*)())ff8_externals.sub_49B0D0)();
}

ff8_draw_menu_sprite_texture_infos *battle_text_parse_common(
    int *a1,
    ff8_draw_menu_sprite_texture_infos *texture_infos,
    int x,
    int y,
    uint8_t *text_data,
    int16_t current_color,
    uint32_t *field8
) {
    ffnx_trace("%s\n", __func__);

    if (text_data == nullptr) {
        return texture_infos;
    }

    ((void(*)())ff8_externals.sub_49B080)();

    int x_orig = x;

    for (;;) {
        uint8_t current_byte = *text_data++;

        if (current_byte == 2) { // new line
            x = x_orig;
            y += 16;

            continue;
        }

        if (current_byte <= 24) {
            break;
        }

        int character;
        if (current_byte < 32) { // two-bytes
            if (current_byte > 27) { // Field extended font
                character = current_byte; // undefined behavior
            } else {
                character = *text_data + 224 * current_byte - 5408;
            }
            text_data++;
        } else {
            character = current_byte - 32;
        }
        fill_texture_infos_for_font(texture_infos, x, y, character, current_color & 7, field8);
        x += get_character_width(character);
        ++texture_infos;
    }

    ((void(*)())ff8_externals.sub_49B080)();

    return texture_infos;
}

ff8_draw_menu_sprite_texture_infos *ff8_fonts_parse_and_render_battle_texts_1(
    int *a1,
    ff8_draw_menu_sprite_texture_infos *texture_infos,
    int x,
    int y,
    uint8_t *text_data,
    int16_t current_color
) {
    ffnx_trace("%s\n", __func__);

    return battle_text_parse_common(a1, texture_infos, x, y, text_data, current_color, ((uint32_t*(*)(int))ff8_externals.sub_403E00)(0) + 228);
}

ff8_draw_menu_sprite_texture_infos *ff8_fonts_parse_and_render_battle_texts_2(
    int *a1,
    ff8_draw_menu_sprite_texture_infos *texture_infos,
    int x,
    int y,
    uint8_t *text_data,
    int16_t current_color
) {
    ffnx_trace("%s\n", __func__);

    DWORD *aicon_sp1_data = ((DWORD*(*)())ff8_externals.get_icon_sp1_data)();
    DWORD *battle_menu_state = (DWORD *)ff8_externals.battle_menu_state;

    uint32_t v14 = *(DWORD *)((char *)aicon_sp1_data + uint16_t(aicon_sp1_data[*((uint16_t *)battle_menu_state + 34) + 1]));
    uint32_t field8 = (*battle_menu_state & 0xFF000000) | 0x808080 | (((v14 >> 26) & 2) << 24);

    return battle_text_parse_common(a1, texture_infos, x, y, text_data, current_color, &field8);
}

int32_t ff8_open_tdw_field(char *id_path, void *data)
{
    char tdw_path[MAX_PATH] = {};

    strncpy(tdw_path, id_path, strnlen(id_path, MAX_PATH) - 2);
    strcat(tdw_path, "tdw");

    if (ff8_externals.sm_pc_read(tdw_path, data) != 8) {
        uint32_t *tdw_header = (uint32_t *)data;

        if (tdw_header[1]) {
            ff8_load_fonts_field((char *)data + tdw_header[1], tdw_path);
            memcpy(font_character_width_local_field, (char *)data + tdw_header[0], sizeof(font_character_width_local_field));
            ((void(*)())ff8_externals.syfont_set_kernel_bin_pointers_sub_49F640)();
        }
    }

    return ff8_externals.sm_pc_read(id_path, data);
}

void convert_ascii_to_ff8_encoding_jp(char *data)
{
    ffnx_trace("%s: %s\n", __func__, data);

    size_t i = 0, len = strlen(data);

    for (; i < len; ++i) {
        char c = data[i];
        if (c >= 'A' && c <= 'Z') {
            c -= 0x73;
        } else if (c >= '0' && c <= '9') {
            c += 0x23;
        } else if (c >= 'a' && c <= 'z') {
            c += 0x6D;
        } else {
            c = 0x5F;
        }
        data[i] = c;
    }

    data[i] = 0;
}

#define JP_NAME_CHARW       16
#define JP_NAME_CHARW_WIDE  17
#define JP_NAME_GROUPW      90
#define JP_NAME_GROUPW_LAST 85
#define JP_NAME_ROWH        19
#define JP_NAME_COLS        3
#define JP_NAME_GROUPCHARS  5

static constexpr uint16_t ff8_jp_name_grid_page0[] = {
    0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
    0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D,
    0x003E, 0x003F, 0xFFFF
};
static constexpr uint16_t ff8_jp_name_grid_page1[] = {
    0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022,
    0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A,
    0x002B, 0x002C, 0xFFFF
};
static constexpr uint16_t ff8_jp_name_grid_page2[] = {
    0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048,
    0x0049, 0x004A, 0x004B, 0x004C, 0xFFFF
};

uint32_t *jp_name_entry_draw_grid(uint8_t *a1, int *a2, uint32_t *a3, int a4, int a5)
{
    uint8_t tab = a1[44];
    const bool lastPage = tab == 2;
    const uint16_t groupw = lastPage ? JP_NAME_GROUPW_LAST : JP_NAME_GROUPW;
    const uint16_t *rowlist = tab == 0 ? ff8_jp_name_grid_page0 : (tab == 1 ? ff8_jp_name_grid_page1 : ff8_jp_name_grid_page2);
    int ctx = *a2;

    for (int i = 0; ; i++)
    {
        uint16_t rowid = rowlist[i];
        if (rowid == 0xFFFF) {
            break;
        }

        int x = a4 + (i % JP_NAME_COLS) * groupw;
        int y = a5 + (i / JP_NAME_COLS) * JP_NAME_ROWH;
        int ypacked = y << 16;
        const uint8_t *s = (const uint8_t *)((char *(*)(int, int, int, int))0x4BD630)(1, 5, rowid, 0);

        bool ended = false;
        for (int col = 0; col < JP_NAME_GROUPCHARS; col++)
        {
            uint8_t b = *s++;
            ctx = sub_4BDD60(ctx, (ff8_draw_menu_sprite_texture_infos *)a3, int(b) - 32, 7, ypacked | uint16_t(x + (lastPage ? JP_NAME_CHARW : JP_NAME_CHARW_WIDE) * col));
            a3 += 5;
        }

        a3[0] = 0x01000000;
        a3[1] = 0xE100041F;
        a3 += 2;
    }

    return a3;
}

void menu_name_controller_alter_structure(uint8_t *a1)
{
    ffnx_trace("%s\n", __func__);
    if (a1[47]) {
        ((void(*)(char,int16_t,int16_t))0x4BD6E0)(0, 34, 16 * a1[46] + 98);
    } else {
        uint8_t tab = a1[44];
        int y = a1[45], x = y % 15 / 5;
        if (tab != 2) {
            x = JP_NAME_CHARW * (y % 5) + 90 * x;
        } else {
            x = JP_NAME_CHARW_WIDE * (5 * x + y % 5);
        }
        ((void(*)(char,int16_t,int16_t))0x4BD6E0)(0, int16_t(x) + 100, 19 * (int16_t(y) / 15) + 96);
    }
}

void menu_name_controller(int a1)
{
    ((void(*)(DWORD))0x4BD690)(*(DWORD *)(a1 + 40));

    uint8_t tab = *(uint8_t *)(a1 + 44);
    const bool lastPage = tab == 2;
    const uint8_t last_row = lastPage ? 3 : 5;
    const uint16_t *rowlist = tab == 0 ? ff8_jp_name_grid_page0 : (tab == 1 ? ff8_jp_name_grid_page1 : ff8_jp_name_grid_page2);
    int16_t dword_1D76A98 = *(int16_t *)0x1D76A98;
    int16_t dword_1D76A9A = *(int16_t *)0x1D76A9A;
    int16_t dword_1D76A9C = *(int16_t *)0x1D76A9C;

    ffnx_trace("%s: %d tab=%d dword_1D76A98=%X dword_1D76A9A=%X dword_1D76A9C=%X\n", __func__, *(WORD *)(a1 + 16), tab, dword_1D76A98, dword_1D76A9A, dword_1D76A9C);

    switch (*(WORD *)(a1 + 16)) {
    case 0:
        *(DWORD *)(a1 + 40) = 0;
        *(WORD *)(a1 + 16) = 1;
        break;
    case 1: {
        int v3 = *(DWORD *)(a1 + 40) + 256;
        *(DWORD *)(a1 + 40) = v3;
        if ( v3 >= 4096 ) {
            *(DWORD *)(a1 + 40) = 4096;
            *(WORD *)(a1 + 16) = 2;
        }
        menu_name_controller_alter_structure((uint8_t *)a1);
        break;
    }
    case 2:
        *(BYTE *)(a1 + 47) = 0;
        *(WORD *)(a1 + 16) = 3;
        menu_name_controller_alter_structure((uint8_t *)a1);
        break;
    case 3: {
        int col = *(uint8_t *)(a1 + 45) % 15; // 14 -> 15
        int row = *(uint8_t *)(a1 + 45) / 15; // 14 -> 15
        if ((dword_1D76A9A & 0x1000) != 0) { // up
            if (--row >= 0) {
                ((void(*)(int))0x4B92A0)(1);
            } else {
                row = 0;
            }
        }
        if ((dword_1D76A9A & 0x4000) != 0) { // down
            if (row < last_row) {
                ((void(*)(int))0x4B92A0)(1);
            }
            ++row;
        }
        if (row > last_row) {
            row = last_row;
        }
        if ((dword_1D76A9A & 0x2000) != 0) { // right
            if (++col < 15) { // 14 -> 15
                ((void(*)(int))0x4B92A0)(1);
            } else {
                col = 14; // 13 -> 14
            }
        }
        if ((dword_1D76A9A & 0x8000) != 0) { // left
            ((void(*)(int))0x4B92A0)(1);
            if (--col < 0) {
                col = 0;
                *(WORD *)(a1 + 16) = 4;
            }
        }
        *(BYTE *)(a1 + 45) = col + 15 * row; // 14 -> 15
        menu_name_controller_alter_structure((uint8_t *)a1);
        if ((dword_1D76A9C & 0x10) != 0) {
            ((void(*)(int))0x4B92A0)(3);
            int v10 = *(DWORD *)(a1 + 36);
            if (*(BYTE *)v10) {
                *(BYTE *)(v10 + strlen((const char *)(v10 + 1))) = 0;
            }
        } else if ((dword_1D76A9A & 0x40) != 0) {
            ((void(*)(int))0x4B92A0)(2);
            BYTE *v11 = *(BYTE **)(a1 + 36);
            int v12 = **(uint8_t **)(a1 + 32);
            int v13 = 0;
            int v14 = 0;
            BYTE *v15 = v11;
            if (!**(BYTE **)(a1 + 32)) {
                v13 = v12 - 1;
            } else {
                do
                {
                    if (!*v15++) break;
                    ++v13;
                    ++v14;
                } while ( v14 < v12 );
                if ( v13 >= v12 ) {
                    v13 = v12 - 1;
                }
            }
            v11[v13] = ((char *(*)(int, int, int, int))0x4BD630)(
                1,
                5,
                *(rowlist + *(uint8_t *)(a1 + 45) / 5),
                0)[*(uint8_t *)(a1 + 45) % 5];
            v11[v13 + 1] = 0;
        } else {
            if ((dword_1D76A9C & 8) != 0) {
                ((void(*)(int))0x4B92A0)(1);
                *(BYTE *)(a1 + 44) = (*(BYTE *)(a1 + 44) + 1) % 3; // Instead of % 2
            }
            if ((dword_1D76A9C & 4) != 0) {
                ((void(*)(int))0x4B92A0)(1);
                *(BYTE *)(a1 + 44) = (*(uint8_t *)(a1 + 44) - 1 + (*(uint8_t *)(a1 + 44) - 1 < 0 ? 3 : 0)) % 3; // Instead of % 2
            }
            if ((dword_1D76A9C & 0x800) != 0) { // Start
                ((void(*)(int))0x4B92A0)(1);
                *(BYTE *)(a1 + 46) = 3;
                *(WORD *)(a1 + 16) = 4;
            }
        }
        break;
    }
    case 4:
        *(BYTE *)(a1 + 47) = 1;
        *(WORD *)(a1 + 16) = 5;
        break;
    case 5: {
        int v17 = *(uint8_t *)(a1 + 46);
        if ((dword_1D76A9C & 8) != 0) {
            ((void(*)(int))0x4B92A0)(1);
            *(BYTE *)(a1 + 44) = (*(BYTE *)(a1 + 44) + 1) % 3; // Instead of % 2
        }
        if ((dword_1D76A9C & 4) != 0) {
            ((void(*)(int))0x4B92A0)(1);
            *(BYTE *)(a1 + 44) = (*(uint8_t *)(a1 + 44) - 1 + (*(uint8_t *)(a1 + 44) - 1 < 0 ? 3 : 0)) % 3; // Instead of % 2
        }
        if ((dword_1D76A9A & 0x4000) != 0) {
            ((void(*)(int))0x4B92A0)(1);
            if (++v17 >= 6) {
                v17 = 0;
            }
        }
        if ((dword_1D76A9A & 0x1000) != 0) {
            ((void(*)(int))0x4B92A0)(1);
            if (--v17 < 0) {
                v17 = 5;
            }
        }
        if ((dword_1D76A9C & 0x40) != 0) {
            ffnx_trace("%s: left menu 2 %d\n", __func__, v17);
            switch (v17) {
            case 0:
                ((void(*)(int))0x4B92A0)(2);
                *(BYTE *)(a1 + 44) = 0;
                break;
            case 1:
                ((void(*)(int))0x4B92A0)(2);
                *(BYTE *)(a1 + 44) = 1;
                break;
            case 2:
                ((void(*)(int))0x4B92A0)(2);
                *(BYTE *)(a1 + 44) = 2;
                break;
            case 3: {
                BYTE *v25 = *(BYTE **)(a1 + 36);
                int sfx = 5;
                if (strlen((const char *)v25)) {
                    for (;;) {
                        int v26 = (char)*v25++;
                        if (!v26) break;
                        if (v26 != *((uint8_t*(*)(int))ff8_externals.kernel_bin_get_section_sub_47EC70)(11)) {
                            sfx = 2;
                            *(WORD *)(a1 + 16) = 6;
                            break;
                        }
                    }
                }

                ((void(*)(int))0x4B92A0)(sfx);
                break;
            }
            case 4: {
                ((void(*)(int))0x4B92A0)(3);
                int v24 = *(DWORD *)(a1 + 36);
                if (*(BYTE *)v24) {
                    *(BYTE *)(v24 + strlen((const char *)(v24 + 1))) = 0;
                }
                break;
            }
            case 5: {
                ((void(*)(int))0x4B92A0)(2);
                char *text = ((char *(*)(int, int, int, int))0x4BD630)(1, 5, *(uint16_t *)(*(DWORD *)(a1 + 32) + 2), 0);
                strcpy(*(char **)(a1 + 36), text);
                break;
            }
            }
        }
        if ((dword_1D76A9C & 0x800) != 0) { // Start
            ((void(*)(int))0x4B92A0)(1);
            v17 = 3;
        }
        *(BYTE *)(a1 + 46) = v17;
        menu_name_controller_alter_structure((uint8_t *)a1);
        if ((dword_1D76A9A & 0x2000) != 0) {
            ((void(*)(int))0x4B92A0)(2);
            *(WORD *)(a1 + 16) = 1;
        }
        break;
    }
    case 6:
        ((void(*)())0x495EF0)();
        menu_name_controller_alter_structure((uint8_t *)a1);
        *(WORD *)(a1 + 16) = 7;
        break;
    case 7: {
        int v33 = *(DWORD *)(a1 + 40);
        *(DWORD *)(a1 + 40) = v33 - 256;
        if (v33 - 256 < 0) {
            *(DWORD *)(a1 + 40) = 0;
            ((void(*)(int))0x4BE610)(a1);
            ((void(*)())0x4BDAC0)();
            *(WORD *)(a1 + 16) = 2;
        }
        menu_name_controller_alter_structure((uint8_t *)a1);
        break;
    }
    }

    BYTE *v36 = *(BYTE **)(a1 + 32), *v37 = *(BYTE **)(a1 + 36);
    int v38 = 0, v39 = (uint8_t)*v36, v40 = 0;

    if (!*v36) {
        v38 = v39 - 1;
    } else {
        do {
            if (!*v37++) break;
            ++v38;
            ++v40;
        } while (v40 < v39);
        if (v38 >= v39) {
            v38 = v39 - 1;
        }
    }
    *(BYTE *)(a1 + 48) = v38;

    ((void(*)(DWORD))0x4BD690)(*(DWORD *)(a1 + 40));
}

ff8_draw_menu_sprite_texture_infos *ff8_fonts_parse_and_render_menu_texts_3(
    int *a1,
    ff8_draw_menu_sprite_texture_infos *texture_infos,
    int x,
    int y,
    uint8_t *text_data,
    int current_color)
{
    if (text_data == nullptr || y > 256 || y < -8) {
        return texture_infos;
    }

    const int x_orig = x;
    int min_x = *(int *)0x1D76AC6;
    int max_x = *(int *)0x1D77148;

    ((void(*)())ff8_externals.sub_49B080)();
    for (;;) {
        uint8_t current_byte = *text_data++;

        if (current_byte == 2) { // new line
            x = x_orig;
            y += 13;
        } else if (current_byte == 5) {
            uint8_t next_byte = *text_data++;
            texture_infos->command = 0x1000000;
            texture_infos->inner.texID = 0xE100041F;
            int icon_id = get_icon_id(next_byte);
            texture_infos = ((ff8_draw_menu_sprite_texture_infos *(*)(int*,ff8_draw_menu_sprite_texture_infos*,int,uint16_t,uint16_t,uint32_t))ff8_externals.ff8_draw_icon_or_key1)(
                a1,
                (ff8_draw_menu_sprite_texture_infos *)&texture_infos->inner.color,
                icon_id,
                x,
                y,
                ff8_externals.dword_1D2B100[0]
            );
            x += uint8_t(((uint16_t(*)(void*,int))ff8_externals.sub_4B73F0)(((void*(*)())ff8_externals.get_icon_sp1_data)(), icon_id)) + 1;
        } else if (current_byte > 24 && x < max_x) {
            int character;
            if (current_byte < 32) {
                character = *text_data + 224 * current_byte - 0x1520;
                text_data++;
            } else {
                character = current_byte - 32;
            }
            if (x >= min_x) {
                fill_texture_infos_for_font(texture_infos, x, y, character, current_color, ff8_externals.dword_1D2B100, 0x4000000);
                texture_infos = (ff8_draw_menu_sprite_texture_infos *)((char *)texture_infos + 20);
            }
            x += get_character_width(character);
        } else {
            break;
        }
    }
    texture_infos->command = 0x1000000;
    texture_infos->inner.texID = 0xE100041F;
    ((void(*)())ff8_externals.sub_49B080)();

    return (ff8_draw_menu_sprite_texture_infos *)&texture_infos->inner.color;
}

void fonts_init_jp()
{
    ffnx_trace("%s: fonts_initialized=%d is_japanese_font_loaded=%d\n", __func__, fonts_initialized, fonts_sysevn->graphics_object48 != nullptr);

    if (JP_VERSION || fonts_initialized || fonts_sysevn->graphics_object48 == nullptr) {
        return;
    }

    fonts_initialized = true;

    // Rendering
    replace_call(ff8_externals.engine_draw_2D_texture_sub_4980C0 + 0xF0 + 0x7, ff8_fonts_jp_rendering_reset_field_58);
    replace_call(ff8_externals.engine_draw_2D_texture_sub_4980C0 + 0xCD, ff8_fonts_jp_draw);

    // Name selection menu
    replace_function(0x4E7470, jp_name_entry_draw_grid);
    replace_function(0x4E6990, menu_name_controller);
    patch_code_byte(0x4E7170 + 0x4D, 41); // x: 48 -> 41
    patch_code_dword(0x4E7170 + 0x70, 110); // x: 124 -> 110
    patch_code_dword(0x4E7170 + 0x13A, 92); // x: 106 -> 92
    patch_code_dword(0x4E7170 + 0x145, 272); // x: 258 -> 272
    patch_code_word(0x4E7170 + 0x2AE, 92);
    patch_code_word(0x4E7170 + 0x2BE, 272);
    patch_code_byte(0x4E7170 + 0x193, 100); // x: 114 -> 110
    patch_code_byte(0x4E7170 + 0x27E, 100);
    patch_code_byte(0x4E7170 + 0x27C, 96); // y: 98 -> 96
    patch_code_word(0x4E7170 + 0x26D, 66); // x: 80 -> 66

    /* patch_code_dword(0x4E6990 + 0xAA, 15);
    patch_code_byte(0x4E6990 + 0x125, 15);
    patch_code_dword(0x4E6990 + 0x130, 14);
    patch_code_dword(0x4E6990 + 0x283, 5);
    patch_code_byte(0x4E6990 + 0x2F7, 0xB8);
    patch_code_dword(0x4E6990 + 0x2F7 + 1, 3);
    patch_code_byte(0x4E6990 + 0x2F7 + 5, 0x99);
    // Fix third page change glitch
    patch_code_dword(0x4E6990 + 0x45A, 0x022C46C6);
    patch_code_byte(0x4E6990 + 0x45A + 4, 0xE9);
    patch_code_dword(0x4E6990 + 0x45A + 5, 49); */

    // Menu simple text
    //replace_call(ff8_externals.sub_49C910 + 0xB, ff8_fonts_jp_render_simple_menus);
    replace_function(0x4BDD60, sub_4BDD60);
    // Menu simple text with kernel.bin changes
    replace_call(ff8_externals.syfont_set_kernel_bin_pointers_sub_49F640 + 0xD2, ff8_fonts_jp_kernel_bin_get_section);
    replace_call(ff8_externals.sub_49C5F0 + 0xB, ff8_fonts_jp_render_kernel_menus);
    replace_call(ff8_externals.sub_4A3400 + 0x21, ff8_fonts_jp_render_kernel_menus_before_loops);
    replace_call(ff8_externals.sub_4A3400 + 0xCE, ff8_fonts_jp_render_kernel_menus_after_loops);
    // Try to bring back save entry icons
    //replace_call(0x4E6020 + 0x82, load_save_render_entry_icon);
    //replace_call(0x4E6020 + 0xC3, load_save_render_entry_icon2);
    // Battle simple text with kernel.bin changes
    // TODO: rewrite the sub completely without font use (icon instead)
    /* replace_call(0x56F5E0 + 0x86, call_draw_icon_sub_4B7210);
    replace_call(0x56F5E0 + 0xFE, ff8_fonts_jp_render_kernel_menus_before_loops);
    replace_call(0x56F5E0 + 0x1C0, ff8_fonts_jp_render_kernel_menus_after_loops); */

    // Complex text parsing
    replace_function(ff8_externals.font_text_size_calculation_sub_4A0D10, ff8_fonts_get_text_dimensions); // For text size calculation
    replace_function(ff8_externals.sub_4A1020, ff8_fonts_parse_and_render_menu_texts_1); // Menu texts and icons
    replace_function(ff8_externals.font_parse_and_render_menu_2_sub_4A1200, ff8_fonts_parse_and_render_menu_texts_2); // Menu texts
    replace_function(0x4BDE30, ff8_fonts_parse_and_render_menu_texts_3); // Menu texts
    replace_function(ff8_externals.render_text_field_sub_4A1570, ff8_fonts_parse_and_render_field_texts); // Field
    replace_function(ff8_externals.parse_battle_texts1_sub_4A7250, ff8_fonts_parse_and_render_battle_texts_1); // Battle
    replace_function(ff8_externals.parse_and_render_battle_texts_hud_sub_4B0A90, ff8_fonts_parse_and_render_battle_texts_2); // Battle HUD

    // font8
    /* replace_call(0x49BAB0 + 0x23, build_icon_graphic_object_font8);
    replace_call(0x49BB30 + 0x29, build_icon_graphic_object_font8);
    replace_call(0x49C610 + 0x2D, build_icon_graphic_object_font8);
    replace_call(0x49C660 + 0x115, build_icon_graphic_object_font8);
    replace_call(0x49C660 + 0x16C, build_icon_graphic_object_font8);
    replace_call(0x49C660 + 0x210, build_icon_graphic_object_font8);
    replace_call(0x49C660 + 0x269, build_icon_graphic_object_font8);
    replace_call(0x49CB10 + 0x24, build_icon_graphic_object_font8);
    replace_call(0x49CB50 + 0x168, build_icon_graphic_object_font8);
    replace_call(0x49CB50 + 0x1AD, build_icon_graphic_object_font8);
    replace_call(0x49CB50 + 0x1D0, build_icon_graphic_object_font8);
    replace_call(0x49CB50 + 0x1F5, build_icon_graphic_object_font8);
    replace_call(0x49CB50 + 0x218, build_icon_graphic_object_font8);
    replace_call(0x49CB50 + 0x279, build_icon_graphic_object_font8);
    replace_call(0x49CB50 + 0x29B, build_icon_graphic_object_font8);
    replace_call(0x49CB50 + 0x2DD, build_icon_graphic_object_font8);
    replace_call(0x49CB50 + 0x2FF, build_icon_graphic_object_font8); */

    // Open tdw in field
    replace_call(ff8_externals.read_field_data + 0x88B, ff8_open_tdw_field);

    // Kerning
    replace_function(uint32_t(ff8_externals.get_character_width), get_character_width);

    // Convert ASCII to ff8 encoding
    replace_function(ff8_externals.convert_ascii_to_ff8enc_sub_4A2F20, convert_ascii_to_ff8_encoding_jp);
    // Cancel occidental font duo optimizations
    patch_code_dword(ff8_externals.sub_4B8B30 + 0x4B, 0x100); // Replace `current_byte >= 232` to `current_byte >= 256`
}

void ff8_load_fonts_jp(ff8_file_container *file_container, int is_exit_menu)
{
    ff8_create_graphic_object create_graphics_object_infos;
    bool is_flfifs_opened_locally = false;

    // Fake empty standard font
    if (*ff8_externals.fonts == nullptr) {
        *ff8_externals.fonts = malloc_ff8_font_structure();
    }

    if (fonts_fieldtdw_even == nullptr) {
        fonts_fieldtdw_even = malloc_ff8_font_structure();
    }
    if (fonts_fieldtdw_odd == nullptr) {
        fonts_fieldtdw_odd = malloc_ff8_font_structure();
    }

    int is_exit_menu_or_just_allocated = is_exit_menu;

    if (fonts_sysevn == nullptr) {
        fonts_sysevn = malloc_ff8_font_structure();
        fonts_sysodd = malloc_ff8_font_structure();

        is_exit_menu_or_just_allocated = 1;
    }

    free_font_graphics_object(fonts_sysevn);
    free_font_graphics_object(fonts_sysodd);

    if (graphic_object_font8_even != nullptr) {
        ff8_externals.free_graphics_object(graphic_object_font8_even);
        graphic_object_font8_even = nullptr;
    }
    if (graphic_object_font8_odd != nullptr) {
        ff8_externals.free_graphics_object(graphic_object_font8_odd);
        graphic_object_font8_odd = nullptr;
    }

    create_graphics_object_info_structure_for_font(&create_graphics_object_infos);

    if (file_container == nullptr) {
        file_container = ff8_externals.get_file_container_sub_51B410("\\MENU\\");
        is_flfifs_opened_locally = true;
    }
    create_graphics_object_infos.file_container = file_container;
    if (*ff8_externals.config_highres_font_multiplier == 2 && *ff8_externals.config_use_highres_font) { // high res
        if (is_exit_menu_or_just_allocated) {
            fonts_sysevn->field_1 = 1;
            fonts_sysevn->field_3C = 0;
            fonts_sysodd->field_1 = 1;
            fonts_sysodd->field_3C = 0;
        } else {
            fonts_sysevn->field_1 = 0;
            fonts_sysevn->field_3C = 1;
            fonts_sysodd->field_1 = 0;
            fonts_sysodd->field_3C = 1;
        }
        fonts_sysevn->graphics_object48 = ff8_create_font_graphic_object("hires\\sysevn00.tim", &create_graphics_object_infos);
        fonts_sysevn->graphics_object4C = ff8_create_font_graphic_object("hires\\sysevn01.tim", &create_graphics_object_infos);
        fonts_sysevn->graphics_object50 = ff8_create_font_graphic_object("hires\\sysevn02.tim", &create_graphics_object_infos);
        fonts_sysevn->graphics_object54 = ff8_create_font_graphic_object("hires\\sysevn03.tim", &create_graphics_object_infos);
        fonts_sysevn->field_30 = 4;
        fonts_sysevn->field_34 = 2;
        fonts_sysodd->graphics_object48 = ff8_create_font_graphic_object("hires\\sysodd00.tim", &create_graphics_object_infos);
        fonts_sysodd->graphics_object4C = ff8_create_font_graphic_object("hires\\sysodd01.tim", &create_graphics_object_infos);
        fonts_sysodd->graphics_object50 = ff8_create_font_graphic_object("hires\\sysodd02.tim", &create_graphics_object_infos);
        fonts_sysodd->graphics_object54 = ff8_create_font_graphic_object("hires\\sysodd03.tim", &create_graphics_object_infos);
        fonts_sysodd->field_30 = 4;
        fonts_sysodd->field_34 = 2;
    } else { // low res
        fonts_sysevn->graphics_object48 = ff8_create_font_graphic_object("sysfnt_even.tim", &create_graphics_object_infos);
        fonts_sysevn->field_1 = 0;
        fonts_sysevn->field_30 = 1;
        fonts_sysevn->field_34 = 1;
        fonts_sysevn->field_3C = 0;
        fonts_sysodd->graphics_object48 = ff8_create_font_graphic_object("sysfnt_odd.tim", &create_graphics_object_infos);
        fonts_sysodd->field_1 = 0;
        fonts_sysodd->field_30 = 1;
        fonts_sysodd->field_34 = 1;
        fonts_sysodd->field_3C = 0;
    }

    fill_font_structure(fonts_sysevn, 256, 252, fonts_sysodd->field_34);
    fill_font_structure(fonts_sysodd, 256, 252, fonts_sysodd->field_34);

    graphic_object_font8_even = ff8_create_font_graphic_object("font8_even.tim", &create_graphics_object_infos);
    graphic_object_font8_odd = ff8_create_font_graphic_object("font8_odd.tim", &create_graphics_object_infos);

    if (is_flfifs_opened_locally) {
        ff8_externals.free_file_container(file_container);
    }

    fonts_init_jp();
}

void ff8_load_icons_jp(ff8_file_container *file_container, int is_exit_menu)
{
    bool use_highres_font = *ff8_externals.config_use_highres_font;

    // Disable highres icons for JP version
    *ff8_externals.config_use_highres_font = false;
    ((void(*)(ff8_file_container*,int))ff8_externals.load_icons)(file_container, is_exit_menu);
    *ff8_externals.config_use_highres_font = use_highres_font;
}

void ff8_cleanup_fonts_jp()
{
    if (fonts_fieldtdw_even != nullptr) {
        free_font_graphics_object(fonts_fieldtdw_even);
        external_free(fonts_fieldtdw_even);
        fonts_fieldtdw_even = nullptr;
    }
    if (fonts_fieldtdw_odd != nullptr) {
        free_font_graphics_object(fonts_fieldtdw_odd);
        external_free(fonts_fieldtdw_odd);
        fonts_fieldtdw_odd = nullptr;
    }
    if (fonts_sysevn != nullptr) {
        free_font_graphics_object(fonts_sysevn);
        external_free(fonts_sysevn);
        fonts_sysevn = nullptr;
    }
    if (fonts_sysodd != nullptr) {
        free_font_graphics_object(fonts_sysodd);
        external_free(fonts_sysodd);
        fonts_sysodd = nullptr;
    }
    if (graphic_object_font8_even != nullptr) {
        ff8_externals.free_graphics_object(graphic_object_font8_even);
        graphic_object_font8_even = nullptr;
    }
    if (graphic_object_font8_odd != nullptr) {
        ff8_externals.free_graphics_object(graphic_object_font8_odd);
        graphic_object_font8_odd = nullptr;
    }

    ((void(*)())ff8_externals.pubintro_cleanup_textures_menu)();
}

void fonts_init()
{
    // Use hardcoded values for remastered edition
    if (ff8_remastered_edition && !ff8_enable_japanese_font) {
        replace_call(ff8_externals.sub_4972A0 + 0x16, ff8_load_fonts_hardcoded_tdw);
        replace_function(reinterpret_cast<uint32_t>(ff8_externals.get_character_width), ff8_get_character_width_hardcoded_tdw);
    }

    if (JP_VERSION || !ff8_enable_japanese_font) {
        return;
    }

    replace_function(ff8_externals.load_fonts, ff8_load_fonts_jp);

    replace_call(ff8_externals.menu_enter2 + 0x1F, ff8_load_icons_jp);
    replace_call(ff8_externals.sub_4972A0 + 0x1F, ff8_load_icons_jp);
    replace_call(ff8_externals.sub_497F20 + 0xBB, ff8_load_icons_jp);

    replace_call(ff8_externals.pubintro_cleanup_textures + 0x0, ff8_cleanup_fonts_jp);
}
