#include "Recorder.h"

#include <cdll_int.h>

#include "base/Interfaces.h"
#include "base/Sig.h"

bool Recorder::FilterTime(void* p, float dt) {
  if (!isRecording) {
    auto fn = hookFilterTime.GetTrampoline(&Recorder::FilterTime);
    return fn(p, dt);
  }

  Frame();
  return true;
}

void Recorder::Load() {
  void* ptrFilterTime = Sig::Scan("engine.dll", "40 53 48 83 EC 40 80 3D ?? ?? ?? ?? ?? 48 8B D9 0F 29 74 24 ?? 0F 28 F1 74 2B 80 3D ?? ?? ?? ?? ?? 75 22", 0, 0);
  hookFilterTime.Install(ptrFilterTime, &Recorder::FilterTime, this);

  Video.Load();
  Audio.Load();
}

void Recorder::Unload() {
  Audio.Unload();
  Video.Unload();

  hookFilterTime.Remove();
}

void Recorder::Start(const char* outputFile, int fps) {
  movie.fps = fps;
  snprintf(movie.filename, sizeof(movie.filename), "%s", outputFile);

  Interfaces::EngineClient->GetScreenSize(movie.width, movie.height);

  ConVarRef fps_max("fps_max");
  fps_max.SetValue(0);

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(movie.fps);

  ConVarRef engine_no_focus_sleep("engine_no_focus_sleep");
  engine_no_focus_sleep.SetValue(0);

  Video.Start(movie.filename, movie.fps, movie.width, movie.height);
  Audio.Start(movie.filename, movie.fps);

  isRecording = true;
}

void Recorder::Stop() {
  isRecording = false;

  Video.Stop();
  Audio.Stop();

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(0);
}

void Recorder::recorder_start(const CCommand& args) {
  if (args.ArgC() != 2) return ConMsg("Usage:  recorder_start <filename>\n");
  if (!Interfaces::EngineClient->IsPlayingDemo()) return ConMsg("Not in a demo!\n");

  Start(args[1], 60);

  timeStart = std::chrono::high_resolution_clock::now();
}

void Recorder::recorder_stop(const CCommand& args) {
  if (!isRecording) return ConMsg("Not recording!\n");

  Stop();

  const auto endTime = std::chrono::high_resolution_clock::now();
  const auto delta = std::chrono::duration<float>(endTime - timeStart);
  ConColorMsg(Color(255, 255, 0, 255), "Finished recording <%s> in %1.2fs (%ix%i @ %i fps)\n", movie.filename, delta.count(), movie.width, movie.height, movie.fps);
}

void Recorder::Frame() {
  Video.Frame();
  Audio.Frame();
}
