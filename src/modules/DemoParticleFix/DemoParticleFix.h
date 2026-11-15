#pragma once

#include <convar.h>

#include "base/Hook.h"
#include "base/Modules.h"

enum ParticleAttachment_t {
};

class CNewParticleEffect;

class DemoParticleFix : public Module {
 public:
  const char *GetModuleName() { return "DemoParticleFix"; }

  bool Load();
  void Unload();

 private:
  ConVar demo_particlefix{"demo_particlefix", "1", FCVAR_NONE, "Hides particles during demo skip"};

  CNewParticleEffect *ParticlePropertyCreate(void *p, const char *pszParticleName, ParticleAttachment_t iAttachType, int iAttachmentPoint, Vector vecOriginOffset);
  Hook hookParticlePropertyCreate;
};

inline DemoParticleFix m_DemoParticleFix;
