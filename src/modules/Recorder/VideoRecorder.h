#pragma once

#include <d3d9.h>

#include "base/Hook.h"

class VideoRecorder {
 public:
  void Load();
  void Unload();

  void Start(const char* outputFile, int fps, int width, int height);
  void Stop();

  void Frame();

 private:
  bool isRecording;
  int fps;
  int width;
  int height;

  FILE* pipe;

 private:
  // extern IDirect3DDevice9 *g_pD3DDevice;
  IDirect3DDevice9Ex* videoDevice;

  // IDirect3DDevice9::Present( CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion )
  HRESULT __stdcall D3D9Present(void* p, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
  Hook hookD3D9Present;
};