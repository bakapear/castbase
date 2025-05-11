#pragma once

#include "base/Hook.h"

#define SND_SAMPLE_RATE 44100
#define SND_CHANNELS 2
#define SND_BIT_DEPTH 16

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
  void Load();
  void Unload();

  void Start(const char* outputFile, int fps);
  void Stop();

  void Frame();

 private:
  bool isRecording;
  int fps;

  bool isPainting;
  bool isUnderwater;
  int numSamples;
  int skippedSamples;
  float lostMixTime;

  FILE* pipe;

 private:
  // int g_paintedtime;
  int* paintedTime;

  // portable_samplepair_t *g_paintbuffer;
  SndSample* paintBuffer;

  // void MIX_PaintChannels( int endtime, bool bIsUnderwater )
  void MixPaintChannels(int endtime, bool isUnderwater);
  Hook hookMixPaintChannels;

  // void IAudioDevice::TransferSamples( int end )
  void TransferSamples(void* p, int end);
  Hook hookTransferSamples;
};