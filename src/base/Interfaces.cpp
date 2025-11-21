#include "Interfaces.h"

#include <cdll_int.h>
#include <client/cliententitylist.h>
#include <engine/IEngineTrace.h>
#include <engine/IStaticPropMgr.h>
#include <engine/ivdebugoverlay.h>
#include <engine/ivmodelinfo.h>
#include <entitylist_base.h>
#include <filesystem.h>
#include <game/client/iclientrendertargets.h>
#include <icliententitylist.h>
#include <igameevents.h>
#include <iprediction.h>
#include <ivrenderview.h>
#include <shaderapi/ishaderapi.h>
#include <tier1/tier1.h>
#include <tier2/tier2.h>
#include <tier3/tier3.h>
#include <toolframework/iclientenginetools.h>
#include <toolframework/ienginetool.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/HTML.h>

#include "base/Sig.h"
#include "common.h"

IBaseClientDLL* Interfaces::ClientDLL = nullptr;
IClientEngineTools* Interfaces::ClientEngineTools = nullptr;
IClientEntityList* Interfaces::ClientEntityList = nullptr;
IStaticPropMgrClient* Interfaces::StaticPropMgr;
IVEngineClient* Interfaces::EngineClient = nullptr;
IEngineTool* Interfaces::EngineTool = nullptr;
IGameEventManager2* Interfaces::GameEventManager = nullptr;
IPrediction* Interfaces::Prediction = nullptr;
IVModelInfoClient* Interfaces::ModelInfoClient = nullptr;
IVRenderView* Interfaces::RenderView = nullptr;
IMaterialSystem* Interfaces::MaterialSystem = nullptr;
IShaderAPI* Interfaces::ShaderAPI = nullptr;
CSteamAPIContext* Interfaces::SteamAPIContext = nullptr;
IFileSystem* Interfaces::FileSystem = nullptr;
IVDebugOverlay* Interfaces::DebugOverlay = nullptr;
IEngineTrace* Interfaces::EngineTrace = nullptr;
ISpatialPartition* Interfaces::SpatialPartition = nullptr;
IClientLeafSystem* Interfaces::ClientLeafSystem = nullptr;
IClientRenderTargets* Interfaces::ClientRenderTargets = nullptr;

bool Interfaces::SteamLibrariesAvailable = false;
bool Interfaces::VGUILibrariesAvailable = false;

void Interfaces::Load(CreateInterfaceFn factory) {
  ConnectTier1Libraries(&factory, 1);
  ConnectTier2Libraries(&factory, 1);
  ConnectTier3Libraries(&factory, 1);

  if (!factory) Error("factory is null");

  VGUILibrariesAvailable = vgui::VGui_InitInterfacesList(PLUGIN_NAME, &factory, 1);

  ClientEngineTools = (IClientEngineTools*)factory(VCLIENTENGINETOOLS_INTERFACE_VERSION, nullptr);
  EngineClient = (IVEngineClient*)factory(VENGINE_CLIENT_INTERFACE_VERSION, nullptr);
  EngineTool = (IEngineTool*)factory(VENGINETOOL_INTERFACE_VERSION, nullptr);
  GameEventManager = (IGameEventManager2*)factory(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);
  ModelInfoClient = (IVModelInfoClient*)factory(VMODELINFO_CLIENT_INTERFACE_VERSION, nullptr);
  RenderView = (IVRenderView*)factory(VENGINE_RENDERVIEW_INTERFACE_VERSION, nullptr);
  MaterialSystem = (IMaterialSystem*)factory(MATERIAL_SYSTEM_INTERFACE_VERSION, nullptr);
  ShaderAPI = (IShaderAPI*)factory(SHADERAPI_INTERFACE_VERSION, nullptr);
  FileSystem = (IFileSystem*)factory(FILESYSTEM_INTERFACE_VERSION, nullptr);
  DebugOverlay = (IVDebugOverlay*)factory(VDEBUG_OVERLAY_INTERFACE_VERSION, nullptr);
  EngineTrace = (IEngineTrace*)factory(INTERFACEVERSION_ENGINETRACE_CLIENT, nullptr);
  SpatialPartition = (ISpatialPartition*)factory(INTERFACEVERSION_SPATIALPARTITION, nullptr);
  ClientRenderTargets = (IClientRenderTargets*)factory(CLIENTRENDERTARGETS_INTERFACE_VERSION, nullptr);
  StaticPropMgr = (IStaticPropMgrClient*)factory(INTERFACEVERSION_STATICPROPMGR_CLIENT, nullptr);

  CreateInterfaceFn gameClientFactory;
  EngineTool->GetClientFactory(gameClientFactory);

  if (!gameClientFactory) Error("gameClientFactory is null");

  ClientDLL = (IBaseClientDLL*)gameClientFactory(CLIENT_DLL_INTERFACE_VERSION, nullptr);
  ClientEntityList = (IClientEntityList*)gameClientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, nullptr);
  Prediction = (IPrediction*)gameClientFactory(VCLIENT_PREDICTION_INTERFACE_VERSION, nullptr);
  ClientLeafSystem = (IClientLeafSystem*)gameClientFactory(CLIENTLEAFSYSTEM_INTERFACE_VERSION, nullptr);

  SteamAPIContext = new CSteamAPIContext();
  SteamLibrariesAvailable = SteamAPI_InitSafe() && SteamAPIContext->Init();
}

void Interfaces::Unload() {
  ClientEngineTools = nullptr;
  EngineClient = nullptr;
  EngineTool = nullptr;
  GameEventManager = nullptr;
  ModelInfoClient = nullptr;
  RenderView = nullptr;
  MaterialSystem = nullptr;
  ShaderAPI = nullptr;
  FileSystem = nullptr;
  DebugOverlay = nullptr;
  EngineTrace = nullptr;
  SpatialPartition = nullptr;
  StaticPropMgr = nullptr;
  ClientDLL = nullptr;
  ClientEntityList = nullptr;
  Prediction = nullptr;
  ClientLeafSystem = nullptr;
  ClientRenderTargets = nullptr;
  SteamAPIContext = nullptr;

  SteamLibrariesAvailable = false;
  VGUILibrariesAvailable = false;

  DisconnectTier3Libraries();
  DisconnectTier2Libraries();
  DisconnectTier1Libraries();
}