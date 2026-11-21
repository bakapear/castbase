#define _SIGPAT_EXP(module, pattern, offs, disp) module, pattern, offs, disp

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/game/client/clientsteamcontext.cpp#L58
// void CClientSteamContext::Activate()
#define SIGPAT_ClientSteamContextActivate _SIGPAT_EXP("client.dll", "40 53 48 83 EC 20 80 B9 18 01 00 00 00 48 8B D9 75 ?? C6 81 18 01 00 00 01", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/game/client/c_baseplayer.cpp#L297
// RecvPropEHandle ( RECVINFO(m_hObserverTarget), RecvProxy_ObserverTarget )
#define RECVOFFS_ObserverTarget 0x1650

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/game/client/c_baseplayer.cpp#L238
// RecvPropFloat ( RECVINFO(m_vecVelocity[0]), 0, RecvProxy_LocalVelocityX )
#define RECVOFFS_Velocity 0x178

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/common/GameUI/IGameUI.h#L59
// virtual void OnGameUIActivated() = 0;
#define SIGVTI_OnGameUIActivated 6

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/cl_demoactionmanager.cpp#L193
// void CDemoActionManager::StartPlaying( char const *demfilename )
#define SIGPAT_StartPlaying _SIGPAT_EXP("engine.dll", "48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 41 56 48 81 EC 20 02 00 00 48 8B 01 48 8B FA 4C 8B F1 FF 50 20", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/cl_demo.cpp#L838
// void CDemoPlayer::StopPlayback( void )
#define SIGPAT_StopPlayback _SIGPAT_EXP("engine.dll", "40 53 48 83 EC 30 48 8B 01 48 8B D9 FF 50 30 84 C0 0F 84 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 89 74 24 40 0F 29 74 24 20 48 8B 01 FF 50 20 48 8D 4B 08 E8 ?? ?? ?? ??", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/sys_engine.cpp#L234
// bool CEngine::FilterTime( float dt )
#define SIGPAT_FilterTime _SIGPAT_EXP("engine.dll", "40 53 48 83 EC 40 80 3D ?? ?? ?? ?? ?? 48 8B D9 0F 29 74 24 ?? 0F 28 F1 74 2B 80 3D ?? ?? ?? ?? ?? 75 22", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/audio/private/snd_dma.cpp#L7041
// int g_paintedtime;
#define SIGPAT_PaintedTime _SIGPAT_EXP("engine.dll", "8B 3D ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ??", 2, 4)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/audio/private/snd_mix.cpp#L76
// portable_samplepair_t *g_paintbuffer;
#define SIGPAT_PaintBuffer _SIGPAT_EXP("engine.dll", "48 8B 3D ?? ?? ?? ?? 48 89 B5 ?? ?? ?? ?? 48 89 9D ?? ?? ?? ?? 0F 29 B4 24 ?? ?? ?? ??", 3, 4)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/audio/private/snd_mix.cpp#L2265
// void MIX_PaintChannels( int endtime, bool bIsUnderwater )
#define SIGPAT_MixPaintChannels _SIGPAT_EXP("engine.dll", "48 8B C4 88 50 10 89 48 08 53 48 81 EC ?? ?? ?? ?? 48 89 78 E0 33 FF 4C 89 68 D0 4C 89 78 C0", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/engine/audio/private/snd_dev_wave.cpp#L542
// void IAudioDevice::TransferSamples( int end )
#define SIGPAT_TransferSamples _SIGPAT_EXP("engine.dll", "48 89 5C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ??", 0, 0)

// https://github.com/mastercomfig/tf2-patches-old/blob/3598ce3f78810b5caf7a3c7ec363588709a73e6d/src/game/shared/particle_property.cpp#L125
// CNewParticleEffect *CParticleProperty::Create( const char *pszParticleName, ParticleAttachment_t iAttachType, int iAttachmentPoint, Vector vecOriginOffset )
#define SIGPAT_ParticlePropertyCreate _SIGPAT_EXP("client.dll", "44 89 4C 24 20 44 89 44 24 18 53 55 56 57", 0, 0)
