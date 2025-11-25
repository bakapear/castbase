#include "AudioRecorder.h"

#include "base/Sig.h"
#include "base/fills/popen.h"

void AudioRecorder::MixPaintChannels(int endtime, bool underwater) {
  this->isUnderwater = underwater;

  if (!isRecording || isPainting) {
    auto fn = hookMixPaintChannels.GetTrampoline(&AudioRecorder::MixPaintChannels);
    return fn(endtime, underwater);
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
  paintedTime = (int*)Sig::Scan(SIGPAT_PaintedTime);

  paintBuffer = *(SndSample**)Sig::Scan(SIGPAT_PaintBuffer);

  void* ptrMixPaintChannels = Sig::Scan(SIGPAT_MixPaintChannels);
  hookMixPaintChannels.Install(ptrMixPaintChannels, &AudioRecorder::MixPaintChannels, this);

  void* ptrTransferSamples = Sig::Scan(SIGPAT_TransferSamples);
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

void AudioRecorder::Start(const char* outputFile, int outFps) {
  this->fps = outFps;

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