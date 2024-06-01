#pragma once

#include <cdll_int.h>
#include <vtf/vtf.h>

struct MovieInfo_t {};

abstract_class IVideoMode {
 public:
  virtual ~IVideoMode() {}
  virtual bool Init() = 0;
  virtual void Shutdown(void) = 0;
  virtual void DrawStartupGraphic() = 0;
  virtual bool CreateGameWindow(int nWidth, int nHeight, bool bWindowed) = 0;
  virtual void SetGameWindow(void* hWnd) = 0;
  virtual bool SetMode(int nWidth, int nHeight, bool bWindowed) = 0;
  virtual int GetModeCount(void) = 0;
  virtual struct vmode_s* GetMode(int num) = 0;
  virtual void UpdateWindowPosition(void) = 0;
  virtual void RestoreVideo(void) = 0;
  virtual void ReleaseVideo(void) = 0;
  virtual void DrawNullBackground(void* hdc, int w, int h) = 0;
  virtual void InvalidateWindow() = 0;
  virtual int GetModeWidth() const = 0;
  virtual int GetModeHeight() const = 0;
  virtual bool IsWindowedMode() const = 0;
  virtual int GetModeStereoWidth() const = 0;
  virtual int GetModeStereoHeight() const = 0;
  virtual int GetModeUIWidth() const = 0;
  virtual int GetModeUIHeight() const = 0;
  virtual const vrect_t& GetClientViewRect() const = 0;
  virtual void SetClientViewRect(const vrect_t& viewRect) = 0;
  virtual void MarkClientViewRectDirty() = 0;
  virtual void TakeSnapshotTGA(const char* pFileName) = 0;
  virtual void TakeSnapshotTGARect(const char* pFilename, int x, int y, int w, int h, int resampleWidth, int resampleHeight, bool bPFM = false, CubeMapFaceIndex_t faceIndex = CUBEMAP_FACE_RIGHT) = 0;
  virtual void WriteMovieFrame(const MovieInfo_t& info) = 0;
  virtual void TakeSnapshotJPEG(const char* pFileName, int quality) = 0;
  virtual bool TakeSnapshotJPEGToBuffer(CUtlBuffer & buf, int quality) = 0;
  virtual void ReadScreenPixels(int x, int y, int w, int h, void* pBuffer, ImageFormat format) = 0;
};