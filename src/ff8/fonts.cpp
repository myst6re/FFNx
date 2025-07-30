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
#include "../globals.h"
#include "../patch.h"
#include "../log.h"

struct ff8_font {
    uint8_t field_0;
    uint8_t field_1;
    uint8_t field_2;
    uint8_t field_3;
    uint32_t field_4;
    uint32_t field_8;
    uint32_t field_C;
    uint32_t field_10;
    uint32_t field_14;
    uint32_t field_18;
    uint32_t field_1C;
    uint32_t field_20;
    uint32_t field_24;
    uint32_t field_28;
    uint32_t field_2C;
    uint32_t field_30;
    uint32_t field_34;
    uint32_t field_38;
    uint8_t field_3C;
    uint8_t field_3D;
    uint16_t field_3E;
    uint16_t field_40;
    uint16_t field_42;
    uint32_t field_44;
    ff8_graphics_object *graphics_object48;
    ff8_graphics_object *graphics_object4C;
    ff8_graphics_object *graphics_object50;
    ff8_graphics_object *graphics_object54;
};

ff8_font *fonts_fieldtdw_even = nullptr;
ff8_font *fonts_fieldtdw_odd = nullptr;
ff8_font *fonts_sysevn = nullptr;
ff8_font *fonts_sysodd = nullptr;
ff8_graphics_object *graphic_object_font8_even = nullptr;
ff8_graphics_object *graphic_object_font8_odd = nullptr;

ff8_font *malloc_ff8_font_structure()
{
    ff8_font *font = (ff8_font *)external_malloc(sizeof(ff8_font));
    font->graphics_object48 = nullptr;
    font->graphics_object4C = nullptr;
    font->graphics_object50 = nullptr;
    font->graphics_object54 = nullptr;

    return font;
}

ff8_graphics_object *ff8_create_font_graphic_object(const char *path, ff8_create_graphic_object *create_graphics_object_infos)
{
    char buffer[MAX_PATH] = {};

    if (create_graphics_object_infos->file_container != nullptr) {
        sprintf(buffer, "c:%s%s", ff8_externals.archive_path_prefix_menu, path);
    } else {
        sprintf(buffer, "%s%s", ff8_externals.archive_path_prefix_menu, path);
    }

    void *dword_2230220 = *(void **)0x2230220;

    return ((ff8_graphics_object*(*)(int,int,ff8_create_graphic_object*,char*,void*))(ff8_externals._load_texture))(1, 12, create_graphics_object_infos, buffer, dword_2230220);
}

