#pragma once

#include "base/Hook.h"

#define SND_SAMPLE_RATE 44100

class VulkanCapture;

struct SndSample {
  int left;
  int right;
};

struct WaveSample {
  short left;
  short right;
};

class AudioRecorder {
 public:
  bool Load();
  void Unload();

  void Start(int outFps, float outVolume, VulkanCapture* target);
  void Stop();

 private:
  VulkanCapture* video;
  bool isRecording;
  int fps;
  float volume;

  bool isPainting;
  bool isUnderwater;
  int numSamples;
  int skippedSamples;
  float lostMixTime;

 private:
  // int g_paintedtime;
  int* paintedTime;
  void* ptrPaintedTime;

  // portable_samplepair_t *g_paintbuffer;
  SndSample* paintBuffer;
  void* ptrPaintBuffer;

  // bool CEngine::FilterTime( float dt )
  bool FilterTime(void* p, float dt);
  Hook hookFilterTime;

  // void MIX_PaintChannels( int endtime, bool bIsUnderwater )
  void MixPaintChannels(int endtime, bool isUnderwater);
  Hook hookMixPaintChannels;

  // void IAudioDevice::TransferSamples( int end )
  void TransferSamples(void* p, int end);
  Hook hookTransferSamples;
};