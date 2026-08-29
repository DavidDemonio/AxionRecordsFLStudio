#pragma once

#include "public.sdk/source/vst/vstsinglecomponenteffect.h"

#include <array>
#include <atomic>
#include <cstdint>

struct ma_context;
struct ma_device;

namespace Steinberg::Vst
{

class SteamMicSend final : public SingleComponentEffect
{
public:
    SteamMicSend();
    ~SteamMicSend() override;

    static FUnknown* createInstance(void*)
    {
        return static_cast<IAudioProcessor*>(new SteamMicSend());
    }

    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API terminate() override;
    tresult PLUGIN_API setupProcessing(ProcessSetup& setup) override;
    tresult PLUGIN_API setActive(TBool state) override;
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs,
                                           int32 numIns,
                                           SpeakerArrangement* outputs,
                                           int32 numOuts) override;
    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
    tresult PLUGIN_API process(ProcessData& data) override;
    IPlugView* PLUGIN_API createView(const char*) override { return nullptr; }

    void renderVirtualMic(float* outputInterleaved, uint32_t frameCount) noexcept;

private:
    static constexpr uint32_t kRingFrames = 1u << 17; // ~2.7 s at 48 kHz
    static constexpr uint32_t kRingMask = kRingFrames - 1;

    void clearRing() noexcept;
    void pushStereo32(const float* left, const float* right, uint32_t frames) noexcept;
    void pushStereo64(const double* left, const double* right, uint32_t frames) noexcept;
    bool startVirtualMic();
    void stopVirtualMic() noexcept;

    std::array<float, kRingFrames * 2> ring_{};
    std::atomic<uint64_t> writeFrame_{0};
    std::atomic<uint64_t> readFrame_{0};

    double hostSampleRate_{48000.0};
    bool active_{false};

    ma_context* audioContext_{nullptr};
    ma_device* audioDevice_{nullptr};
};

} // namespace Steinberg::Vst
