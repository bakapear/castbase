#include "Recorder.h"

#include <cdll_int.h>
#include <tier1/KeyValues.h>
#include <toolframework/ienginetool.h>

#include "base/Interfaces.h"
#include "base/Sig.h"

bool __fastcall Recorder::FilterTime(void* p, void* edx, float dt) {
  if (this->isRecording) {
    ConMsg("Recording!\n");
    return true;
  }

  // call original
  auto fn = FilterTimeHook.GetTrampoline(&Recorder::FilterTime);
  return fn(p, edx, dt);
}

void Recorder::Load() {
  void* ptrFilterTime = Sig::Scan("engine.dll", "40 53 48 83 EC 40 80 3D ?? ?? ?? ?? ?? 48 8B D9 0F 29 74 24 ?? 0F 28 F1 74 2B 80 3D ?? ?? ?? ?? ?? 75 22", 0, 0);
  FilterTimeHook.Install(ptrFilterTime, &Recorder::FilterTime, this);
}

void Recorder::Unload() { FilterTimeHook.Remove(); }

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
