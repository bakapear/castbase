#pragma once

#include "base/Hook.h"

#define SND_SAMPLE_RATE 44100

#pragma pack(push, 1)
struct WavHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t fileSize = 36;
  char wave[4] = {'W', 'A', 'V', 'E'};
  char fmt[4] = {'f', 'm', 't', ' '};
  uint32_t fmtSize = 16;
  uint16_t audioFormat = 1;
  uint16_t channels = 2;
  uint32_t sampleRate = SND_SAMPLE_RATE;
  uint32_t byteRate = SND_SAMPLE_RATE * 2 * 2;
  uint16_t blockAlign = 2 * 2;
  uint16_t bitsPerSample = 16;
  char data[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize = 0;
};
#pragma pack(pop)

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

 private:
  bool isRecording;
  int fps;

  bool isPainting;
  bool isUnderwater;
  int numSamples;
  int skippedSamples;
  float lostMixTime;

  FILE* wavFile;
  unsigned int dataSize;

 private:
  // int g_paintedtime;
  int* paintedTime;

  // portable_samplepair_t *g_paintbuffer;
  SndSample* paintBuffer;

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