void ff8_load_fonts(ff8_file_container *file_container, int is_exit_menu)
{
    ((void(*)(ff8_file_container*,int))ff8_externals.load_fonts)(file_container, is_exit_menu);

    ff8_create_graphic_object create_graphics_object_infos;
    bool is_flfifs_opened_locally = false;

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

    if (fonts_sysevn->graphics_object48 != nullptr) {
        ff8_externals.free_graphics_object(fonts_sysevn->graphics_object48);
        fonts_sysevn->graphics_object48 = nullptr;
    }
    if (fonts_sysevn->graphics_object4C != nullptr) {
        ff8_externals.free_graphics_object(fonts_sysevn->graphics_object4C);
        fonts_sysevn->graphics_object4C = nullptr;
    }
    if (fonts_sysevn->graphics_object50 != nullptr) {
        ff8_externals.free_graphics_object(fonts_sysevn->graphics_object50);
        fonts_sysevn->graphics_object50 = nullptr;
    }
    if (fonts_sysevn->graphics_object54 != nullptr) {
        ff8_externals.free_graphics_object(fonts_sysevn->graphics_object54);
        fonts_sysevn->graphics_object54 = nullptr;
    }
    if (fonts_sysodd->graphics_object48 != nullptr) {
        ff8_externals.free_graphics_object(fonts_sysodd->graphics_object48);
        fonts_sysodd->graphics_object48 = nullptr;
    }
    if (fonts_sysodd->graphics_object4C != nullptr) {
        ff8_externals.free_graphics_object(fonts_sysodd->graphics_object4C);
        fonts_sysodd->graphics_object4C = nullptr;
    }
    if (fonts_sysodd->graphics_object50 != nullptr) {
        ff8_externals.free_graphics_object(fonts_sysodd->graphics_object50);
        fonts_sysodd->graphics_object50 = nullptr;
    }
    if (fonts_sysodd->graphics_object54 != nullptr) {
        ff8_externals.free_graphics_object(fonts_sysodd->graphics_object54);
        fonts_sysodd->graphics_object54 = nullptr;
    }
    if (graphic_object_font8_even != nullptr) {
        ff8_externals.free_graphics_object(graphic_object_font8_even);
        graphic_object_font8_even = nullptr;
    }
    if (graphic_object_font8_odd != nullptr) {
        ff8_externals.free_graphics_object(graphic_object_font8_odd);
        graphic_object_font8_odd = nullptr;
    }

    ff8_externals.create_graphics_object_info_structure(4, &create_graphics_object_infos);
    create_graphics_object_infos.field_7C |= 0x80u;
    create_graphics_object_infos.flags |= 0x11u;
    create_graphics_object_infos.field_18 = 0; // graphics_instance_dword_2230224;
    if (file_container == nullptr) {
        //file_container = menu_fifls_struct_sub_51F2D0("\\MENU\\");
        is_flfifs_opened_locally = true;
    }
    create_graphics_object_infos.file_container = file_container;
    if (true /* dword_D8A360 == 2 && reg_graphics_flag_20_highres_font_byte_2230214 */) { // high res
        if (is_exit_menu_or_just_allocated) {
            fonts_sysevn->field_1 = 1;
            fonts_sysevn->field_3C = 0;
            fonts_sysevn->field_3D = 0;
            fonts_sysodd->field_1 = 1;
            fonts_sysodd->field_3C = 0;
        } else {
            fonts_sysevn->field_1 = 0;
            fonts_sysevn->field_3C = 1;
            fonts_sysevn->field_3D = 0;
            fonts_sysodd->field_1 = 0;
            fonts_sysodd->field_3C = 1;
        }
        fonts_sysodd->field_3D = 0;
        fonts_sysevn->graphics_object48 = ff8_create_font_graphic_object("hires\\sysevn00.tim", &create_graphics_object_infos);
        fonts_sysevn->graphics_object4C = ff8_create_font_graphic_object("hires\\sysevn01.tim", &create_graphics_object_infos);
        fonts_sysevn->graphics_object50 = ff8_create_font_graphic_object("hires\\sysevn02.tim", &create_graphics_object_infos);
        fonts_sysevn->graphics_object54 = ff8_create_font_graphic_object("hires\\sysevn03.tim", &create_graphics_object_infos);
        fonts_sysevn->field_4 = 512.0;
        fonts_sysevn->field_8 = 504.0;
        fonts_sysevn->field_30 = 4;
        fonts_sysevn->field_34 = 2;
        fonts_sysevn->field_38 = 2;
        fonts_sysodd->graphics_object48 = ff8_create_font_graphic_object("hires\\sysodd00.tim", &create_graphics_object_infos);
        fonts_sysodd->graphics_object4C = ff8_create_font_graphic_object("hires\\sysodd01.tim", &create_graphics_object_infos);
        fonts_sysodd->graphics_object50 = ff8_create_font_graphic_object("hires\\sysodd02.tim", &create_graphics_object_infos);
        fonts_sysodd->graphics_object54 = ff8_create_font_graphic_object("hires\\sysodd03.tim", &create_graphics_object_infos);
        fonts_sysodd->field_4 = 512.0;
        fonts_sysodd->field_8 = 504.0;
        fonts_sysodd->field_30 = 4;
        fonts_sysodd->field_34 = 2;
        fonts_sysodd->field_38 = 2;
    } else { // low res
        fonts_sysevn->graphics_object48 = ff8_create_font_graphic_object("sysfnt_even.tim", &create_graphics_object_infos);
        fonts_sysevn->field_1 = 0;
        fonts_sysevn->field_3C = 0;
        fonts_sysevn->field_3D = 0;
        fonts_sysevn->field_4 = 256.0;
        fonts_sysevn->field_8 = 252.0;
        fonts_sysevn->field_30 = 1;
        fonts_sysevn->field_34 = 1;
        fonts_sysevn->field_38 = 1;
        fonts_sysodd->graphics_object48 = ff8_create_font_graphic_object("sysfnt_odd.tim", &create_graphics_object_infos);
        fonts_sysodd->field_1 = 0;
        fonts_sysodd->field_3C = 0;
        fonts_sysodd->field_3D = 0;
        fonts_sysodd->field_4 = 256.0;
        fonts_sysodd->field_8 = 252.0;
        fonts_sysodd->field_30 = 1;
        fonts_sysodd->field_34 = 1;
        fonts_sysodd->field_38 = 1;
    }
    fonts_sysevn->field_0 = 0;
    fonts_sysevn->field_14 = 4;
    fonts_sysevn->field_3E = ((ff8_tex_header *)(((ff8_texture_set *)(fonts_sysevn->graphics_object48->hundred_data->texture_set))->tex_header))->field_DC;
    fonts_sysevn->field_40 = ((ff8_tex_header *)(((ff8_texture_set *)(fonts_sysevn->graphics_object48->hundred_data->texture_set))->tex_header))->field_E0;
    fonts_sysevn->field_C = 1.0 / fonts_sysevn->field_4;
    fonts_sysevn->field_10 = 1.0 / fonts_sysevn->field_8;
    fonts_sysevn->field_18 = 256.0;
    fonts_sysevn->field_1C = 252.0;
    fonts_sysevn->field_20 = 1.0 / fonts_sysevn->field_18;
    fonts_sysevn->field_24 = 1.0 / fonts_sysevn->field_1C;
    fonts_sysevn->field_28 = fonts_sysevn->field_4 / fonts_sysevn->field_18;
    fonts_sysevn->field_2C = fonts_sysevn->field_8 / fonts_sysevn->field_1C;
    fonts_sysodd->field_0 = 0;
    fonts_sysodd->field_14 = 4;
    fonts_sysodd->field_3E = ((ff8_tex_header *)(((ff8_texture_set *)(fonts_sysodd->graphics_object48->hundred_data->texture_set))->tex_header))->field_DC;
    fonts_sysodd->field_40 = ((ff8_tex_header *)(((ff8_texture_set *)(fonts_sysodd->graphics_object48->hundred_data->texture_set))->tex_header))->field_E0;
    fonts_sysodd->field_C = 1.0 / fonts_sysodd->field_4;
    fonts_sysodd->field_10 = 1.0 / fonts_sysodd->field_8;
    fonts_sysodd->field_18 = 256.0;
    fonts_sysodd->field_1C = 252.0;
    fonts_sysodd->field_20 = 1.0 / fonts_sysodd->field_18;
    fonts_sysodd->field_24 = 1.0 / fonts_sysodd->field_1C;
    fonts_sysodd->field_28 = fonts_sysodd->field_4 / fonts_sysodd->field_18;
    fonts_sysodd->field_2C = fonts_sysodd->field_8 / fonts_sysodd->field_1C;
    graphic_object_font8_even = ff8_create_font_graphic_object("font8_even.tim", &create_graphics_object_infos);
    graphic_object_font8_odd = ff8_create_font_graphic_object("font8_odd.tim", &create_graphics_object_infos);
    if (is_flfifs_opened_locally) {
        //clean_close_free_flfifs_struct_sub_51F390(file_container);
    }
}

