#pragma once

#include <convar.h>

#include <chrono>

#include "AudioRecorder.h"
#include "VulkanCapture.h"
#include "base/Hook.h"
#include "base/Modules.h"

struct Movie {
  char filename[256];
  int fps;
};

class Recorder : public Module {
 public:
  const char* GetModuleName() { return "Recorder"; }

  void Load();
  void Unload();

  void Start(const char* outputFile, int fps, const char* encoder, int qp);
  void Stop();

 private:
  Movie movie;
  bool isRecording;
  std::chrono::high_resolution_clock::time_point timeStart;

 private:
  AudioRecorder Audio;
  VulkanCapture Video;

 private:
  CON_COMMAND_MEMBER_F(Recorder, "recorder_init", recorder_init, "Initialize recorder (required at least once before recording)", FCVAR_DONTRECORD);
  CON_COMMAND_MEMBER_F(Recorder, "recorder_start", recorder_start, "Starts recording", FCVAR_DONTRECORD);
  CON_COMMAND_MEMBER_F(Recorder, "recorder_stop", recorder_stop, "Stops recording", FCVAR_DONTRECORD);

  ConVar recorder_fps{"recorder_fps", "60", FCVAR_DONTRECORD, "Recording fps"};
  ConVar recorder_encoder{"recorder_encoder", "h264_vulkan", FCVAR_DONTRECORD, "Recording GPU encoder (h264_vulkan, hvec_vulkan, av1_vulkan)"};
  ConVar recorder_qp{"recorder_qp", "18", FCVAR_DONTRECORD, "Recording quality & quantization (0-51, 0 = lossless)"};
};

inline Recorder m_Recorder;