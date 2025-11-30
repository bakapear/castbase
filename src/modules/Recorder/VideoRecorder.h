#pragma once

#include <d3d11.h>
#include <d3d9.h>
#include <dxgi.h>

#include <memory>

#include "base/Hook.h"

class VideoRecorder {
 public:
  void Load();
  void Unload();

  void Start(const char* outputFile, int fps, int width, int height);
  void Stop();

  void Frame();

 private:
  bool CreateSharedTexture(int width, int height);

 private:
  bool isRecording;
  int fps;
  int width;
  int height;

  FILE* pipe;

 private:
  ID3D11Device* d3d11Device;
  ID3D11DeviceContext* d3d11Context;
  ID3D11Texture2D* contentTexture;
  ID3D11ShaderResourceView* contentSRV;
  ID3D11Texture2D* stagingTexture;
  HANDLE sharedHandle;
  IDirect3DTexture9* sharedTexture9;
  IDirect3DSurface9* sharedSurface9;
  std::unique_ptr<uint8_t[]> frameBuffer;
  int frameBufferSize;

 private:
  // extern IDirect3DDevice9 *g_pD3DDevice;
  IDirect3DDevice9Ex* videoDevice;

  // IDirect3DDevice9::Present( CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion )
  HRESULT __stdcall D3D9Present(void* p, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
  Hook hookD3D9Present;
};