/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2024 myst6re                                            //
//    Copyright (C) 2024 Julian Xhokaxhiu                                   //
//    Copyright (C) 2023 Cosmos                                             //
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

#include "trace.h"
#include "../../log.h"
#include "../../patch.h"

int bdlinktask2(uint32_t *a1, int cb)
{
	ffnx_trace("%s: cb=0x%X\n", __func__, cb);

	int v2 = ((int(*)(uint32_t*))0x507D70)(a1);

	if (!v2)
	{
		ffnx_error("%s: FAILED !!!!\n", __func__);

		return 0;
	}

	*(BYTE *)v2 |= 1u;
	*(DWORD *)(v2 + 8) = cb;
	int v3 = a1[1];
	*(WORD *)(v2 + 2) = 0;
	*(DWORD *)(v2 + 4) = 0;
	if ( v3 )
		*(DWORD *)(v3 + 4) = v2;
	else
		*a1 = v2;
	a1[1] = v2;

	return v2;
}

int bdlinktask3(uint32_t *a1, int cb)
{
	ffnx_trace("%s: cb=0x%X\n", __func__, cb);

	return ((int(*)(uint32_t*, int))0x507DA0)(a1, cb);
}

void *battle_set_texture_action_upload1_sub_5057A0_call1(DWORD *pos_and_size, char *texture_data)
{
	int *dword1D980F8 = (int *)0x1D980F8;

	ffnx_trace("%s: number=%d\n", __func__, *dword1D980F8);

	return ((void*(*)(DWORD*, char*))0x5057A0)(pos_and_size, texture_data);
}

void *battle_set_texture_action_upload1_sub_5057A0_call2(DWORD *pos_and_size, char *texture_data)
{
	int *dword1D980F8 = (int *)0x1D980F8;

	ffnx_trace("%s: number=%d\n", __func__, *dword1D980F8);

	return ((void*(*)(DWORD*, char*))0x5057A0)(pos_and_size, texture_data);
}

void *battle_set_texture_action_upload1_sub_5057A0_call3(DWORD *pos_and_size, char *texture_data)
{
	int *dword1D980F8 = (int *)0x1D980F8;

	ffnx_trace("%s: number=%d\n", __func__, *dword1D980F8);

	return ((void*(*)(DWORD*, char*))0x5057A0)(pos_and_size, texture_data);
}

void *battle_set_texture_action_upload1_sub_5057A0_call4(DWORD *pos_and_size, char *texture_data)
{
	int *dword1D980F8 = (int *)0x1D980F8;
	int16_t *dword2797128 = (int16_t *)(*(int *)0x2797128);
	int16_t *dword279712A = (int16_t *)(*(int *)0x2797128 + 2);
	BYTE thirdNumber = *(BYTE *)(*(DWORD *)(*(int *)0x27970C4 + 164) + *dword279712A) & 0x7F;

	ffnx_trace("%s: number=%d othernumber=%d thirdNumber=%d dword2797128=%d\n", __func__, *dword1D980F8, *dword279712A, thirdNumber, *dword2797128);

	return ((void*(*)(DWORD*, char*))0xB66740)(pos_and_size, texture_data);
}

void *battle_set_texture_action_upload1_sub_5057A0_call5(DWORD *pos_and_size, char *texture_data)
{
	int16_t *dword2797128 = (int16_t *)(*(int *)0x2797128);
	int16_t *dword279712A = (int16_t *)(*(int *)0x2797128 + 2);
	BYTE thirdNumber = *(uint8_t *)(*(DWORD *)(*(int *)0x27970C4 + 168) + *dword279712A);

	ffnx_trace("%s: palette othernumber=%d thirdNumber=%d dword2797128=%d\n", __func__, *dword279712A, thirdNumber, *dword2797128);

	return ((void*(*)(DWORD*, char*))0xB66740)(pos_and_size, texture_data);
}


void(*effect_funcs[327])() = {};

#define effectFunc(NUM) \
	void effect##NUM() \
	{ \
		int texture_data_related_dword_27970C4 = *(int *)0x27970C4; \
		ffnx_info("%s: texture_data=0x%X\n", __func__, *(char **)(texture_data_related_dword_27970C4 + 180)); \
		effect_funcs[NUM](); \
	}
#define effectReplace(NUM) \
	if (int(effect_funcs[NUM]) > 0xAF0000) {\
		effect_funcs2[NUM] = effect##NUM; \
	}

