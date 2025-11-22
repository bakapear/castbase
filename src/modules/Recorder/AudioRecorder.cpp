#include "AudioRecorder.h"

#include "base/Sig.h"
#include "base/fills/popen.h"

void AudioRecorder::MixPaintChannels(int endtime, bool isUnderwater) {
  this->isUnderwater = isUnderwater;

  if (!isRecording || isPainting) {
    auto fn = hookMixPaintChannels.GetTrampoline(&AudioRecorder::MixPaintChannels);
    return fn(endtime, isUnderwater);
  }
}

void AudioRecorder::TransferSamples(void* p, int end) {
  if (!isRecording) {
    auto fn = hookTransferSamples.GetTrampoline(&AudioRecorder::TransferSamples);
    return fn(p, end);
  }

  WaveSample* buf = (WaveSample*)_alloca(sizeof(WaveSample) * numSamples);

  for (int i = 0; i < numSamples; i++) {
    SndSample* sample = &paintBuffer[i];
    buf[i] = WaveSample{(short)sample->left, (short)sample->right};
  }

  fwrite(buf, sizeof(WaveSample), numSamples, pipe);
}

void AudioRecorder::Load() {
  paintedTime = (int*)Sig::Scan("engine.dll", "8B 3D ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ??", 2, 4);

  paintBuffer = *(SndSample**)Sig::Scan("engine.dll", "48 8B 3D ?? ?? ?? ?? 48 89 B5 ?? ?? ?? ?? 48 89 9D ?? ?? ?? ?? 0F 29 B4 24 ?? ?? ?? ??", 3, 4);

  void* ptrMixPaintChannels = Sig::Scan("engine.dll", "48 8B C4 88 50 10 89 48 08 53 48 81 EC ?? ?? ?? ?? 48 89 78 E0 33 FF 4C 89 68 D0 4C 89 78 C0", 0, 0);
  hookMixPaintChannels.Install(ptrMixPaintChannels, &AudioRecorder::MixPaintChannels, this);

  void* ptrTransferSamples = Sig::Scan("engine.dll", "48 89 5C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ??", 0, 0);
  hookTransferSamples.Install(ptrTransferSamples, &AudioRecorder::TransferSamples, this);
}

void AudioRecorder::Unload() {
  paintedTime = nullptr;
  paintBuffer = nullptr;
  hookMixPaintChannels.Remove();
  hookTransferSamples.Remove();

  if (pipe) {
    fclose(pipe);
    pipe = nullptr;
  }
}

void AudioRecorder::Start(const char* outputFile, int fps) {
  this->fps = fps;

  lostMixTime = 0.0;
  skippedSamples = 0;
  numSamples = 0;

  char cmd[512];
  snprintf(cmd, sizeof(cmd), "ffmpeg -f s16le -c:a pcm_s16le -ar 44100 -ac 2 -i - %s.wav", outputFile);

  pipe = popen(cmd, "w");
  if (!pipe) return Error("Could not open ffmpeg audio pipe!");

  isRecording = true;
}

void AudioRecorder::Stop() {
  isRecording = false;

  if (pipe) {
    fclose(pipe);
    pipe = nullptr;
  }
}

void AudioRecorder::Frame() {
  float timeAheadToMix = 1.0f / (float)fps;
  float numFracSamplesToMix = (timeAheadToMix * SND_SAMPLE_RATE) + lostMixTime;

  int numSamplesToMix = (int)numFracSamplesToMix;
  lostMixTime = numFracSamplesToMix - (float)numSamplesToMix;

  int pt = *paintedTime;

  int rawEndTime = pt + numSamplesToMix + skippedSamples;
  int alignedEndTime = rawEndTime & ~3;
  numSamples = alignedEndTime - pt;
  skippedSamples = rawEndTime - alignedEndTime;

  if (numSamples > 0) {
    isPainting = true;
    MixPaintChannels(alignedEndTime, isUnderwater);
    isPainting = false;
  }
}