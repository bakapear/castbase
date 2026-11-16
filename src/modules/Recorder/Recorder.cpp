#include "Recorder.h"

#include <cdll_int.h>
#include <demofile/demoformat.h>
#include <filesystem.h>
#include <replay/ienginereplay.h>

#include "base/Interfaces.h"
#include "base/Sig.h"

#define OVERRIDE_DEM "__override.dem"
#define OVERRIDE_VDM "__override.vdm"

std::string FixColonArgs(const CCommand& args, int start = 1) {
  std::string result;
  for (int i = start; i < args.ArgC(); ++i) {
    if (!result.empty() && args[i][0] != ':' && args[i - 1][0] != ':') result += " ";
    result += args[i];
  }
  return result;
}

std::string EnsureExt(const char* filename, const char* extension) {
  std::string result = filename ? filename : "";
  std::string ext = extension ? extension : "";
  if (ext.empty() || result.empty()) return result;
  if (result.size() < ext.size() || result.compare(result.size() - ext.size(), ext.size(), ext) != 0) result += ext;
  return result;
}

void Recorder::StartPlaying(void* p, char const* demfilename) {
  auto fn = hookStartPlaying.GetTrampoline(&Recorder::StartPlaying);
  PluginOut("DEMO_LAUNCH");

  if (g_pFullFileSystem->FileExists(OVERRIDE_VDM, "MOD")) {
    fn(p, OVERRIDE_DEM);
    g_pFullFileSystem->RemoveFile(OVERRIDE_VDM, "MOD");
  } else {
    fn(p, demfilename);
  }
}

void Recorder::StopPlayback(void* p) {
  auto fn = hookStopPlayback.GetTrampoline(&Recorder::StopPlayback);

  if (Interfaces::EngineClient->IsPlayingDemo()) {
    if (isRecording) Stop();
    PluginOut("DEMO_EXIT");
  }

  fn(p);
}

void Recorder::OnGameUIActivated() {
  // dont call original, will crash game
  if (isRecording && recorder_opt_escstop.GetBool()) Stop();
}

bool Recorder::Load() {
  if (g_pFullFileSystem->FileExists(OVERRIDE_VDM, "MOD")) g_pFullFileSystem->RemoveFile(OVERRIDE_VDM, "MOD");

  void* ptrStartPlaying = Sig::Scan(SIGPAT_StartPlaying);
  if (!hookStartPlaying.Install(ptrStartPlaying, &Recorder::StartPlaying, this)) return false;

  void* ptrStopPlayback = Sig::Scan(SIGPAT_StopPlayback);
  if (!hookStopPlayback.Install(ptrStopPlayback, &Recorder::StopPlayback, this)) return false;

  void* ptrOnGameUIActivated = Sig::Virtual(Interfaces::GameUI, SIGVTI_OnGameUIActivated);
  if (!hookOnGameUIActivated.Install(ptrOnGameUIActivated, &Recorder::OnGameUIActivated, this)) return false;

  if (!Video.Load()) return false;
  if (!Audio.Load()) return false;

  return true;
}

void Recorder::Unload() {
  Audio.Unload();
  Video.Unload();

  hookStartPlaying.Remove();
  hookStopPlayback.Remove();
}

void Recorder::Start(const char* outputFile) {
  if (GetModuleHandleA("shaderapivk.dll") == nullptr) return ConWarning("Recorder only supports Vulkan!\n");

  if (isRecording) Stop();

  movie.filename = EnsureExt(outputFile, ".mp4");
  movie.fps = recorder_encoder_fps.GetInt();
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

  if (recorder_opt_hidehud.GetBool()) {
    ConVarRef cl_drawhud("cl_drawhud");
    cl_drawhud.SetValue(0);
  }

  if (recorder_opt_escstop.GetBool()) Interfaces::EngineClient->ClientCmd_Unrestricted("gameui_hide");

  Video.Start(movie.filename.c_str(), movie.fps, recorder_encoder_codec.GetString(), recorder_encoder_qp.GetInt(), SND_SAMPLE_RATE);
  Audio.Start(movie.fps, &Video);

  isRecording = true;
  timeStart = std::chrono::high_resolution_clock::now();
  ConColorMsg(Color(255, 255, 0, 255), "Started recording <%s>\n", movie.filename.c_str());

  PluginOut("RECORDER_START");
}