effectFunc(0)
effectFunc(1)
effectFunc(2)
effectFunc(3)
effectFunc(4)
effectFunc(5)
effectFunc(6)
effectFunc(7)
effectFunc(8)
effectFunc(9)
effectFunc(10)
effectFunc(11)
effectFunc(12)
effectFunc(13)
effectFunc(14)
effectFunc(15)
effectFunc(16)
effectFunc(17)
effectFunc(18)
effectFunc(19)
effectFunc(20)
effectFunc(21)
effectFunc(22)
effectFunc(23)
effectFunc(24)
effectFunc(25)
effectFunc(26)
effectFunc(27)
effectFunc(28)
effectFunc(29)
effectFunc(30)
effectFunc(31)
effectFunc(32)
effectFunc(33)
effectFunc(34)
effectFunc(35)
effectFunc(36)
effectFunc(37)
effectFunc(38)
effectFunc(39)
effectFunc(40)
effectFunc(41)
effectFunc(42)
effectFunc(43)
effectFunc(44)
effectFunc(45)
effectFunc(46)
effectFunc(47)
effectFunc(48)
effectFunc(49)
effectFunc(50)
effectFunc(51)
effectFunc(52)
effectFunc(53)
effectFunc(54)
effectFunc(55)
effectFunc(56)
effectFunc(57)
effectFunc(58)
effectFunc(59)
effectFunc(60)
effectFunc(61)
effectFunc(62)
effectFunc(63)
effectFunc(64)
effectFunc(65)
effectFunc(66)
effectFunc(67)
effectFunc(68)
effectFunc(69)
effectFunc(70)
effectFunc(71)
effectFunc(72)
effectFunc(73)
effectFunc(74)
effectFunc(75)
effectFunc(76)
effectFunc(77)
effectFunc(78)
effectFunc(79)
effectFunc(80)
effectFunc(81)
effectFunc(82)
effectFunc(83)
effectFunc(84)
effectFunc(85)
effectFunc(86)
effectFunc(87)
effectFunc(88)
effectFunc(89)
effectFunc(90)
effectFunc(91)
effectFunc(92)
effectFunc(93)
effectFunc(94)
effectFunc(95)
effectFunc(96)
effectFunc(97)
effectFunc(98)
effectFunc(99)
effectFunc(100)
effectFunc(101)
effectFunc(102)
effectFunc(103)
effectFunc(104)
effectFunc(105)
effectFunc(106)
effectFunc(107)
effectFunc(108)
effectFunc(109)
effectFunc(110)
effectFunc(111)
effectFunc(112)
effectFunc(113)
effectFunc(114)
effectFunc(115)
effectFunc(116)
effectFunc(117)
effectFunc(118)
effectFunc(119)
effectFunc(120)
effectFunc(121)
effectFunc(122)
effectFunc(123)
effectFunc(124)
effectFunc(125)
effectFunc(126)
effectFunc(127)
effectFunc(128)
effectFunc(129)
effectFunc(130)
effectFunc(131)
effectFunc(132)
effectFunc(133)
effectFunc(134)
effectFunc(135)
effectFunc(136)
effectFunc(137)
effectFunc(138)
effectFunc(139)
effectFunc(140)
effectFunc(141)
effectFunc(142)
effectFunc(143)
effectFunc(144)
effectFunc(145)
effectFunc(146)
effectFunc(147)
effectFunc(148)
effectFunc(149)
effectFunc(150)
effectFunc(151)
effectFunc(152)
effectFunc(153)
effectFunc(154)
effectFunc(155)
effectFunc(156)
effectFunc(157)
effectFunc(158)
effectFunc(159)
effectFunc(160)
effectFunc(161)
effectFunc(162)
effectFunc(163)
effectFunc(164)
effectFunc(165)
effectFunc(166)
effectFunc(167)
effectFunc(168)
effectFunc(169)
effectFunc(170)
effectFunc(171)
effectFunc(172)
effectFunc(173)
effectFunc(174)
effectFunc(175)
effectFunc(176)
effectFunc(177)
effectFunc(178)
effectFunc(179)
effectFunc(180)
effectFunc(181)
effectFunc(182)
effectFunc(183)
effectFunc(184)
effectFunc(185)
effectFunc(186)
effectFunc(187)
effectFunc(188)
effectFunc(189)
effectFunc(190)
effectFunc(191)
effectFunc(192)
effectFunc(193)
effectFunc(194)
effectFunc(195)
effectFunc(196)
effectFunc(197)
effectFunc(198)
effectFunc(199)
effectFunc(200)
effectFunc(201)
effectFunc(202)
effectFunc(203)
effectFunc(204)
effectFunc(205)
effectFunc(206)
effectFunc(207)
effectFunc(208)
effectFunc(209)
effectFunc(210)
effectFunc(211)
effectFunc(212)
effectFunc(213)
effectFunc(214)
effectFunc(215)
effectFunc(216)
effectFunc(217)
effectFunc(218)
effectFunc(219)
effectFunc(220)
effectFunc(221)
effectFunc(222)
effectFunc(223)
effectFunc(224)
effectFunc(225)
effectFunc(226)
effectFunc(227)
effectFunc(228)
effectFunc(229)
effectFunc(230)
effectFunc(231)
effectFunc(232)
effectFunc(233)
effectFunc(234)
effectFunc(235)
effectFunc(236)
effectFunc(237)
effectFunc(238)
effectFunc(239)
effectFunc(240)
effectFunc(241)
effectFunc(242)
effectFunc(243)
effectFunc(244)
effectFunc(245)
effectFunc(246)
effectFunc(247)
effectFunc(248)
effectFunc(249)
effectFunc(250)
effectFunc(251)
effectFunc(252)
effectFunc(253)
effectFunc(254)
effectFunc(255)
effectFunc(256)
effectFunc(257)
effectFunc(258)
effectFunc(259)
effectFunc(260)
effectFunc(261)
effectFunc(262)
effectFunc(263)
effectFunc(264)
effectFunc(265)
effectFunc(266)
effectFunc(267)
effectFunc(268)
effectFunc(269)
effectFunc(270)
effectFunc(271)
effectFunc(272)
effectFunc(273)
effectFunc(274)
effectFunc(275)
effectFunc(276)
effectFunc(277)
effectFunc(278)
effectFunc(279)
effectFunc(280)
effectFunc(281)
effectFunc(282)
effectFunc(283)
effectFunc(284)
effectFunc(285)
effectFunc(286)
effectFunc(287)
effectFunc(288)
effectFunc(289)
effectFunc(290)
effectFunc(291)
effectFunc(292)
effectFunc(293)
effectFunc(294)
effectFunc(295)
effectFunc(296)
effectFunc(297)
effectFunc(298)
effectFunc(299)
effectFunc(300)
effectFunc(301)
effectFunc(302)
effectFunc(303)
effectFunc(304)
effectFunc(305)
effectFunc(306)
effectFunc(307)
effectFunc(308)
effectFunc(309)
effectFunc(310)
effectFunc(311)
effectFunc(312)
effectFunc(313)
effectFunc(314)
effectFunc(315)
effectFunc(316)
effectFunc(317)
effectFunc(318)
effectFunc(319)
effectFunc(320)
effectFunc(321)
effectFunc(322)
effectFunc(323)
effectFunc(324)
effectFunc(325)
effectFunc(326)
effectFunc(327)

