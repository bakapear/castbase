#include "DemoParticleFix.h"

#include <cdll_int.h>

#include "base/Interfaces.h"
#include "base/Sig.h"

void DemoParticleFix::Load() {
  if (conVarToggle.GetBool()) {
    void* ptrParticlePropertyCreate = Sig::Scan(SIGPAT_ParticlePropertyCreate);
    hookParticlePropertyCreate.Install(ptrParticlePropertyCreate, &DemoParticleFix::ParticlePropertyCreate, this);
  } else {
    hookParticlePropertyCreate.Remove();
  }
}

void DemoParticleFix::Unload() {
  if (conVarToggle.GetBool()) hookParticlePropertyCreate.Remove();
}

inline void DemoParticleFix::OnConVarChanged(IConVar* pConVar, const char* pOldValue, float flOldValue) { m_DemoParticleFix.Load(); }

CNewParticleEffect* DemoParticleFix::ParticlePropertyCreate(void* p, const char* pszParticleName, ParticleAttachment_t iAttachType, int iAttachmentPoint, Vector vecOriginOffset) {
  if (Interfaces::EngineClient->IsPlayingDemo() && Interfaces::EngineClient->IsSkippingPlayback()) return NULL;

  auto fn = hookParticlePropertyCreate.GetTrampoline(&DemoParticleFix::ParticlePropertyCreate);
  return fn(p, pszParticleName, iAttachType, iAttachmentPoint, vecOriginOffset);
}
