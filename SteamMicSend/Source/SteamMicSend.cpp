#include "SteamMicSend.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <new>
#include <string>

namespace Steinberg::Vst
{

namespace
{
constexpr const char* kTargetDeviceNeedle = "steam streaming microphone";

bool containsIgnoreCase(const char* haystack, const char* needle)
{
    if (haystack == nullptr || needle == nullptr)
        return false;

    std::string h(haystack);
    std::string n(needle);

    std::transform(h.begin(), h.end(), h.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return h.find(n) != std::string::npos;
}
} // namespace

SteamMicSend::SteamMicSend() = default;

SteamMicSend::~SteamMicSend()
{
    stopVirtualMic();
}

tresult PLUGIN_API SteamMicSend::initialize(FUnknown* context)
{
    const auto result = SingleComponentEffect::initialize(context);
    if (result != kResultOk)
        return result;

    addAudioInput(STR16("Audio In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Audio Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API SteamMicSend::terminate()
{
    stopVirtualMic();
    return SingleComponentEffect::terminate();
}

tresult PLUGIN_API SteamMicSend::setupProcessing(ProcessSetup& setup)
{
    hostSampleRate_ = setup.sampleRate > 1000.0 ? setup.sampleRate : 48000.0;
    clearRing();
    return SingleComponentEffect::setupProcessing(setup);
}

tresult PLUGIN_API SteamMicSend::setActive(TBool state)
{
    if (!state)
        stopVirtualMic();

    const auto result = SingleComponentEffect::setActive(state);
    if (result != kResultOk)
        return result;

    active_ = state != 0;
    if (active_)
        startVirtualMic();

    return kResultOk;
}

tresult PLUGIN_API SteamMicSend::setBusArrangements(SpeakerArrangement* inputs,
                                                       int32 numIns,
                                                       SpeakerArrangement* outputs,
                                                       int32 numOuts)
{
    if (numIns != 1 || numOuts != 1 || inputs == nullptr || outputs == nullptr)
        return kResultFalse;

    const auto inChannels = SpeakerArr::getChannelCount(inputs[0]);
    const auto outChannels = SpeakerArr::getChannelCount(outputs[0]);

    if (inChannels != outChannels || (inChannels != 1 && inChannels != 2))
        return kResultFalse;

    return SingleComponentEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

tresult PLUGIN_API SteamMicSend::canProcessSampleSize(int32 symbolicSampleSize)
{
    return (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64)
               ? kResultTrue
               : kResultFalse;
}

tresult PLUGIN_API SteamMicSend::process(ProcessData& data)
{
    if (data.numSamples <= 0 || data.numInputs < 1 || data.numOutputs < 1)
        return kResultOk;

    auto& inBus = data.inputs[0];
    auto& outBus = data.outputs[0];
    const int32 inChannels = inBus.numChannels;
    const int32 outChannels = outBus.numChannels;
    const int32 copyChannels = std::min(inChannels, outChannels);

    if (data.symbolicSampleSize == kSample32)
    {
        for (int32 c = 0; c < copyChannels; ++c)
        {
            const float* src = inBus.channelBuffers32[c];
            float* dst = outBus.channelBuffers32[c];
            if (src != nullptr && dst != nullptr && src != dst)
                std::memcpy(dst, src, static_cast<size_t>(data.numSamples) * sizeof(float));
        }

        if (active_ && inChannels > 0 && inBus.channelBuffers32[0] != nullptr)
        {
            const float* left = inBus.channelBuffers32[0];
            const float* right = (inChannels > 1 && inBus.channelBuffers32[1] != nullptr)
                                     ? inBus.channelBuffers32[1]
                                     : left;
            pushStereo32(left, right, static_cast<uint32_t>(data.numSamples));
        }
    }
    else if (data.symbolicSampleSize == kSample64)
    {
        for (int32 c = 0; c < copyChannels; ++c)
        {
            const double* src = inBus.channelBuffers64[c];
            double* dst = outBus.channelBuffers64[c];
            if (src != nullptr && dst != nullptr && src != dst)
                std::memcpy(dst, src, static_cast<size_t>(data.numSamples) * sizeof(double));
        }

        if (active_ && inChannels > 0 && inBus.channelBuffers64[0] != nullptr)
        {
            const double* left = inBus.channelBuffers64[0];
            const double* right = (inChannels > 1 && inBus.channelBuffers64[1] != nullptr)
                                      ? inBus.channelBuffers64[1]
                                      : left;
            pushStereo64(left, right, static_cast<uint32_t>(data.numSamples));
        }
    }

    outBus.silenceFlags = inBus.silenceFlags;
    return kResultOk;
}

void SteamMicSend::clearRing() noexcept
{
    readFrame_.store(0, std::memory_order_release);
    writeFrame_.store(0, std::memory_order_release);
}

void SteamMicSend::pushStereo32(const float* left, const float* right, uint32_t frames) noexcept
{
    if (left == nullptr || right == nullptr || frames == 0)
        return;

    const uint64_t w = writeFrame_.load(std::memory_order_relaxed);
    const uint64_t r = readFrame_.load(std::memory_order_acquire);
    const uint64_t used = w >= r ? (w - r) : 0;
    const uint32_t freeFrames = used >= kRingFrames ? 0u : static_cast<uint32_t>(kRingFrames - used);
    const uint32_t toWrite = std::min(frames, freeFrames);

    for (uint32_t i = 0; i < toWrite; ++i)
    {
        const uint32_t index = static_cast<uint32_t>((w + i) & kRingMask) * 2u;
        ring_[index] = left[i];
        ring_[index + 1u] = right[i];
    }

    if (toWrite != 0)
        writeFrame_.store(w + toWrite, std::memory_order_release);
}

void SteamMicSend::pushStereo64(const double* left, const double* right, uint32_t frames) noexcept
{
    if (left == nullptr || right == nullptr || frames == 0)
        return;

    const uint64_t w = writeFrame_.load(std::memory_order_relaxed);
    const uint64_t r = readFrame_.load(std::memory_order_acquire);
    const uint64_t used = w >= r ? (w - r) : 0;
    const uint32_t freeFrames = used >= kRingFrames ? 0u : static_cast<uint32_t>(kRingFrames - used);
    const uint32_t toWrite = std::min(frames, freeFrames);

    for (uint32_t i = 0; i < toWrite; ++i)
    {
        const uint32_t index = static_cast<uint32_t>((w + i) & kRingMask) * 2u;
        ring_[index] = static_cast<float>(left[i]);
        ring_[index + 1u] = static_cast<float>(right[i]);
    }

    if (toWrite != 0)
        writeFrame_.store(w + toWrite, std::memory_order_release);
}

void SteamMicSend::renderVirtualMic(float* outputInterleaved, uint32_t frameCount) noexcept
{
    if (outputInterleaved == nullptr || frameCount == 0)
        return;

    std::memset(outputInterleaved, 0, static_cast<size_t>(frameCount) * 2u * sizeof(float));

    const uint64_t r = readFrame_.load(std::memory_order_relaxed);
    const uint64_t w = writeFrame_.load(std::memory_order_acquire);
    const uint64_t available64 = w >= r ? (w - r) : 0;
    const uint32_t available = static_cast<uint32_t>(std::min<uint64_t>(available64, kRingFrames));
    const uint32_t toRead = std::min(frameCount, available);

    for (uint32_t i = 0; i < toRead; ++i)
    {
        const uint32_t index = static_cast<uint32_t>((r + i) & kRingMask) * 2u;
        outputInterleaved[i * 2u] = ring_[index];
        outputInterleaved[i * 2u + 1u] = ring_[index + 1u];
    }

    if (toRead != 0)
        readFrame_.store(r + toRead, std::memory_order_release);
}

bool SteamMicSend::startVirtualMic()
{
    stopVirtualMic();
    clearRing();

    audioContext_ = new (std::nothrow) ma_context{};
    if (audioContext_ == nullptr)
        return false;

    if (ma_context_init(nullptr, 0, nullptr, audioContext_) != MA_SUCCESS)
    {
        delete audioContext_;
        audioContext_ = nullptr;
        return false;
    }

    ma_device_info* playbackInfos = nullptr;
    ma_uint32 playbackCount = 0;
    if (ma_context_get_devices(audioContext_, &playbackInfos, &playbackCount, nullptr, nullptr) != MA_SUCCESS)
    {
        ma_context_uninit(audioContext_);
        delete audioContext_;
        audioContext_ = nullptr;
        return false;
    }

    bool found = false;
    ma_device_id targetId{};
    for (ma_uint32 i = 0; i < playbackCount; ++i)
    {
        if (containsIgnoreCase(playbackInfos[i].name, kTargetDeviceNeedle))
        {
            targetId = playbackInfos[i].id;
            found = true;
            break;
        }
    }

    if (!found)
    {
        ma_context_uninit(audioContext_);
        delete audioContext_;
        audioContext_ = nullptr;
        return false;
    }

    audioDevice_ = new (std::nothrow) ma_device{};
    if (audioDevice_ == nullptr)
    {
        ma_context_uninit(audioContext_);
        delete audioContext_;
        audioContext_ = nullptr;
        return false;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.pDeviceID = &targetId;
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = static_cast<ma_uint32>(std::lround(hostSampleRate_));
    config.performanceProfile = ma_performance_profile_low_latency;
    config.pUserData = this;
    config.dataCallback = [](ma_device* device, void* output, const void*, ma_uint32 frames)
    {
        if (device != nullptr && device->pUserData != nullptr)
            static_cast<SteamMicSend*>(device->pUserData)->renderVirtualMic(static_cast<float*>(output), frames);
    };

    if (ma_device_init(audioContext_, &config, audioDevice_) != MA_SUCCESS)
    {
        delete audioDevice_;
        audioDevice_ = nullptr;
        ma_context_uninit(audioContext_);
        delete audioContext_;
        audioContext_ = nullptr;
        return false;
    }

    if (ma_device_start(audioDevice_) != MA_SUCCESS)
    {
        ma_device_uninit(audioDevice_);
        delete audioDevice_;
        audioDevice_ = nullptr;
        ma_context_uninit(audioContext_);
        delete audioContext_;
        audioContext_ = nullptr;
        return false;
    }

    return true;
}

void SteamMicSend::stopVirtualMic() noexcept
{
    if (audioDevice_ != nullptr)
    {
        ma_device_uninit(audioDevice_);
        delete audioDevice_;
        audioDevice_ = nullptr;
    }

    if (audioContext_ != nullptr)
    {
        ma_context_uninit(audioContext_);
        delete audioContext_;
        audioContext_ = nullptr;
    }

    clearRing();
}

} // namespace Steinberg::Vst