struct struc_46_menu_callbacks {
    uint32_t controller_callback;
    uint32_t field_4;
    uint32_t renderer_callback;
    uint32_t other_renderer_callback;
    uint8_t field_10;
    uint8_t field_11;
    uint8_t field_12;
    uint8_t field_13;
};

void battle_set_controller_render_sub_4B9440(int id, int controller_callback, int renderer_callback, int other_renderer_callback)
{
    ffnx_trace("%s: id=%d (%X, %X, %X)\n", __func__, id, controller_callback, renderer_callback, other_renderer_callback);

    struc_46_menu_callbacks *battle_menu_callbacks_array_unk_1D76300 = (struc_46_menu_callbacks *)0x1D76300;

    struc_46_menu_callbacks *v4 = &battle_menu_callbacks_array_unk_1D76300[id];
    v4->field_10 = -1;
    v4->field_11 = -1;
    v4->field_12 = 0;
    v4->controller_callback = controller_callback;
    v4->renderer_callback = renderer_callback;
    v4->other_renderer_callback = other_renderer_callback;
}

int ff8_battle_sub_500510(int a1)
{
	ffnx_trace("%s: a1=%d\n", __func__, a1);

	int ret = ((int(*)(int))0x500510)(a1);

	ffnx_trace("/%s: a1=%d\n", __func__, a1);

	return ret;
}

int ff8_battle_sub_502460(int a1)
{
	ffnx_trace("%s: a1=0x%X\n", __func__, a1);

	int ret = ((int(*)(int))0x502460)(a1);

	ffnx_trace("/%s: a1=%d\n", __func__, a1);

	return ret;
}

