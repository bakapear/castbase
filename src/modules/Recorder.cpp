#include "Recorder.h"

#include <shaderapi/ishaderapi.h>
#include <tier1/KeyValues.h>
#include <toolframework/ienginetool.h>

#include "base/Interfaces.h"
#include "base/Sig.h"

void Recorder::Hook_WriteMovieFrame(const MovieInfo_t& info) {
  Msg(">> Called WriteMovieFrame\n");

  if (this->isRecording) {
    RETURN_META(MRES_SUPERCEDE);
  }
}

void Recorder::Hook_FrameStageNotify(ClientFrameStage_t curStage) {
  if (curStage != FRAME_RENDER_END) return;

  if (this->isRecording) {
    RecConfig* config = &(this->config);
    uint8_t* data = new uint8_t[config->width * config->height * 3];
    Interfaces::ShaderAPI->ReadPixels(0, 0, config->width, config->height, data, IMAGE_FORMAT_RGB888);
    delete data;
    RETURN_META(MRES_SUPERCEDE);
    // Msg(">> isRecording @ FrameStageNotify\n");
  }
}

void Recorder::Hook_ReadPixels_0(int x, int y, int width, int height, unsigned char* data, ImageFormat dstFormat) {
  Msg(">> Called ReadPixels 0\n");
  // todo
}

void Recorder::Hook_ReadPixels_1(Rect_t* pSrcRect, Rect_t* pDstRect, unsigned char* data, ImageFormat dstFormat, int nDstStride) {
  Msg(">> Called ReadPixels 1\n");
  // todo
}

void Recorder::Load() {
  SH_ADD_HOOK(IVideoMode, WriteMovieFrame, Interfaces::VideoMode, SH_MEMBER(this, &Recorder::Hook_WriteMovieFrame), false);
  SH_ADD_HOOK(IBaseClientDLL, FrameStageNotify, Interfaces::ClientDLL, SH_MEMBER(this, &Recorder::Hook_FrameStageNotify), false);
  SH_ADD_HOOK(IShaderAPI, ReadPixels, Interfaces::ShaderAPI, SH_MEMBER(this, &Recorder::Hook_ReadPixels_0), false);
  SH_ADD_HOOK(IShaderAPI, ReadPixels, Interfaces::ShaderAPI, SH_MEMBER(this, &Recorder::Hook_ReadPixels_1), false);
}

void Recorder::Unload() {
  SH_REMOVE_HOOK(IVideoMode, WriteMovieFrame, Interfaces::VideoMode, SH_MEMBER(this, &Recorder::Hook_WriteMovieFrame), false);
  SH_REMOVE_HOOK(IBaseClientDLL, FrameStageNotify, Interfaces::ClientDLL, SH_MEMBER(this, &Recorder::Hook_FrameStageNotify), false);
  SH_REMOVE_HOOK(IShaderAPI, ReadPixels, Interfaces::ShaderAPI, SH_MEMBER(this, &Recorder::Hook_ReadPixels_0), false);
  SH_REMOVE_HOOK(IShaderAPI, ReadPixels, Interfaces::ShaderAPI, SH_MEMBER(this, &Recorder::Hook_ReadPixels_1), false);
}

void Recorder::recorder_start(const CCommand& args) {
  if (args.ArgC() != 2) return ConMsg("Usage:  recorder_start <filename>\n");
  if (!Interfaces::EngineClient->IsPlayingDemo()) return ConMsg("Not in a demo!\n");

  RecConfig* config = &(this->config);
  config->filename = args[1];
  Interfaces::EngineClient->GetScreenSize(config->width, config->height);

  ConVarRef fps_max("fps_max");
  fps_max.SetValue(0);

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(60);

  this->isRecording = true;
  this->timeStart = std::chrono::high_resolution_clock::now();
}

void Recorder::recorder_stop(const CCommand& args) {
  if (!this->isRecording) return ConMsg("Not recording!\n");

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(0);

  this->isRecording = false;

  const auto endTime = std::chrono::high_resolution_clock::now();
  const auto delta = std::chrono::duration<float>(endTime - (this->timeStart));

  RecConfig* config = &(this->config);

  ConColorMsg(Color(255, 255, 0, 255), "Finished recording <%s> in %1.2fs (%ix%i, 60 fps)\n", config->filename, delta.count(), config->width, config->height);
}
