#include "VideoRecorder.h"

#include <Windows.h>
#include <dxgi1_2.h>

#include <chrono>

#include "base/Sig.h"
#include "base/fills/popen.h"

HRESULT __stdcall VideoRecorder::D3D9Present(void* p, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
  auto fn = hookD3D9Present.GetTrampoline(&VideoRecorder::D3D9Present);
  return fn(p, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void VideoRecorder::Load() {
  videoDevice = *(IDirect3DDevice9Ex**)Sig::Scan(SIGPAT_VideoDevice);
  videoDevice->AddRef();

  void* ptrD3D9Present = Sig::Virtual(videoDevice, SIGVTI_Present);
  hookD3D9Present.Install(ptrD3D9Present, &VideoRecorder::D3D9Present, this);

  D3D_FEATURE_LEVEL featureLevel;
  D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &d3d11Device, &featureLevel, &d3d11Context);
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

  if (contentTexture) {
    contentTexture->Release();
    contentTexture = nullptr;
  }

  if (contentSRV) {
    contentSRV->Release();
    contentSRV = nullptr;
  }

  if (stagingTexture) {
    stagingTexture->Release();
    stagingTexture = nullptr;
  }

  if (d3d11Context) {
    d3d11Context->Release();
    d3d11Context = nullptr;
  }

  if (d3d11Device) {
    d3d11Device->Release();
    d3d11Device = nullptr;
  }

  if (sharedHandle) {
    CloseHandle(sharedHandle);
    sharedHandle = nullptr;
  }

  if (sharedSurface9) {
    sharedSurface9->Release();
    sharedSurface9 = nullptr;
  }

  if (sharedTexture9) {
    sharedTexture9->Release();
    sharedTexture9 = nullptr;
  }
}

bool VideoRecorder::CreateSharedTexture(int outWidth, int outHeight) {
  if (!d3d11Device) return false;

  D3D11_TEXTURE2D_DESC texDesc = {};
  texDesc.Width = outWidth;
  texDesc.Height = outHeight;
  texDesc.MipLevels = 1;
  texDesc.ArraySize = 1;
  texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  texDesc.SampleDesc.Count = 1;
  texDesc.Usage = D3D11_USAGE_DEFAULT;
  texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

  d3d11Device->CreateTexture2D(&texDesc, nullptr, &contentTexture);

  IDXGIResource* dxgiResource = nullptr;
  contentTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&dxgiResource);

  dxgiResource->GetSharedHandle(&sharedHandle);
  dxgiResource->Release();

  D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
  stagingDesc.Usage = D3D11_USAGE_STAGING;
  stagingDesc.BindFlags = 0;
  stagingDesc.MiscFlags = 0;
  stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

  d3d11Device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);

  videoDevice->CreateTexture(outWidth, outHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &sharedTexture9, &sharedHandle);

  sharedTexture9->GetSurfaceLevel(0, &sharedSurface9);

  d3d11Device->OpenSharedResource(sharedHandle, __uuidof(ID3D11Texture2D), (void**)&contentTexture);

  d3d11Device->CreateShaderResourceView(contentTexture, nullptr, &contentSRV);

  frameBufferSize = outWidth * outHeight * 4;  // BGRA = 4 bytes per pixel
  frameBuffer = std::make_unique<uint8_t[]>(frameBufferSize);

  return true;
}

void VideoRecorder::Start(const char* outputFile, int outFps, int outWidth, int outHeight) {
  this->fps = outFps;
  this->width = outWidth;
  this->height = outHeight;

  CreateSharedTexture(outWidth, outHeight);

  char cmd[512];
  snprintf(cmd, sizeof(cmd), "ffmpeg -f rawvideo -pixel_format bgra -video_size %dx%d -framerate %d -i - -c:v libx264 -preset ultrafast -crf 15 -pix_fmt yuv420p %s.mp4", outWidth, outHeight, outFps, outputFile);

  pipe = popen(cmd, "wb");
  if (!pipe) return Error("Could not open ffmpeg video pipe!");

  isRecording = true;
}

void VideoRecorder::Stop() {
  isRecording = false;

  if (pipe) {
    fclose(pipe);
    pipe = nullptr;
  }
}

void VideoRecorder::Frame() {
  if (!isRecording) return;

  IDirect3DSurface9* renderTarget = nullptr;
  videoDevice->GetRenderTarget(0, &renderTarget);

  D3DSURFACE_DESC desc;
  renderTarget->GetDesc(&desc);

  videoDevice->StretchRect(renderTarget, NULL, sharedSurface9, NULL, D3DTEXF_NONE);
  d3d11Context->CopyResource(stagingTexture, contentTexture);

  D3D11_MAPPED_SUBRESOURCE mappedResource;
  d3d11Context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);

  // copy pixel data to our frame buffer
  // TODO: use shaders instead
  uint8_t* srcPixels = (uint8_t*)mappedResource.pData;
  uint8_t* dstPixels = frameBuffer.get();
  int srcPitch = mappedResource.RowPitch;
  int dstPitch = width * 4;  // BGRA = 4 bytes per pixel
  for (int y = 0; y < height; y++) {
    memcpy(dstPixels + (y * dstPitch), srcPixels + (y * srcPitch), dstPitch);
  }

  d3d11Context->Unmap(stagingTexture, 0);
  renderTarget->Release();

  fwrite(frameBuffer.get(), 1, frameBufferSize, pipe);
}