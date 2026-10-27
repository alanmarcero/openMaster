#include "MasterProcessorCore.h"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstdlib>
#include <new>
namespace {
thread_local bool countAllocations = false;
thread_local size_t allocations = 0;
} // namespace
void *operator new(std::size_t n) {
    if (countAllocations)
        ++allocations;
    if (void *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) {
    return ::operator new(n);
}
void operator delete(void *p) noexcept {
    std::free(p);
}
void operator delete[](void *p) noexcept {
    std::free(p);
}
void *operator new(std::size_t n, std::align_val_t alignment) {
    if (countAllocations)
        ++allocations;
    const auto a = static_cast<std::size_t>(alignment);
#if defined(_MSC_VER)
    if (void *p = _aligned_malloc(n ? n : 1, a))
        return p;
#else
    if (void *p = std::aligned_alloc(a, ((n + a - 1) / a) * a))
        return p;
#endif
    throw std::bad_alloc();
}
void operator delete(void *p, std::align_val_t) noexcept {
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
TEST_CASE("Audio processing automation and meter reset do not allocate", "[realtime]") {
    using namespace analog_master::core;
    MasterProcessorCore p;
    p.prepare(48000);
    std::array<float, 257> l{}, r{};
    CompressorSettings comp;
    DynamicEqSettings dyn;
    SaturationSettings sat;
    ClipperSettings clip;
    LimiterSettings limit;
    ToneSettings tone;
    comp.enabled = dyn.enabled = sat.enabled = clip.enabled = limit.enabled = tone.enabled = true;
    allocations = 0;
    countAllocations = true;
    for (int block = 0; block < 240; ++block) {
        comp.thresholdDb = -24.f + static_cast<float>(block % 12);
        dyn.bands[1].frequency = 1000.f + static_cast<float>(block) * 20.f;
        sat.driveDb = static_cast<float>(block % 24);
        tone.tiltDb = static_cast<float>(block % 6);
        clip.shape = block % 2;
        limit.ceilingDb = -1.f - static_cast<float>(block % 3);
        p.setCompressor(comp);
        p.setDynamicEq(dyn);
        p.setSaturation(sat);
        p.setTone(tone);
        p.setClipper(clip);
        p.setLimiter(limit);
        p.setMasterBypassed(block % 31 == 0);
        p.setMatchBypass(block % 2 == 0);
        if (block % 29 == 0)
            p.requestMeterReset();
        l.fill(.3f);
        r.fill(-.2f);
        p.processBlock(l.data(), r.data(), static_cast<int>(l.size()));
        (void)p.takeMeterLevels();
    }
    countAllocations = false;
    REQUIRE(allocations == 0);
}
