#include "Recorder.h"

#include <cdll_int.h>
#include <tier1/KeyValues.h>
#include <toolframework/ienginetool.h>

#include "base/Interfaces.h"
#include "base/Sig.h"
#include "base/fills/popen.h"

void Recorder::AudioFrame() {
  float timeAheadToMix = 1.0f / (float)movie.fps;
  float numFracSamplesToMix = (timeAheadToMix * SND_SAMPLE_RATE) + sndLostMixTime;

  int numSamplesToMix = (int)numFracSamplesToMix;
  sndLostMixTime = numFracSamplesToMix - (float)numSamplesToMix;

  int pt = *paintedTime;

  int rawEndTime = pt + numSamplesToMix + sndSkippedSamples;
  int alignedEndTime = rawEndTime & ~3;
  int numSamples = alignedEndTime - pt;

  sndSkippedSamples = rawEndTime - alignedEndTime;
  sndNumSamples = numSamples;

  if (numSamples > 0) {
    sndIsPainting = true;
    Recorder::MixPaintChannels(alignedEndTime, sndIsUnderwater);
    sndIsPainting = false;
  }
}

void Recorder::RecordFrame() {
  Recorder::AudioFrame();
  // Recorder::VideoFrame();
}

bool Recorder::FilterTime(void* p, float dt) {
  if (!this->isRecording) {
    auto fn = hookFilterTime.GetTrampoline(&Recorder::FilterTime);
    return fn(p, dt);
  }

  Recorder::RecordFrame();
  return true;
}

void Recorder::MixPaintChannels(int endtime, bool isUnderwater) {
  sndIsUnderwater = isUnderwater;

  if (!this->isRecording || sndIsPainting) {
    auto fn = hookMixPaintChannels.GetTrampoline(&Recorder::MixPaintChannels);
    return fn(endtime, isUnderwater);
  }
}

void Recorder::TransferSamples(void* p, int end) {
  if (!this->isRecording) {
    auto fn = hookTransferSamples.GetTrampoline(&Recorder::TransferSamples);
    return fn(p, end);
  }

  for (int i = 0; i < sndNumSamples; i++) {
    SndSample* sample = &paintBuffer[i];
    WaveSample waveSample = {(short)sample->left, (short)sample->right};
    fwrite(&waveSample, sizeof(WaveSample), 1, movie.audio);
  }

  // TODO: maybe this is faster. test!
  /*
  WaveSample* buf = (WaveSample*)_alloca(sizeof(WaveSample) * sndNumSamples);

  for (int i = 0; i < sndNumSamples; i++) {
    SndSample* sample = &paintBuffer[i];
    buf[i] = WaveSample{(short)sample->left, (short)sample->right};
  }

  fwrite(buf, sizeof(WaveSample), sndNumSamples, pipe);
  */
}

HRESULT __stdcall Recorder::D3D9Present(void* p, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
  auto fn = hookD3D9Present.GetTrampoline(&Recorder::D3D9Present);
  return fn(p, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void Recorder::Load() {
  void* ptrFilterTime = Sig::Scan("engine.dll", "40 53 48 83 EC 40 80 3D ?? ?? ?? ?? ?? 48 8B D9 0F 29 74 24 ?? 0F 28 F1 74 2B 80 3D ?? ?? ?? ?? ?? 75 22", 0, 0);
  hookFilterTime.Install(ptrFilterTime, &Recorder::FilterTime, this);

  videoDevice = *(IDirect3DDevice9Ex**)Sig::Scan("shaderapidx9.dll", "48 8B 0D ?? ?? ?? ?? 8D 46 01 48 63 D0 4C 8D 85 ?? ?? ?? ?? 4C 8B 09 4D 8D 04 D0", 3, 4);
  videoDevice->AddRef();

  void* ptrD3D9Present = Sig::Virtual(videoDevice, 17);
  hookD3D9Present.Install(ptrD3D9Present, &Recorder::D3D9Present, this);

  paintBuffer = *(SndSample**)Sig::Scan("engine.dll", "48 8B 3D ?? ?? ?? ?? 48 89 B5 ?? ?? ?? ?? 48 89 9D ?? ?? ?? ?? 0F 29 B4 24 ?? ?? ?? ??", 3, 4);

  paintedTime = (int*)Sig::Scan("engine.dll", "8B 3D ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ??", 2, 4);

  void* ptrMixPaintChannels = Sig::Scan("engine.dll", "48 8B C4 88 50 10 89 48 08 53 48 81 EC ?? ?? ?? ?? 48 89 78 E0 33 FF 4C 89 68 D0 4C 89 78 C0", 0, 0);
  hookMixPaintChannels.Install(ptrMixPaintChannels, &Recorder::MixPaintChannels, this);

  void* ptrTransferSamples = Sig::Scan("engine.dll", "48 89 5C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ??", 0, 0);
  hookTransferSamples.Install(ptrTransferSamples, &Recorder::TransferSamples, this);
}

void Recorder::Unload() {
  hookFilterTime.Remove();

  videoDevice->Release();
  videoDevice = nullptr;

  paintBuffer = nullptr;

  paintedTime = nullptr;

  hookMixPaintChannels.Remove();

  hookTransferSamples.Remove();
}

void Recorder::recorder_start(const CCommand& args) {
  if (args.ArgC() != 2) return ConMsg("Usage:  recorder_start <filename>\n");
  if (!Interfaces::EngineClient->IsPlayingDemo()) return ConMsg("Not in a demo!\n");

  movie.fps = 60;
  movie.filename = args[1];
  Interfaces::EngineClient->GetScreenSize(movie.width, movie.height);

  ConVarRef fps_max("fps_max");
  fps_max.SetValue(0);

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(movie.fps);

  this->sndLostMixTime = 0.0;
  this->sndSkippedSamples = 0;
  this->sndNumSamples = 0;

  std::string cmd = "ffmpeg -f s16le -c:a pcm_s16le -ar 44100 -ac 2 -i - " + movie.filename + ".wav";
  movie.audio = popen(cmd.c_str(), "w");
  if (!movie.audio) return Error("Could not open ffmpeg audio pipe!");

  this->isRecording = true;
  this->timeStart = std::chrono::high_resolution_clock::now();
}

void Recorder::recorder_stop(const CCommand& args) {
  if (!this->isRecording) return ConMsg("Not recording!\n");

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(0);

  this->isRecording = false;

  fclose(movie.audio);

  const auto endTime = std::chrono::high_resolution_clock::now();
  const auto delta = std::chrono::duration<float>(endTime - (this->timeStart));

  ConColorMsg(Color(255, 255, 0, 255), "Finished recording <%s> in %1.2fs (%ix%i, 60 fps)\n", movie.filename.c_str(), delta.count(), movie.width, movie.height);
}
