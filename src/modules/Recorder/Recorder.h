#pragma once

#include <convar.h>

#include <chrono>

#include "AudioRecorder.h"
#include "VideoRecorder.h"
#include "base/Hook.h"
#include "base/Modules.h"

struct Movie {
  char filename[256];
  int fps;
  int width;
  int height;
};

class Recorder : public Module {
 public:
  const char* GetModuleName() { return "Recorder"; }

  void Load();
  void Unload();

  void Start(const char* outputFile, int fps);
  void Stop();

  void Frame();

 private:
  Movie movie;
  bool isRecording;
  std::chrono::high_resolution_clock::time_point timeStart;

 private:
  AudioRecorder Audio;
  VideoRecorder Video;

 private:
  CON_COMMAND_MEMBER_F(Recorder, "recorder_start", recorder_start, "Starts Recording", FCVAR_DONTRECORD);
  CON_COMMAND_MEMBER_F(Recorder, "recorder_stop", recorder_stop, "Stops Recording", FCVAR_DONTRECORD);

 private:
  // bool CEngine::FilterTime( float dt )
  bool FilterTime(void* p, float dt);
  Hook hookFilterTime;
};

inline Recorder m_Recorder;