void Recorder::Stop() {
  isRecording = false;

  Audio.Stop();

  int frames = Video.GetFrameCount();
  Video.Stop();

  ConVarRef host_framerate("host_framerate");
  host_framerate.SetValue(0);

  if (recorder_opt_hidehud.GetBool()) {
    ConVarRef cl_drawhud("cl_drawhud");
    cl_drawhud.SetValue(1);
  }

  const auto timeEnd = std::chrono::high_resolution_clock::now();
  const auto timeTotal = std::chrono::duration<float>(timeEnd - timeStart);
  ConColorMsg(Color(255, 255, 0, 255), "Finished recording <%s> in %1.2fs (%i frames, %ix%i @ %i fps)\n", movie.filename.c_str(), timeTotal.count(), frames, movie.width, movie.height, movie.fps);

  char fullPath[MAX_PATH];
  g_pFullFileSystem->RelativePathToFullPath(movie.filename.c_str(), nullptr, fullPath, sizeof(fullPath));
  ConColorMsg(Color(255, 255, 0, 255), "Saved to: %s\n", fullPath);

  PluginOut("RECORDER_STOP");
}

void Recorder::recorder_start(const CCommand& args) {
  if (args.ArgC() != 2) return ConMsg("Usage:  recorder_start <filename>\n");
  if (!Interfaces::EngineClient->IsPlayingDemo()) return ConWarning("Not in a demo!\n");
  if (isRecording) return ConWarning("Already recording!\n");

  Start(args[1]);
}

void Recorder::recorder_stop(const CCommand& args) {
  if (!isRecording) return ConWarning("Not recording!\n");

  Stop();
}

void Recorder::recorder_run(const CCommand& args) {
  if (args.ArgC() < 5) return ConMsg("Usage:  recorder_run <filename> <demo> <start tick> <end tick> (<player name>/<steam id>)\n");
  if (isRecording) return ConWarning("Already recording!\n");

  std::string filename = EnsureExt(args[1], ".mp4");

  std::string demofile = EnsureExt(args[2], ".dem");
  if (!g_pFullFileSystem->FileExists(demofile.c_str(), "MOD")) return ConWarning("Demo file \"%s\" not found!", demofile.c_str());

  demoheader_t header{};
  Interfaces::EngineReplay->ReadDemoHeader(demofile.c_str(), header);
  int total = header.playback_ticks;

  int tick_start = atoi(args[3]);
  int tick_end = atoi(args[4]);

  if (tick_start < 10) tick_start = 10;
  if (tick_end > total) tick_end = total;

  if (tick_start >= tick_end - 100) return ConWarning("Invalid tick range!\n");

  std::string player = FixColonArgs(args, 5);

  FileHandle_t file = g_pFullFileSystem->Open(OVERRIDE_VDM, "w", "MOD");
  if (file == FILESYSTEM_INVALID_HANDLE) return ConWarning("Failed to create override vdm file!");

  std::string act = "";
  if (player.length()) act += "recorder_spec " + player + "; ";
  if (!recorder_opt_hidehud.GetBool()) act += "cl_drawhud 1; ";
  act += "recorder_start " + filename;

  std::string buffer = "demoactions {\n";
  buffer += "\"1\" { name \"1\" factory \"PlayCommands\" starttick \"0\" commands \"recorder_progress 0; cl_drawhud 0; demo_gototick " + std::to_string(tick_start) + "\" }\n";
  buffer += "\"1\" { name \"1\" factory \"PlayCommands\" starttick \"" + std::to_string(tick_start) + "\" commands \"" + act + "\" }\n";

  int lt = -1;
  for (int p = 1; p < 100; p++) {
    int t = tick_start + (tick_end - tick_start) * p / 100;
    if (t == lt) continue;
    lt = t;
    buffer += "\"1\" { name \"1\" factory \"PlayCommands\" starttick \"" + std::to_string(t) + "\" commands \"recorder_progress " + std::to_string(p) + "\" }\n";
  }

  buffer += "\"1\" { name \"1\" factory \"PlayCommands\" starttick \"" + std::to_string(tick_end) + "\" commands \"recorder_progress 100; recorder_stop; stopdemo\" }\n";
  buffer += "}";

  g_pFullFileSystem->Write(buffer.data(), (int)buffer.length(), file);
  g_pFullFileSystem->Close(file);

  char cmd[256];
  snprintf(cmd, sizeof(cmd), "playdemo \"%s\"", args[2]);
  Interfaces::EngineClient->ClientCmd_Unrestricted(cmd);
}

void Recorder::recorder_progress(const CCommand& args) {
  if (args.ArgC() != 2 || !Interfaces::EngineClient->IsPlayingDemo()) return;

  if (isRecording)
    PluginOut("RECORDER_PROGRESS_%s", args[1]);
  else if (atoi(args[1]) == 0)
    PluginOut("DEMO_SKIP");
}

void Recorder::recorder_spec(const CCommand& args) {
  if (args.ArgC() < 2 || !Interfaces::EngineClient->IsPlayingDemo()) return;

  std::string player = FixColonArgs(args, 1);

  char cmd[256];
  snprintf(cmd, sizeof(cmd), "spec_mode 4; spec_player \"%s\"", player.c_str());
  Interfaces::EngineClient->ClientCmd_Unrestricted(cmd);
}
