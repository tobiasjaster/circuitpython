// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 James Bowman for Excamera Labs
//
// SPDX-License-Identifier: MIT

#pragma once

#include "shared-module/_eve/__init__.h"

void common_hal__eve_flush(common_hal__eve_t *eve);
void common_hal__eve_add(common_hal__eve_t *eve, size_t len, void *buf);
void common_hal__eve_Vertex2f(common_hal__eve_t *eve, mp_float_t x, mp_float_t y);

void common_hal__eve_AlphaFunc(common_hal__eve_t *eve, uint32_t func, uint32_t ref);
void common_hal__eve_Begin(common_hal__eve_t *eve, uint32_t prim);
void common_hal__eve_BitmapExtFormat(common_hal__eve_t *eve, uint32_t fmt);
void common_hal__eve_BitmapHandle(common_hal__eve_t *eve, uint32_t handle);
void common_hal__eve_BitmapLayoutH(common_hal__eve_t *eve, uint32_t linestride, uint32_t height);
void common_hal__eve_BitmapLayout(common_hal__eve_t *eve, uint32_t format, uint32_t linestride, uint32_t height);
void common_hal__eve_BitmapSizeH(common_hal__eve_t *eve, uint32_t width, uint32_t height);
void common_hal__eve_BitmapSize(common_hal__eve_t *eve, uint32_t filter, uint32_t wrapx, uint32_t wrapy, uint32_t width, uint32_t height);
void common_hal__eve_BitmapSource(common_hal__eve_t *eve, uint32_t addr);
void common_hal__eve_BitmapSwizzle(common_hal__eve_t *eve, uint32_t r, uint32_t g, uint32_t b, uint32_t a);
void common_hal__eve_BitmapTransformA(common_hal__eve_t *eve, uint32_t p, uint32_t v);
void common_hal__eve_BitmapTransformB(common_hal__eve_t *eve, uint32_t p, uint32_t v);
void common_hal__eve_BitmapTransformC(common_hal__eve_t *eve, uint32_t v);
void common_hal__eve_BitmapTransformD(common_hal__eve_t *eve, uint32_t p, uint32_t v);
void common_hal__eve_BitmapTransformE(common_hal__eve_t *eve, uint32_t p, uint32_t v);
void common_hal__eve_BitmapTransformF(common_hal__eve_t *eve, uint32_t v);
void common_hal__eve_BlendFunc(common_hal__eve_t *eve, uint32_t src, uint32_t dst);
void common_hal__eve_Call(common_hal__eve_t *eve, uint32_t dest);
void common_hal__eve_Cell(common_hal__eve_t *eve, uint32_t cell);
void common_hal__eve_ClearColorA(common_hal__eve_t *eve, uint32_t alpha);
void common_hal__eve_ClearColorRGB(common_hal__eve_t *eve, uint32_t red, uint32_t green, uint32_t blue);
void common_hal__eve_Clear(common_hal__eve_t *eve, uint32_t c, uint32_t s, uint32_t t);
void common_hal__eve_ClearStencil(common_hal__eve_t *eve, uint32_t s);
void common_hal__eve_ClearTag(common_hal__eve_t *eve, uint32_t s);
void common_hal__eve_ColorA(common_hal__eve_t *eve, uint32_t alpha);
void common_hal__eve_ColorMask(common_hal__eve_t *eve, uint32_t r, uint32_t g, uint32_t b, uint32_t a);
void common_hal__eve_ColorRGB(common_hal__eve_t *eve, uint32_t red, uint32_t green, uint32_t blue);
void common_hal__eve_Display(common_hal__eve_t *eve);
void common_hal__eve_End(common_hal__eve_t *eve);
void common_hal__eve_Jump(common_hal__eve_t *eve, uint32_t dest);
void common_hal__eve_LineWidth(common_hal__eve_t *eve, mp_float_t width);
void common_hal__eve_Macro(common_hal__eve_t *eve, uint32_t m);
void common_hal__eve_Nop(common_hal__eve_t *eve);
void common_hal__eve_PaletteSource(common_hal__eve_t *eve, uint32_t addr);
void common_hal__eve_PointSize(common_hal__eve_t *eve, mp_float_t size);
void common_hal__eve_RestoreContext(common_hal__eve_t *eve);
void common_hal__eve_Return(common_hal__eve_t *eve);
void common_hal__eve_SaveContext(common_hal__eve_t *eve);
void common_hal__eve_ScissorSize(common_hal__eve_t *eve, uint32_t width, uint32_t height);
void common_hal__eve_ScissorXY(common_hal__eve_t *eve, uint32_t x, uint32_t y);
void common_hal__eve_StencilFunc(common_hal__eve_t *eve, uint32_t func, uint32_t ref, uint32_t mask);
void common_hal__eve_StencilMask(common_hal__eve_t *eve, uint32_t mask);
void common_hal__eve_StencilOp(common_hal__eve_t *eve, uint32_t sfail, uint32_t spass);
void common_hal__eve_TagMask(common_hal__eve_t *eve, uint32_t mask);
void common_hal__eve_Tag(common_hal__eve_t *eve, uint32_t s);
void common_hal__eve_VertexTranslateX(common_hal__eve_t *eve, mp_float_t x);
void common_hal__eve_VertexTranslateY(common_hal__eve_t *eve, mp_float_t y);
void common_hal__eve_VertexFormat(common_hal__eve_t *eve, uint32_t frac);
void common_hal__eve_Vertex2ii(common_hal__eve_t *eve, uint32_t x, uint32_t y, uint32_t handle, uint32_t cell);