int ff8_battle_sub_507FA0(int a1)
{
	ffnx_trace("%s: a1=%d\n", __func__, a1);

	int ret = ((int(*)(int))0x507FA0)(a1);

	ffnx_trace("/%s: a1=%d\n", __func__, a1);

	return ret;
}

int fileSize = 0;

int open_file_0(int fileId, void *data, int unused, int callback)
{
	//ffnx_trace("%s: fileId=%d data=0x%X\n", __func__, fileId, data);
	fileSize = 0;
	int ret = ((int(*)(int,void*,int,int))0x48D0C0)(fileId, data, unused, callback);

	ffnx_trace("%s: fileId=%d data=0x%X fileSize=%d dataEnd=0x%X\n", __func__, fileId, data, fileSize, (uint8_t *)data + fileSize);

	return ret;
}

int battle_file_get_file_size(int id, int pos, int whence)
{
	int ret = ((int(*)(int,int,int))0x51B770)(id, pos, whence);

	fileSize = ret;

	return ret;
}

int *summon_sub_56C670(int16_t *a1, int16_t *a2, int *a3)
{
	ffnx_trace("%s: a1=0x%X a2=0x%X a3=0x%X\n", __func__, a1, a2, a3);

	return ((int*(*)(int16_t*,int16_t*,int*))0x56C670)(a1, a2, a3);
}

uint8_t *remaster_crash_v3 = 0;

void remaster_crash_sub_508640(int a1)
{
	uint8_t **remaster_crash_v2 = *(uint8_t ***)(a1 + 4);
	remaster_crash_v3 = *remaster_crash_v2;
	ffnx_trace("%s: a1=0x%X remaster_crash_v2=0x%X remaster_crash_v3=0x%X remaster_crash_v3_value=%d\n", __func__, a1, remaster_crash_v2, remaster_crash_v3, *remaster_crash_v3);

	((void(*)(int))0x508640)(a1);
}

void freeze_sub_508910(int a1)
{
	ffnx_trace("%s: a1=0x%X\n", __func__, a1);
	((void(*)(int))0x508910)(a1);
}

int *sub_56D1B0(int16_t *a1, int *a2)
{
	uint8_t *v4 = (uint8_t*)a1 + 32;
	int16_t index_v5 = *((int16_t *)v4 - 18);
	int remaster_crash_1_v7 = remaster_crash_v3 ? (int)&remaster_crash_v3[48 * index_v5 + 16] : 42;
	ffnx_trace("%s: a1=%X a2=%X v4=%X index_v5=%d remaster_crash_1_v7=0x%X remaster_crash_v3_value=%d\n", __func__, a1, a2, v4, index_v5, remaster_crash_1_v7, remaster_crash_v3 ? *remaster_crash_v3 : 0);

	return ((int*(*)(int16_t*,int*))0x56D1B0)(a1, a2);
}

void sub_56C270(int16_t *a1, int *a2)
{
	ffnx_trace("%s: a1=%X a2=%X\n", __func__, a1, a2);

	((void(*)(int16_t*,int*))0x56C270)(a1, a2);
}

