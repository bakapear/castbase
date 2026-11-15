#include "DemoParticleFix.h"

#include <cdll_int.h>

#include "base/Interfaces.h"
#include "base/Sig.h"

bool DemoParticleFix::Load() {
  void* ptrParticlePropertyCreate = Sig::Scan(SIGPAT_ParticlePropertyCreate);
  if (!hookParticlePropertyCreate.Install(ptrParticlePropertyCreate, &DemoParticleFix::ParticlePropertyCreate, this)) return false;

  return true;
}

void DemoParticleFix::Unload() { hookParticlePropertyCreate.Remove(); }

CNewParticleEffect* DemoParticleFix::ParticlePropertyCreate(void* p, const char* pszParticleName, ParticleAttachment_t iAttachType, int iAttachmentPoint, Vector vecOriginOffset) {
  if (demo_particlefix.GetBool() && Interfaces::EngineClient->IsPlayingDemo() && Interfaces::EngineClient->IsSkippingPlayback()) return NULL;

  auto fn = hookParticlePropertyCreate.GetTrampoline(&DemoParticleFix::ParticlePropertyCreate);
  return fn(p, pszParticleName, iAttachType, iAttachmentPoint, vecOriginOffset);
}
