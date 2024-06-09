#pragma once

#include <convar.h>

#include <chrono>
#include <string>

#include "base/Hook.h"
#include "base/Modules.h"

struct Movie {
  std::string filename;
  int width;
  int height;
};

struct SndSample {
  int left;
  int right;
};

class Recorder : public Module {
 public:
  const char* GetModuleName() { return "Recorder"; }

  void Load();
  void Unload();

 private:
  Movie movie;
  bool isRecording;
  std::chrono::high_resolution_clock::time_point timeStart;

  // commands
 private:
  CON_COMMAND_MEMBER_F(Recorder, "recorder_start", recorder_start, "Starts Recording", 0);
  CON_COMMAND_MEMBER_F(Recorder, "recorder_stop", recorder_stop, "Stops Recording", 0);

  // game pointers and hooks
 private:
  // portable_samplepair_t *g_paintbuffer;
  SndSample* paintBuffer;

  // bool CEngine::FilterTime( float dt )
  bool __fastcall FilterTime(void* p, void* edx, float dt);
  Hook hookFilterTime;

  // void MIX_PaintChannels( int endtime, bool bIsUnderwater )
  void __cdecl MixPaintChannels(int endtime, bool isUnderwater);
  Hook hookMixPaintChannels;

  // void IAudioDevice::TransferSamples( int end )
  void __fastcall TransferSamples(void* p, void* edx, int end);
  Hook hookTransferSamples;
};

inline Recorder m_Recorder;