void ff8_battle_trace_init()
{
	replace_function(0x507D10, bdlinktask2);
	replace_call(0x506260 + 0x20, bdlinktask3);
    replace_function(0x4B9440, battle_set_controller_render_sub_4B9440);
	replace_call(0x507E30 + 0x1B, open_file_0);
	replace_call(0x52CDB0 + 0x77, battle_file_get_file_size);
	replace_call(0x508640 + 0xDF, summon_sub_56C670);
	replace_call(0x508910 + 0x9, remaster_crash_sub_508640);
	replace_call(0x502460 + 0x125, freeze_sub_508910);
	replace_call(0x508640 + 0x48, sub_56D1B0);
	replace_call(0x508640 + 0x6B, sub_56C270);
	/* replace_call(0xAFDC20 + 0x3B, battle_set_texture_action_upload1_sub_5057A0_call1);
	replace_call(0xAFDC20 + 0x85, battle_set_texture_action_upload1_sub_5057A0_call2);
	replace_call(0xAFDC20 + 0xB6, battle_set_texture_action_upload1_sub_5057A0_call3);
	replace_call(0xAFE110 + 0x22, battle_set_texture_action_upload1_sub_5057A0_call4);
	replace_call(0xAFE160 + 0x2F, battle_set_texture_action_upload1_sub_5057A0_call5); */
	patch_code_dword(0x500360 + 0x9F, uint32_t(ff8_battle_sub_500510));
	patch_code_dword(0x502180 + 0x6, uint32_t(ff8_battle_sub_502460));
	patch_code_dword(0x507F80 + 0x1, uint32_t(ff8_battle_sub_507FA0));

	//void *func_pointer = (void*)0x1871EFC; // taurus
	//void *func_pointer = (void*)0x18735D8; // cerberus
	void *func_pointer = (void *)0x1877914; // leviathan

	memcpy(effect_funcs, func_pointer, 327 * sizeof(uint32_t));

	void(**effect_funcs2)() = (void(**)())func_pointer;

	effectReplace(0)
	effectReplace(1)
	effectReplace(2)
	effectReplace(3)
	effectReplace(4)
	effectReplace(5)
	effectReplace(6)
	effectReplace(7)
	effectReplace(8)
	effectReplace(9)
	effectReplace(10)
	effectReplace(11)
	effectReplace(12)
	effectReplace(13)
	effectReplace(14)
	effectReplace(15)
	effectReplace(16)
	effectReplace(17)
	effectReplace(18)
	effectReplace(19)
	effectReplace(20)
	effectReplace(21)
	effectReplace(22)
	effectReplace(23)
	effectReplace(24)
	effectReplace(25)
	effectReplace(26)
	effectReplace(27)
	effectReplace(28)
	effectReplace(29)
	effectReplace(30)
	effectReplace(31)
	effectReplace(32)
	effectReplace(33)
	effectReplace(34)
	effectReplace(35)
	effectReplace(36)
	effectReplace(37)
	effectReplace(38)
	effectReplace(39)
	effectReplace(40)
	effectReplace(41)
	effectReplace(42)
	effectReplace(43)
	effectReplace(44)
	effectReplace(45)
	effectReplace(46)
	effectReplace(47)
	effectReplace(48)
	effectReplace(49)
	effectReplace(50)
	effectReplace(51)
	effectReplace(52)
	effectReplace(53)
	effectReplace(54)
	effectReplace(55)
	effectReplace(56)
	effectReplace(57)
	effectReplace(58)
	effectReplace(59)
	effectReplace(60)
	effectReplace(61)
	effectReplace(62)
	effectReplace(63)
	effectReplace(64)
	effectReplace(65)
	effectReplace(66)
	effectReplace(67)
	effectReplace(68)
	effectReplace(69)
	effectReplace(70)
	effectReplace(71)
	effectReplace(72)
	effectReplace(73)
	effectReplace(74)
	effectReplace(75)
	effectReplace(76)
	effectReplace(77)
	effectReplace(78)
	effectReplace(79)
	effectReplace(80)
	effectReplace(81)
	effectReplace(82)
	effectReplace(83)
	effectReplace(84)
	effectReplace(85)
	effectReplace(86)
	effectReplace(87)
	effectReplace(88)
	effectReplace(89)
	effectReplace(90)
	effectReplace(91)
	effectReplace(92)
	effectReplace(93)
	effectReplace(94)
	effectReplace(95)
	effectReplace(96)
	effectReplace(97)
	effectReplace(98)
	effectReplace(99)
	effectReplace(100)
	effectReplace(101)
	effectReplace(102)
	effectReplace(103)
	effectReplace(104)
	effectReplace(105)
	effectReplace(106)
	effectReplace(107)
	effectReplace(108)
	effectReplace(109)
	effectReplace(110)
	effectReplace(111)
	effectReplace(112)
	effectReplace(113)
	effectReplace(114)
	effectReplace(115)
	effectReplace(116)
	effectReplace(117)
	effectReplace(118)
	effectReplace(119)
	effectReplace(120)
	effectReplace(121)
	effectReplace(122)
	effectReplace(123)
	effectReplace(124)
	effectReplace(125)
	effectReplace(126)
	effectReplace(127)
	effectReplace(128)
	effectReplace(129)
	effectReplace(130)
	effectReplace(131)
	effectReplace(132)
	effectReplace(133)
	effectReplace(134)
	effectReplace(135)
	effectReplace(136)
	effectReplace(137)
	effectReplace(138)
	effectReplace(139)
	effectReplace(140)
	effectReplace(141)
	effectReplace(142)
	effectReplace(143)
	effectReplace(144)
	effectReplace(145)
	effectReplace(146)
	effectReplace(147)
	effectReplace(148)
	effectReplace(149)
	effectReplace(150)
	effectReplace(151)
	effectReplace(152)
	effectReplace(153)
	effectReplace(154)
	effectReplace(155)
	effectReplace(156)
	effectReplace(157)
	effectReplace(158)
	effectReplace(159)
	effectReplace(160)
	effectReplace(161)
	effectReplace(162)
	effectReplace(163)
	effectReplace(164)
	effectReplace(165)
	effectReplace(166)
	effectReplace(167)
	effectReplace(168)
	effectReplace(169)
	effectReplace(170)
	effectReplace(171)
	effectReplace(172)
	effectReplace(173)
	effectReplace(174)
	effectReplace(175)
	effectReplace(176)
	effectReplace(177)
	effectReplace(178)
	effectReplace(179)
	effectReplace(180)
	effectReplace(181)
	effectReplace(182)
	effectReplace(183)
	effectReplace(184)
	effectReplace(185)
	effectReplace(186)
	effectReplace(187)
	effectReplace(188)
	effectReplace(189)
	effectReplace(190)
	effectReplace(191)
	effectReplace(192)
	effectReplace(193)
	effectReplace(194)
	effectReplace(195)
	effectReplace(196)
	effectReplace(197)
	effectReplace(198)
	effectReplace(199)
	effectReplace(200)
	effectReplace(201)
	effectReplace(202)
	effectReplace(203)
	effectReplace(204)
	effectReplace(205)
	effectReplace(206)
	effectReplace(207)
	effectReplace(208)
	effectReplace(209)
	effectReplace(210)
	effectReplace(211)
	effectReplace(212)
	effectReplace(213)
	effectReplace(214)
	effectReplace(215)
	effectReplace(216)
	effectReplace(217)
	effectReplace(218)
	effectReplace(219)
	effectReplace(220)
	effectReplace(221)
	effectReplace(222)
	effectReplace(223)
	effectReplace(224)
	effectReplace(225)
	effectReplace(226)
	effectReplace(227)
	effectReplace(228)
	effectReplace(229)
	effectReplace(230)
	effectReplace(231)
	effectReplace(232)
	effectReplace(233)
	effectReplace(234)
	effectReplace(235)
	effectReplace(236)
	effectReplace(237)
	effectReplace(238)
	effectReplace(239)
	effectReplace(240)
	effectReplace(241)
	effectReplace(242)
	effectReplace(243)
	effectReplace(244)
	effectReplace(245)
	effectReplace(246)
	effectReplace(247)
	effectReplace(248)
	effectReplace(249)
	effectReplace(250)
	effectReplace(251)
	effectReplace(252)
	effectReplace(253)
	effectReplace(254)
	effectReplace(255)
	effectReplace(256)
	effectReplace(257)
	effectReplace(258)
	effectReplace(259)
	effectReplace(260)
	effectReplace(261)
	effectReplace(262)
	effectReplace(263)
	effectReplace(264)
	effectReplace(265)
	effectReplace(266)
	effectReplace(267)
	effectReplace(268)
	effectReplace(269)
	effectReplace(270)
	effectReplace(271)
	effectReplace(272)
	effectReplace(273)
	effectReplace(274)
	effectReplace(275)
	effectReplace(276)
	effectReplace(277)
	effectReplace(278)
	effectReplace(279)
	effectReplace(280)
	effectReplace(281)
	effectReplace(282)
	effectReplace(283)
	effectReplace(284)
	effectReplace(285)
	effectReplace(286)
	effectReplace(287)
	effectReplace(288)
	effectReplace(289)
	effectReplace(290)
	effectReplace(291)
	effectReplace(292)
	effectReplace(293)
	effectReplace(294)
	effectReplace(295)
	effectReplace(296)
	effectReplace(297)
	effectReplace(298)
	effectReplace(299)
	effectReplace(300)
	effectReplace(301)
	effectReplace(302)
	effectReplace(303)
	effectReplace(304)
	effectReplace(305)
	effectReplace(306)
	effectReplace(307)
	effectReplace(308)
	effectReplace(309)
	effectReplace(310)
	effectReplace(311)
	effectReplace(312)
	effectReplace(313)
	effectReplace(314)
	effectReplace(315)
	effectReplace(316)
	effectReplace(317)
	effectReplace(318)
	effectReplace(319)
	effectReplace(320)
	effectReplace(321)
	effectReplace(322)
	effectReplace(323)
	effectReplace(324)
	effectReplace(325)
	effectReplace(326)
	effectReplace(327)
}
