#include "Recorder.h"

#include <cdll_int.h>
#include <filesystem.h>

#include "../Resolution/Resolution.h"
#include "base/Interfaces.h"
#include "base/Sig.h"

#define OVERRIDE_DEM "__override.dem"
#define OVERRIDE_VDM "__override.vdm"

void Recorder::StartPlaying(void* p, char const* demfilename) {
  auto fn = hookStartPlaying.GetTrampoline(&Recorder::StartPlaying);

  if (g_pFullFileSystem->FileExists(OVERRIDE_VDM, "MOD")) {
    fn(p, OVERRIDE_DEM);
    g_pFullFileSystem->RemoveFile(OVERRIDE_VDM, "MOD");
  } else {
    fn(p, demfilename);
  }
}

void Recorder::Load() {
  if (g_pFullFileSystem->FileExists(OVERRIDE_VDM, "MOD")) g_pFullFileSystem->RemoveFile(OVERRIDE_VDM, "MOD");

  void* ptrStartPlaying = Sig::Scan(SIGPAT_StartPlaying);
  hookStartPlaying.Install(ptrStartPlaying, &Recorder::StartPlaying, this);

  Video.Load();
  Audio.Load();
}

void Recorder::Unload() {
  Audio.Unload();
  Video.Unload();

  hookStartPlaying.Remove();
}

void Recorder::Start(const char* outputFile) {
  if (isRecording) Stop();

  movie.fps = recorder_fps.GetInt();
  snprintf(movie.filename, sizeof(movie.filename), "%s", outputFile);

  ConVarRef fps_max("fps_max");
  fps_max.SetValue(0);

  ConVarRef mat_queue_mode("mat_queue_mode");
  mat_queue_mode.SetValue(2);

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(movie.fps);

  ConVarRef engine_no_focus_sleep("engine_no_focus_sleep");
  engine_no_focus_sleep.SetValue(0);

  ConVarRef snd_mix_async("snd_mix_async");
  snd_mix_async.SetValue(0);

  ConVarRef snd_noextraupdate("snd_noextraupdate");
  snd_noextraupdate.SetValue(1);

  Video.Start(movie.filename, movie.fps, recorder_encoder.GetString(), recorder_qp.GetInt());
  Audio.Start(movie.filename, movie.fps);

  isRecording = true;
  timeStart = std::chrono::high_resolution_clock::now();
  ConColorMsg(Color(255, 255, 0, 255), "Started recording <%s>\n", movie.filename);
}

void Recorder::Stop() {
  isRecording = false;

  Video.Stop();
  Audio.Stop();

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(0);

  const auto timeEnd = std::chrono::high_resolution_clock::now();
  timeTotal = std::chrono::duration<float>(timeEnd - timeStart);

  int width, height;
  Interfaces::EngineClient->GetScreenSize(width, height);
  ConColorMsg(Color(255, 255, 0, 255), "Finished recording <%s> in %1.2fs (%ix%i @ %i fps)\n", movie.filename, timeTotal.count(), width, height, movie.fps);
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
  if (!Interfaces::EngineClient->IsPlayingDemo()) return ConWarning("Not in a demo!\n");
  if (!Video.initialized) return ConWarning("Recorder not initialized!\n");
  if (isRecording) return ConWarning("Already recording!\n");

  Start(args[1]);
}

void Recorder::recorder_stop(const CCommand& args) {
  if (!isRecording) return ConWarning("Not recording!\n");

  Stop();
}

void Recorder::recorder_run(const CCommand& args) {
  if (args.ArgC() < 5) return ConMsg("Usage:  recorder_run <filename> <demo> <start tick> <end tick> (steam id)\n");
  if (!Video.initialized) return ConWarning("Recorder not initialized!\n");
  if (isRecording) return ConWarning("Already recording!\n");
  if (!g_pFullFileSystem->FileExists(args[2], "MOD")) return ConWarning("Demo file \"%s\" not found!", args[2]);

  int tick_start = atoi(args[3]);
  int tick_end = atoi(args[4]);

  if (tick_start >= tick_end) return ConWarning("Invalid tick range!\n");

  char player[64] = "";
  if (args.ArgC() > 5) snprintf(player, sizeof(player), "%s", args[5]);

  FileHandle_t file = g_pFullFileSystem->Open(OVERRIDE_VDM, "w", "MOD");
  if (file == FILESYSTEM_INVALID_HANDLE) return ConWarning("Failed to create override vdm file!");

  char buffer[2048];

  int len = V_snprintf(buffer, sizeof(buffer),
                       "demoactions {\n"
                       "\t\"1\" { name \"1\" factory \"PlayCommands\" starttick \"0\" commands \"demo_gototick %i\" }\n"
                       "\t\"2\" { name \"2\" factory \"PlayCommands\" starttick \"%i\" commands \"spec_mode 4; spec_player %s; recorder_start %s\" }\n"
                       "\t\"3\" { name \"3\" factory \"PlayCommands\" starttick \"%i\" commands \"recorder_stop; stopdemo\" }\n"
                       "}",
                       tick_start, tick_start, player, args[1], tick_end);

  g_pFullFileSystem->Write(buffer, len, file);
  g_pFullFileSystem->Close(file);

  char cmd[256];
  snprintf(cmd, sizeof(cmd), "playdemo \"%s\"", args[2]);
  Interfaces::EngineClient->ClientCmd_Unrestricted(cmd);
}
