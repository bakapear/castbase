#include "VideoRecorder.h"

#include "base/Sig.h"

HRESULT __stdcall VideoRecorder::D3D9Present(void* p, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
  auto fn = hookD3D9Present.GetTrampoline(&VideoRecorder::D3D9Present);
  return fn(p, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void VideoRecorder::Load() {
  videoDevice = *(IDirect3DDevice9Ex**)Sig::Scan("shaderapidx9.dll", "48 8B 0D ?? ?? ?? ?? 8D 46 01 48 63 D0 4C 8D 85 ?? ?? ?? ?? 4C 8B 09 4D 8D 04 D0", 3, 4);
  videoDevice->AddRef();

  void* ptrD3D9Present = Sig::Virtual(videoDevice, 17);
  hookD3D9Present.Install(ptrD3D9Present, &VideoRecorder::D3D9Present, this);
}

void VideoRecorder::Unload() {
  hookD3D9Present.Remove();

  if (videoDevice) {
    videoDevice->Release();
    videoDevice = nullptr;
  }

  if (pipe) {
    fclose(pipe);
    pipe = nullptr;
  }
}

void VideoRecorder::Start(const char* outputFile, int outFps, int outWidth, int outHeight) {
  this->fps = outFps;
  this->width = outWidth;
  this->height = outHeight;

  isRecording = true;
}

void VideoRecorder::Stop() {
  isRecording = false;

  if (pipe) {
    fclose(pipe);
    pipe = nullptr;
  }
}

void VideoRecorder::Frame() {}