#pragma once

#include <cdll_int.h>
#include <shaderapi/ishaderapi.h>
#include <sourcehook_impl.h>

#include "fills/ivideomode.h"

// required sourcehook boilerplate
SourceHook::Impl::CSourceHookImpl g_SourceHook;
SourceHook::ISourceHook *g_SHPtr = &g_SourceHook;
int g_PLID = 0;

SH_DECL_HOOK1_void(IVideoMode, WriteMovieFrame, SH_NOATTRIB, 0, const MovieInfo_t &);
SH_DECL_HOOK1_void(IBaseClientDLL, FrameStageNotify, SH_NOATTRIB, 0, ClientFrameStage_t);
SH_DECL_HOOK6_void(IShaderAPI, ReadPixels, SH_NOATTRIB, 0, int, int, int, int, unsigned char *, ImageFormat);
SH_DECL_HOOK5_void(IShaderAPI, ReadPixels, SH_NOATTRIB, 1, Rect_t *, Rect_t *, unsigned char *, ImageFormat, int);