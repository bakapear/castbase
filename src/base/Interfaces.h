#pragma once

#include "interface.h"

class C_BasePlayer;
class C_GameRules;
class C_HLTVCamera;
struct cmdalias_t;
class HLTVCameraOverride;
class CSteamAPIContext;
class IBaseClientDLL;
class IClientEngineTools;
class IClientEntityList;
class IClientMode;
class IClientRenderTargets;
class IEngineTool;
class IGameEventManager2;
class IGameSystem;
class IGameSystemPerFrame;
class IPrediction;
class IStaticPropMgrClient;
class IVEngineClient;
class IVModelInfoClient;
class IVRenderView;
class IFileSystem;
class IVDebugOverlay;
class IEngineTrace;
class ISpatialPartition;
class IMaterialSystem;
class IShaderAPI;
class IClientLeafSystem;

class Interfaces final {
 public:
  Interfaces() = delete;
  ~Interfaces() = delete;

  static void Load(CreateInterfaceFn factory);
  static void Unload();

 public:
  // #include <cdll_int.h>
  static IBaseClientDLL* ClientDLL;
  // #include <toolframework/iclientenginetools.h>
  static IClientEngineTools* ClientEngineTools;
  // #include <client/cliententitylist.h>
  static IClientEntityList* ClientEntityList;
  // #include <engine/IStaticPropMgr.h>
  static IStaticPropMgrClient* StaticPropMgr;
  // #include <cdll_int.h>
  static IVEngineClient* EngineClient;
  // #include <toolframework/ienginetool.h>
  static IEngineTool* EngineTool;
  // #include <igameevents.h>
  static IGameEventManager2* GameEventManager;
  // #include <engine/ivmodelinfo.h>
  static IVModelInfoClient* ModelInfoClient;
  // #include <iprediction.h>
  static IPrediction* Prediction;
  // #include <imaterialsystem.h>
  static IMaterialSystem* MaterialSystem;
  // #include <shaderapi/ishaderapi.h>
  static IShaderAPI* ShaderAPI;
  // #include <ivrenderview.h>
  static IVRenderView* RenderView;
  // #include <steam/steam_api.h>
  static CSteamAPIContext* SteamAPIContext;
  // #include <filesystem.h>
  static IFileSystem* FileSystem;
  // #include <engine/ivdebugoverlay.h>
  static IVDebugOverlay* DebugOverlay;
  // #include <engine/IEngineTrace.h>
  static IEngineTrace* EngineTrace;
  // #include <ispatialpartition.h>
  static ISpatialPartition* SpatialPartition;
  // #include <clientleafsystem.h>
  static IClientLeafSystem* ClientLeafSystem;
  // #include <game/client/iclientrendertargets.h>
  static IClientRenderTargets* ClientRenderTargets;

  static bool SteamLibrariesAvailable;
  static bool VGUILibrariesAvailable;
};