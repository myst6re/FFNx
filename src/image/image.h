/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2023 myst6re                                            //
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

#pragma once

#include <stdint.h>
#include <bimg/bimg.h>
#include <DirectXTex.h>

class Zzz;

bimg::ImageContainer *loadImageContainer(bx::AllocatorI *allocator, const char *filename, bimg::TextureFormat::Enum targetFormat = bimg::TextureFormat::Count);
// Fast PNG opening, you need to deallocate mip.m_data yourself
bool loadPng(const char *filename, bimg::ImageMip &mip, bimg::TextureFormat::Enum targetFormat = bimg::TextureFormat::Count, Zzz *zzzArchive = nullptr);
// Fast DDS opening
bool parseDds(const char *filename, DirectX::ScratchImage &image, DirectX::TexMetadata &metadata);
bimg::ImageContainer *convertDds(bx::AllocatorI *allocator, DirectX::ScratchImage &image, const DirectX::TexMetadata &metadata, bimg::TextureFormat::Enum targetFormat, int lod);
