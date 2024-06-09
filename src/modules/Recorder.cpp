#include "Recorder.h"

#include <cdll_int.h>
#include <tier1/KeyValues.h>
#include <toolframework/ienginetool.h>

#include "base/Interfaces.h"
#include "base/Sig.h"

bool __fastcall Recorder::FilterTime(void* p, void* edx, float dt) {
  if (!this->isRecording) {
    auto fn = hookFilterTime.GetTrampoline(&Recorder::FilterTime);
    return fn(p, edx, dt);
  }

  ConMsg("Recording!\n");
  return true;
}

void __cdecl Recorder::MixPaintChannels(int endtime, bool isUnderwater) {
  if (!this->isRecording) {
    auto fn = hookMixPaintChannels.GetTrampoline(&Recorder::MixPaintChannels);
    return fn(endtime, isUnderwater);
  }
}

void __fastcall Recorder::TransferSamples(void* p, void* edx, int end) {
  if (!this->isRecording) {
    auto fn = hookTransferSamples.GetTrampoline(&Recorder::TransferSamples);
    return fn(p, edx, end);
  }

  // SndSample* sample = paintBuffer;
}

void Recorder::Load() {
  paintBuffer = (SndSample*)Sig::Scan("engine.dll", "48 8B 3D ?? ?? ?? ?? 48 89 B5 ?? ?? ?? ?? 48 89 9D ?? ?? ?? ?? 0F 29 B4 24 ?? ?? ?? ??", 3, 4);

  void* ptrFilterTime = Sig::Scan("engine.dll", "40 53 48 83 EC 40 80 3D ?? ?? ?? ?? ?? 48 8B D9 0F 29 74 24 ?? 0F 28 F1 74 2B 80 3D ?? ?? ?? ?? ?? 75 22", 0, 0);
  hookFilterTime.Install(ptrFilterTime, &Recorder::FilterTime, this);

  void* ptrMixPaintChannels = Sig::Scan("engine.dll", "48 8B C4 88 50 10 89 48 08 53 48 81 EC ?? ?? ?? ?? 48 89 78 E0 33 FF 4C 89 68 D0 4C 89 78 C0", 0, 0);
  hookMixPaintChannels.Install(ptrMixPaintChannels, &Recorder::MixPaintChannels, this);

  void* ptrTransferSamples = Sig::Scan("engine.dll", "48 89 5C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ??", 0, 0);
  hookTransferSamples.Install(ptrTransferSamples, &Recorder::TransferSamples, this);
}

void Recorder::Unload() {
  hookTransferSamples.Remove();
  hookMixPaintChannels.Remove();
  hookFilterTime.Remove();
  paintBuffer = nullptr;
}

void Recorder::recorder_start(const CCommand& args) {
  if (args.ArgC() != 2) return ConMsg("Usage:  recorder_start <filename>\n");
  if (!Interfaces::EngineClient->IsPlayingDemo()) return ConMsg("Not in a demo!\n");

  movie.filename = args[1];
  Interfaces::EngineClient->GetScreenSize(movie.width, movie.height);

  ConVarRef fps_max("fps_max");
  fps_max.SetValue(0);

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(60);

  this->isRecording = true;
  this->timeStart = std::chrono::high_resolution_clock::now();
}

void Recorder::recorder_stop(const CCommand& args) {
  if (!this->isRecording) return ConMsg("Not recording!\n");

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(0);

  this->isRecording = false;

  const auto endTime = std::chrono::high_resolution_clock::now();
  const auto delta = std::chrono::duration<float>(endTime - (this->timeStart));

  ConColorMsg(Color(255, 255, 0, 255), "Finished recording <%s> in %1.2fs (%ix%i, 60 fps)\n", movie.filename, delta.count(), movie.width, movie.height);
}