void ff8_cleanup_fonts()
{
    ((void(*)())ff8_externals.pubintro_cleanup_textures_menu)();

    if (fonts_fieldtdw_even != nullptr) {
        if (fonts_fieldtdw_even->graphics_object48 != nullptr) {
            ff8_externals.free_graphics_object(fonts_fieldtdw_even->graphics_object48);
            fonts_fieldtdw_even->graphics_object48 = nullptr;
        }
        if (fonts_fieldtdw_even->graphics_object4C != nullptr) {
            ff8_externals.free_graphics_object(fonts_fieldtdw_even->graphics_object4C);
            fonts_fieldtdw_even->graphics_object4C = nullptr;
        }
        if (fonts_fieldtdw_even->graphics_object50 != nullptr)
        {
            ff8_externals.free_graphics_object(fonts_fieldtdw_even->graphics_object50);
            fonts_fieldtdw_even->graphics_object50 = nullptr;
        }
        if (fonts_fieldtdw_even->graphics_object54 != nullptr)
        {
            ff8_externals.free_graphics_object(fonts_fieldtdw_even->graphics_object54);
            fonts_fieldtdw_even->graphics_object54 = nullptr;
        }
        external_free(fonts_fieldtdw_even);
        fonts_fieldtdw_even = nullptr;
    }
    if (fonts_fieldtdw_odd != nullptr) {
        if (fonts_fieldtdw_odd->graphics_object48 != nullptr) {
            ff8_externals.free_graphics_object(fonts_fieldtdw_odd->graphics_object48);
        }
        fonts_fieldtdw_odd->graphics_object48 = nullptr;
        if (fonts_fieldtdw_odd->graphics_object4C != nullptr)
        {
            ff8_externals.free_graphics_object(fonts_fieldtdw_odd->graphics_object4C);
        }
        fonts_fieldtdw_odd->graphics_object4C = nullptr;
        if (fonts_fieldtdw_odd->graphics_object50 != nullptr)
        {
            ff8_externals.free_graphics_object(fonts_fieldtdw_odd->graphics_object50);
        }
        fonts_fieldtdw_odd->graphics_object50 = nullptr;
        if (fonts_fieldtdw_odd->graphics_object54 != nullptr)
        {
            ff8_externals.free_graphics_object(fonts_fieldtdw_odd->graphics_object54);
        }
        fonts_fieldtdw_odd->graphics_object54 = nullptr;
        external_free(fonts_fieldtdw_odd);
        fonts_fieldtdw_odd = nullptr;
    }
    if (fonts_sysevn != nullptr) {
        if (fonts_sysevn->graphics_object48 != nullptr)
        {
            ff8_externals.free_graphics_object(fonts_sysevn->graphics_object48);
            fonts_sysevn->graphics_object48 = nullptr;
        }
        if (fonts_sysevn->graphics_object4C != nullptr)
        {
            ff8_externals.free_graphics_object(fonts_sysevn->graphics_object4C);
            fonts_sysevn->graphics_object4C = nullptr;
        }
        if (fonts_sysevn->graphics_object50 != nullptr)
        {
            ff8_externals.free_graphics_object(fonts_sysevn->graphics_object50);
            fonts_sysevn->graphics_object50 = nullptr;
        }
        if (fonts_sysevn->graphics_object54 != nullptr)
        {
            ff8_externals.free_graphics_object(fonts_sysevn->graphics_object54);
            fonts_sysevn->graphics_object54 = nullptr;
        }
        external_free(fonts_sysevn);
        fonts_sysevn = nullptr;
    }
    if (fonts_sysodd != nullptr) {
        if (fonts_sysodd->graphics_object48 != nullptr) {
            ff8_externals.free_graphics_object(fonts_sysodd->graphics_object48);
            fonts_sysodd->graphics_object48 = nullptr;
        }
        if (fonts_sysodd->graphics_object4C != nullptr) {
            ff8_externals.free_graphics_object(fonts_sysodd->graphics_object4C);
            fonts_sysodd->graphics_object4C = nullptr;
        }
        if (fonts_sysodd->graphics_object50 != nullptr) {
            ff8_externals.free_graphics_object(fonts_sysodd->graphics_object50);
            fonts_sysodd->graphics_object50 = nullptr;
        }
        if (fonts_sysodd->graphics_object54 != nullptr) {
            ff8_externals.free_graphics_object(fonts_sysodd->graphics_object54);
            fonts_sysodd->graphics_object54 = nullptr;
        }
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

void ff8_fonts_reset_field_58()
{
    // TODO: call sub_4B7CC0()

    reset_font_graphics_object_field_58(fonts_fieldtdw_even);
    reset_font_graphics_object_field_58(fonts_fieldtdw_odd);

    reset_font_graphics_object_field_58(fonts_sysevn);
    reset_font_graphics_object_field_58(fonts_sysodd);

    reset_graphics_object_field_58(graphic_object_font8_even);
    reset_graphics_object_field_58(graphic_object_font8_odd);
}

void draw_graphics_object(ff8_graphics_object *graphic_object, game_obj *game_object)
{
    //if (graphic_object != nullptr) graphics_setrenderstate_draw_sub_418307(graphic_object, game_object);
}

void draw_font(ff8_font *font, game_obj *game_object)
{
    draw_graphics_object(font->graphics_object48, game_object);
    draw_graphics_object(font->graphics_object4C, game_object);
    draw_graphics_object(font->graphics_object50, game_object);
    draw_graphics_object(font->graphics_object54, game_object);
}

void ff8_fonts_draw()
{
    game_obj *game_object = common_externals.get_game_object();

    draw_font(fonts_fieldtdw_even, game_object);
    draw_font(fonts_fieldtdw_odd, game_object);

    draw_font(fonts_sysevn, game_object);
    draw_font(fonts_sysodd, game_object);

    draw_graphics_object(graphic_object_font8_even, game_object);
    draw_graphics_object(graphic_object_font8_odd, game_object);

    // TODO: call sub_4B7C40()
}

void fonts_init()
{
    if (!JP_VERSION) {
        replace_call(ff8_externals.menu_enter2 + 0x16, ff8_load_fonts);
        replace_call(ff8_externals.sub_4972A0 + 0x16, ff8_load_fonts);
        replace_call(ff8_externals.sub_497F20 + 0xB3, ff8_load_fonts);
        replace_call(ff8_externals.pubintro_cleanup_textures + 0x0, ff8_cleanup_fonts);
    }
}
