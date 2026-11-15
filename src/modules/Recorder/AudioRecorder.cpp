#include "AudioRecorder.h"

#include "VulkanCapture.h"
#include "base/Sig.h"

bool AudioRecorder::FilterTime(void* p, float dt) {
  auto fn = hookFilterTime.GetTrampoline(&AudioRecorder::FilterTime);
  bool ret = fn(p, dt);

  if (isRecording) {
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

  return ret;
}

void AudioRecorder::MixPaintChannels(int endtime, bool underwater) {
  this->isUnderwater = underwater;

  if (!isRecording || isPainting) {
    auto fn = hookMixPaintChannels.GetTrampoline(&AudioRecorder::MixPaintChannels);
    return fn(endtime, underwater);
  }
}

void AudioRecorder::TransferSamples(void* p, int end) {
  if (!isRecording || !video) {
    auto fn = hookTransferSamples.GetTrampoline(&AudioRecorder::TransferSamples);
    return fn(p, end);
  }

  if (numSamples <= 0) return;

  WaveSample* buf = (WaveSample*)_alloca(sizeof(WaveSample) * numSamples);

  for (int i = 0; i < numSamples; i++) {
    SndSample* sample = &paintBuffer[i];
    buf[i] = WaveSample{(short)sample->left, (short)sample->right};
  }

  video->WriteAudio(reinterpret_cast<const int16_t*>(buf), numSamples);
}

bool AudioRecorder::Load() {
  ptrPaintedTime = Sig::Scan(SIGPAT_PaintedTime);
  if (!ptrPaintedTime) return false;

  ptrPaintBuffer = Sig::Scan(SIGPAT_PaintBuffer);
  if (!ptrPaintBuffer) return false;

  void* ptrFilterTime = Sig::Scan(SIGPAT_FilterTime);
  if (!hookFilterTime.Install(ptrFilterTime, &AudioRecorder::FilterTime, this)) return false;

  void* ptrMixPaintChannels = Sig::Scan(SIGPAT_MixPaintChannels);
  if (!hookMixPaintChannels.Install(ptrMixPaintChannels, &AudioRecorder::MixPaintChannels, this)) return false;

  void* ptrTransferSamples = Sig::Scan(SIGPAT_TransferSamples);
  if (!hookTransferSamples.Install(ptrTransferSamples, &AudioRecorder::TransferSamples, this)) return false;

  return true;
}

void AudioRecorder::Unload() {
  Stop();

  paintedTime = nullptr;
  ptrPaintedTime = nullptr;
  paintBuffer = nullptr;
  ptrPaintBuffer = nullptr;
  hookMixPaintChannels.Remove();
  hookTransferSamples.Remove();
  hookFilterTime.Remove();
}

void AudioRecorder::Start(int outFps, VulkanCapture* target) {
  if (!paintedTime) paintedTime = (int*)ptrPaintedTime;
  if (!paintBuffer) paintBuffer = *(SndSample**)ptrPaintBuffer;

  this->fps = outFps;
  this->video = target;

  lostMixTime = 0.0;
  skippedSamples = 0;
  numSamples = 0;

  isRecording = true;
}

void AudioRecorder::Stop() {
  isRecording = false;
  video = nullptr;
}
