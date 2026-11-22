#pragma once

#define _SIGPAT_EXP(module, pattern, offs, disp) module, pattern, offs, disp

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/game/client/clientsteamcontext.cpp#L58
// void CClientSteamContext::Activate()
#define SIGPAT_ClientSteamContextActivate _SIGPAT_EXP("client.so", "80 BF 30 01 00 00 00 74 ??", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/game/client/c_baseplayer.cpp#L297
// RecvPropEHandle ( RECVINFO(m_hObserverTarget), RecvProxy_ObserverTarget )
#define RECVOFFS_ObserverTarget 0x1648

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/game/client/c_baseplayer.cpp#L238
// RecvPropFloat ( RECVINFO(m_vecVelocity[0]), 0, RecvProxy_LocalVelocityX )
#define RECVOFFS_Velocity 0x168

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/common/GameUI/IGameUI.h#L59
// virtual void OnGameUIActivated() = 0;
#define SIGVTI_OnGameUIActivated 6

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/cl_demoactionmanager.cpp#L193
// void CDemoActionManager::StartPlaying( char const *demfilename )
#define SIGPAT_StartPlaying _SIGPAT_EXP("engine.so", "55 48 8D 15 ?? ?? ?? ?? 48 89 E5 41 56 49 89 F6 41 55 41 54 49 89 FC 53 48 81 EC 00 02 00 00 48 8B 07 48 8B 40 28", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/cl_demo.cpp#L838
// void CDemoPlayer::StopPlayback( void )
#define SIGPAT_StopPlayback _SIGPAT_EXP("engine.so", "48 8D 05 ?? ?? ?? ?? 55 48 89 E5 41 56 41 55 41 54 53 48 89 FB 48 8B 38 4C 8D 63 08 48 8B 07 FF 50 28 4C 89 E7 E8 ?? ?? ?? ?? 31 D2 80 BB 3E 06 00 00 00 C6 83 C4 05 00 00 00", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/sys_engine.cpp#L234
// bool CEngine::FilterTime( float dt )
#define SIGPAT_FilterTime _SIGPAT_EXP("engine.so", "55 0F 28 C8 48 89 E5 53 48 89 FB 48 83 EC 18 48 8D 05 ?? ?? ?? ?? 80 B8 A0 01 00 00 00 74 ??", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/audio/private/snd_dma.cpp#L7041
// int g_paintedtime;
#define SIGPAT_PaintedTime _SIGPAT_EXP("engine.so", "44 8B 25 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 3D ?? ?? ?? ?? E8 ?? ?? ?? ??", 3, 7)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/audio/private/snd_mix.cpp#L76
// portable_samplepair_t *g_paintbuffer;
#define SIGPAT_PaintBuffer _SIGPAT_EXP("engine.so", "48 8D 05 ?? ?? ?? ?? 48 89 BD ?? ?? ?? ?? 4C 8B 20 48 8D 05 ?? ?? ?? ?? 48 8B 18 48 8D 05 ?? ?? ?? ?? 4C 8B 38 E8 ?? ?? ?? ??", 3, 7)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/audio/private/snd_mix.cpp#L2265
// void MIX_PaintChannels( int endtime, bool bIsUnderwater )
#define SIGPAT_MixPaintChannels _SIGPAT_EXP("engine.so", "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 81 EC 38 02 00 00 48 8B 05 ?? ?? ?? ?? 89 BD C4 FD FF FF 40 88 B5 C2 FD FF FF", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/audio/private/snd_dev_wave.cpp#L542
// void IAudioDevice::TransferSamples( int end )
#define SIGPAT_TransferSamples _SIGPAT_EXP("engine.so", "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 81 EC 78 60 00 00 48 8D 05 ?? ?? ?? ?? 48 89 BD 70 9F FF FF 4C 8B 20 48 8D 05 ?? ?? ?? ?? 48 8B 18 48 8D 05 ?? ?? ?? ?? 4C 8B 38 E8 ?? ?? ?? ??", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/game/shared/particle_property.cpp#L125
// CNewParticleEffect *CParticleProperty::Create( const char *pszParticleName, ParticleAttachment_t iAttachType, int iAttachmentPoint, Vector vecOriginOffset )
#define SIGPAT_ParticlePropertyCreate _SIGPAT_EXP("client.so", "55 48 89 E5 41 57 41 56 49 89 F6 41 55 41 54 53 48 89 FB 48 83 EC 38 48 8D 05 ?? ?? ?? ?? 89 55 BC 89 4D B8", 0, 0)
