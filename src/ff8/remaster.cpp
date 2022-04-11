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

#include "../globals.h"
#include "../log.h"
#include "remaster.h"

Zzz g_FF8ZzzArchiveMain;
Zzz g_FF8ZzzArchiveOther;

void ff8_remaster_init()
{
    char fullpath[MAX_PATH];

    snprintf(fullpath, sizeof(fullpath), "%s/main.zzz", basedir);
    errno_t err = g_FF8ZzzArchiveMain.open(fullpath);

    if (err != 0) {
        ffnx_error("%s: cannot open main.zzz (error code %d)\n", __func__, err);
        exit(1);
    }

    snprintf(fullpath, sizeof(fullpath), "%s/other.zzz", basedir);

    err = g_FF8ZzzArchiveOther.open(fullpath);

    if (err != 0) {
        ffnx_error("%s: cannot open other.zzz (error code %d)\n", __func__, err);
        exit(1);
    }
}
