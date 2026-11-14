#include "Recorder.h"

#include <cdll_int.h>
#include <filesystem.h>

#include "../Resolution/Resolution.h"
#include "base/Interfaces.h"
#include "base/Sig.h"

#define OVERRIDE_DEM "__override.dem"
#define OVERRIDE_VDM "__override.vdm"

std::string EnsureExt(const char* filename, const char* extension) {
  std::string result = filename ? filename : "";
  std::string ext = extension ? extension : "";
  if (ext.empty() || result.empty()) return result;
  if (result.size() < ext.size() || result.compare(result.size() - ext.size(), ext.size(), ext) != 0) result += ext;
  return result;
}

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

  movie.filename = EnsureExt(outputFile, ".mp4");
  movie.fps = recorder_fps.GetInt();
  Interfaces::EngineClient->GetScreenSize(movie.width, movie.height);

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

  Video.Start(movie.filename.c_str(), movie.fps, recorder_encoder.GetString(), recorder_qp.GetInt());
  Audio.Start(movie.filename.c_str(), movie.fps);

  isRecording = true;
  timeStart = std::chrono::high_resolution_clock::now();
  ConColorMsg(Color(255, 255, 0, 255), "Started recording <%s>\n", movie.filename.c_str());
}

void Recorder::Stop() {
  isRecording = false;

  Video.Stop();
  Audio.Stop();

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(0);

  const auto timeEnd = std::chrono::high_resolution_clock::now();
  const auto timeTotal = std::chrono::duration<float>(timeEnd - timeStart);
  ConColorMsg(Color(255, 255, 0, 255), "Finished recording <%s> in %1.2fs (%ix%i @ %i fps)\n", movie.filename.c_str(), timeTotal.count(), movie.width, movie.height, movie.fps);
}

void Recorder::recorder_init(const CCommand& args) {
  if (Video.initialized) return ConMsg("Recorder already initialized!\n");
  if (GetModuleHandleA("shaderapivk.dll") == nullptr) return ConWarning("Recorder only supports Vulkan!\n");

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

  std::string filename = EnsureExt(args[1], ".mp4");

  std::string demofile = EnsureExt(args[2], ".dem");
  if (!g_pFullFileSystem->FileExists(demofile.c_str(), "MOD")) return ConWarning("Demo file \"%s\" not found!", demofile.c_str());

  int tick_start = atoi(args[3]);
  int tick_end = atoi(args[4]);

  if (tick_start >= tick_end) return ConWarning("Invalid tick range!\n");

  std::string player;
  if (args.ArgC() > 5) player = args[5];

  FileHandle_t file = g_pFullFileSystem->Open(OVERRIDE_VDM, "w", "MOD");
  if (file == FILESYSTEM_INVALID_HANDLE) return ConWarning("Failed to create override vdm file!");

  char buffer[2048];

  int len = V_snprintf(buffer, sizeof(buffer),
                       "demoactions {\n"
                       "\t\"1\" { name \"1\" factory \"PlayCommands\" starttick \"0\" commands \"demo_gototick %i\" }\n"
                       "\t\"2\" { name \"2\" factory \"PlayCommands\" starttick \"%i\" commands \"spec_mode 4; spec_player %s; recorder_start %s\" }\n"
                       "\t\"3\" { name \"3\" factory \"PlayCommands\" starttick \"%i\" commands \"recorder_stop; stopdemo\" }\n"
                       "}",
                       tick_start, tick_start, player.c_str(), filename.c_str(), tick_end);

  g_pFullFileSystem->Write(buffer, len, file);
  g_pFullFileSystem->Close(file);

  char cmd[256];
  snprintf(cmd, sizeof(cmd), "playdemo \"%s\"", args[2]);
  Interfaces::EngineClient->ClientCmd_Unrestricted(cmd);
}
