#include "Recorder.h"

#include <cdll_int.h>

#include "../Resolution/Resolution.h"
#include "base/Interfaces.h"
#include "base/Sig.h"

void Recorder::Load() {
  Video.Load();
  Audio.Load();
}

void Recorder::Unload() {
  Audio.Unload();
  Video.Unload();
}

void Recorder::Start(const char* outputFile, int fps, const char* encoder, int qp) {
  movie.fps = fps;
  snprintf(movie.filename, sizeof(movie.filename), "%s", outputFile);

  ConVarRef fps_max("fps_max");
  fps_max.SetValue(0);

  ConVarRef mat_queue_mode("mat_queue_mode");
  mat_queue_mode.SetValue(2);

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(movie.fps);

  ConVarRef engine_no_focus_sleep("engine_no_focus_sleep");
  engine_no_focus_sleep.SetValue(0);

  Video.Start(movie.filename, movie.fps, encoder, qp);
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

void Recorder::recorder_init(const CCommand& args) {
  if (Video.initialized) return ConMsg("Recorder already initialized!\n");

  int width, height;
  Interfaces::EngineClient->GetScreenSize(width, height);

  Video.onInitialized = [this]() {
    int width, height;
    Interfaces::EngineClient->GetScreenSize(width, height);
    m_Resolution.SetResolution(width, height - 2);
  };

  m_Resolution.SetResolution(width, height + 2);
}

void Recorder::recorder_start(const CCommand& args) {
  if (args.ArgC() != 2) return ConMsg("Usage:  recorder_start <filename>\n");
  if (!Interfaces::EngineClient->IsPlayingDemo()) return ConMsg("Not in a demo!\n");
  if (!Video.initialized) return ConMsg("Recorder not initialized!\n");

  Start(args[1], recorder_fps.GetInt(), recorder_encoder.GetString(), recorder_qp.GetInt());

  timeStart = std::chrono::high_resolution_clock::now();
}

void Recorder::recorder_stop(const CCommand& args) {
  if (!isRecording) return ConMsg("Not recording!\n");

  Stop();

  int width, height;
  Interfaces::EngineClient->GetScreenSize(width, height);

  const auto endTime = std::chrono::high_resolution_clock::now();
  const auto delta = std::chrono::duration<float>(endTime - timeStart);
  ConColorMsg(Color(255, 255, 0, 255), "Finished recording <%s> in %1.2fs (%ix%i @ %i fps)\n", movie.filename, delta.count(), width, height, movie.fps);
}
