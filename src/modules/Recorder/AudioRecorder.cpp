#include "AudioRecorder.h"

#include "base/Sig.h"

bool AudioRecorder::FilterTime(void* p, float dt) {
  if (!isRecording) {
    auto fn = hookFilterTime.GetTrampoline(&AudioRecorder::FilterTime);
    return fn(p, dt);
  }

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

  return true;
}

void AudioRecorder::MixPaintChannels(int endtime, bool underwater) {
  this->isUnderwater = underwater;

  if (!isRecording || isPainting) {
    auto fn = hookMixPaintChannels.GetTrampoline(&AudioRecorder::MixPaintChannels);
    return fn(endtime, underwater);
  }
}

void AudioRecorder::TransferSamples(void* p, int end) {
  if (!isRecording || !wavFile) {
    auto fn = hookTransferSamples.GetTrampoline(&AudioRecorder::TransferSamples);
    return fn(p, end);
  }

  WaveSample* buf = (WaveSample*)_alloca(sizeof(WaveSample) * numSamples);

  for (int i = 0; i < numSamples; i++) {
    SndSample* sample = &paintBuffer[i];
    buf[i] = WaveSample{(short)sample->left, (short)sample->right};
  }

  size_t written = fwrite(buf, sizeof(WaveSample), numSamples, wavFile);
  dataSize += static_cast<uint32_t>(written * sizeof(WaveSample));
}

void AudioRecorder::Load() {
  paintedTime = (int*)Sig::Scan(SIGPAT_PaintedTime);

  paintBuffer = *(SndSample**)Sig::Scan(SIGPAT_PaintBuffer);

  void* ptrFilterTime = Sig::Scan(SIGPAT_FilterTime);
  hookFilterTime.Install(ptrFilterTime, &AudioRecorder::FilterTime, this);

  void* ptrMixPaintChannels = Sig::Scan(SIGPAT_MixPaintChannels);
  hookMixPaintChannels.Install(ptrMixPaintChannels, &AudioRecorder::MixPaintChannels, this);

  void* ptrTransferSamples = Sig::Scan(SIGPAT_TransferSamples);
  hookTransferSamples.Install(ptrTransferSamples, &AudioRecorder::TransferSamples, this);
}

void AudioRecorder::Unload() {
  Stop();

  paintedTime = nullptr;
  paintBuffer = nullptr;
  hookMixPaintChannels.Remove();
  hookTransferSamples.Remove();
}

void AudioRecorder::Start(const char* outputFile, int outFps) {
  this->fps = outFps;

  lostMixTime = 0.0;
  skippedSamples = 0;
  numSamples = 0;
  dataSize = 0;

  char filename[512];
  snprintf(filename, sizeof(filename), "%s.wav", outputFile);

  wavFile = fopen(filename, "wb");
  if (!wavFile) return Error("Could not open WAV file!");

  WavHeader header;
  fwrite(&header, sizeof(header), 1, wavFile);

  isRecording = true;
}

void AudioRecorder::Stop() {
  isRecording = false;

  if (wavFile) {
    WavHeader header;
    header.fileSize = 36 + dataSize;
    header.dataSize = dataSize;

    fseek(wavFile, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, wavFile);

    fclose(wavFile);
    wavFile = nullptr;
  }
}
