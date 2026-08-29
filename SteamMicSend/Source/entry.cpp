#include "SteamMicSend.h"

#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

bool InitModule()
{
    return true;
}

bool DeinitModule()
{
    return true;
}

BEGIN_FACTORY_DEF("SteamMic Tools", "", "")

DEF_CLASS2(INLINE_UID(0xA17D4C90, 0x6F3B4E22, 0x9A87B11C, 0xD54F708E),
           PClassInfo::kManyInstances,
           kVstAudioEffectClass,
           "SteamMic Send",
           0,
           "Fx|Tools",
           "1.0.0",
           kVstVersionString,
           SteamMicSend::createInstance)

END_FACTORY
