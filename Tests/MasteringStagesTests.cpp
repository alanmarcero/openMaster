#include "MasteringStages.h"
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>
#include <numbers>
#include <random>
#include <vector>

using namespace analog_master::core;
namespace {
constexpr double pi = std::numbers::pi;
double db(double v) {
    return 20 * std::log10(std::max(1e-12, v));
}
double gain(double v) {
    return std::pow(10., v / 20);
}
// Independent offline oracle: 64x, 256-source-sample, Hann-windowed sinc.
// Production uses a different window and support. Zero padding includes tails.
double oraclePeak(const std::vector<float> &x, int phases = 64, int taps = 256) {
    std::vector<double> kernel(static_cast<size_t>(phases * taps));
    for (int p = 0; p < phases; ++p) {
        double sum = 0;
        for (int j = 0; j < taps; ++j) {
            double d = j - taps / 2 + static_cast<double>(p) / phases;
            double sinc = std::abs(d) < 1e-12 ? 1 : std::sin(pi * d) / (pi * d);
            double window = std::abs(d) < taps / 2 ? .5 + .5 * std::cos(2 * pi * d / taps) : 0;
            kernel[static_cast<size_t>(p * taps + j)] = sinc * window;
            sum += sinc * window;
        }
        for (int j = 0; j < taps; ++j)
            kernel[static_cast<size_t>(p * taps + j)] /= sum;
    }
    double peak = 0;
    for (int n = -taps / 2; n < static_cast<int>(x.size()) + taps / 2; ++n)
        for (int p = 0; p < phases; ++p) {
            double y = 0;
            for (int j = std::max(0, taps / 2 - n);
                 j < std::min(taps, static_cast<int>(x.size()) + taps / 2 - n); ++j)
                y += x[static_cast<size_t>(n + j - taps / 2)] * kernel[static_cast<size_t>(p * taps + j)];
            peak = std::max(peak, std::abs(y));
        }
    return peak;
}
template <class Stage> void checkBypass() {
    Stage stage;
    stage.prepare(48000);
    REQUIRE(stage.latencySamples() > 0);
    std::vector<float> input(2000);
    std::mt19937 rng(42);
    for (auto &v : input)
        v = static_cast<float>(rng() % 2000) / 1000 - 1;
    for (size_t n = 0; n < input.size(); ++n) {
        float l = input[n], r = -l;
        stage.process(l, r);
        const float expected = n >= static_cast<size_t>(stage.latencySamples())
                                   ? input[n - static_cast<size_t>(stage.latencySamples())]
                                   : 0;
        REQUIRE(l == expected);
        REQUIRE(r == -expected);
    }
    stage.reset();
    float l = 0, r = 0;
    stage.process(l, r);
    REQUIRE(l == 0);
    REQUIRE(r == 0);
}
template <class Stage, class Settings> double toneGain(Settings settings, double hz) {
    Stage stage;
    stage.prepare(48000);
    stage.setSettings(settings);
    double input = 0, output = 0;
    for (int i = 0; i < 24000; ++i) {
        float l = static_cast<float>(.15 * std::sin(2 * pi * hz * i / 48000)), r = l;
        if (i > 12000)
            input += l * l;
        stage.process(l, r);
        if (i > 12000)
            output += l * l;
    }
    return db(std::sqrt(output / input));
}
} // namespace

TEST_CASE("Prepared nonlinear stages bypass with exact fixed delay", "[mastering]") {
    checkBypass<SaturationStage>();
    checkBypass<ClipperStage>();
    checkBypass<LimiterStage>();
}
TEST_CASE("Compressor matches analytic DC curve and preserves stereo ratios", "[mastering]") {
    for (float ratio : {1.f, 2.f, 4.f, 10.f}) {
        CompressorStage stage;
        stage.prepare(48000);
        CompressorSettings s;
        s.enabled = true;
        s.thresholdDb = -18;
        s.ratio = ratio;
        s.kneeDb = 0;
        s.attackMs = .1f;
        s.sidechainHz = 0;
        stage.setSettings(s);
        stage.reset();
        float l = 0, r = 0;
        for (int n = 0; n < 5000; ++n) {
            l = .5f;
            r = -.125f;
            stage.process(l, r);
        }
        const double reduction = (db(.5) + 18) * (1 - 1 / ratio);
        REQUIRE(std::abs(db(l / .5) + reduction) < .01);
        REQUIRE(std::abs(r / l + .25) < 1e-6);
        REQUIRE(std::abs(stage.gainReductionDb() - reduction) < .01);
    }
}
TEST_CASE("Tone shelves have their specified gain at the spectral extremes", "[mastering]") {
    ToneSettings s;
    s.enabled = true;
    s.lowDb = 6;
    s.highDb = -6;
    s.lowHz = 200;
    s.highHz = 6000;
    REQUIRE(std::abs(toneGain<ToneStage>(s, 20) - 6) < .15);
    REQUIRE(std::abs(toneGain<ToneStage>(s, 21000) + 6) < .15);
    s.lowDb = s.highDb = 0;
    s.tiltDb = 4;
    REQUIRE(toneGain<ToneStage>(s, 40) < -3.8);
    REQUIRE(toneGain<ToneStage>(s, 18000) > 3.8);
}
TEST_CASE("Dynamic EQ attenuates only the active band and bounds its range", "[mastering]") {
    DynamicEqSettings s;
    s.enabled = true;
    for (auto &b : s.bands)
        b.rangeDb = 0;
    s.bands[1] = {2000, 3, -40, 6, 1, 50};
    const auto centre = toneGain<DynamicEqStage>(s, 2000);
    REQUIRE(centre < -5.8);
    REQUIRE(centre > -6.1);
    REQUIRE(std::abs(toneGain<DynamicEqStage>(s, 100)) < .1);
    s.soloBand = 1;
    REQUIRE(toneGain<DynamicEqStage>(s, 100) < -30);
}
TEST_CASE("Saturation characters differ and automatic gain preserves small signals", "[mastering]") {
    SaturationSettings s;
    s.enabled = true;
    s.mix = 1;
    s.driveDb = 12;
    s.autoGain = true;
    const double a = toneGain<SaturationStage>(s, 1000);
    s.character = 1;
    const double b = toneGain<SaturationStage>(s, 1000);
    REQUIRE(a < -.1);
    REQUIRE(b < -.1);
    REQUIRE(std::abs(a - b) > .1);
    s.driveDb = 0;
    s.mix = 0;
    REQUIRE(std::abs(toneGain<SaturationStage>(s, 1000)) < .001);
}
TEST_CASE("True peak reconstruction detects the Fs/4 phase peak", "[mastering]") {
    TruePeakDetector meter;
    meter.prepare(48000);
    double peak = 0;
    for (int n = 0; n < 2048; ++n) {
        float v = static_cast<float>(std::sin(2 * pi * .25 * n + pi / 4));
        const float p = meter.process(v, -v);
        if (n > 300)
            peak = std::max(peak, static_cast<double>(p));
    }
    REQUIRE(std::abs(db(peak)) < .05);
}
TEST_CASE("Limiter exported waveform meets ceiling with independent reconstruction",
          "[mastering][truepeak]") {
    for (double sr : {44100., 48000., 96000., 192000.}) {
        for (int fixture = 0; fixture < 10; ++fixture) {
            LimiterStage stage;
            stage.prepare(sr);
            LimiterSettings s;
            s.enabled = true;
            s.ceilingDb = -1;
            s.releaseMs = 50;
            stage.setSettings(s);
            float l = 0, r = 0;
            for (int n = 0; n < static_cast<int>(sr * .02); ++n)
                stage.process(l, r);
            double maximumSample = 0, stereoError = 0;
            std::mt19937 rng(43);
            std::vector<float> out(4096 + static_cast<size_t>(stage.latencySamples()) + 256);
            for (size_t n = 0; n < out.size(); ++n) {
                double x = 0;
                if (n < 4096) {
                    if (fixture == 0)
                        x = (n == 100 || n == 340 || n == 581) ? 4 : 0;
                    if (fixture == 1)
                        x = (n % 4 < 2) ? 2 : -2;
                    if (fixture == 2)
                        x = 2 * std::sin(2 * pi * .49 * static_cast<double>(n) + .37);
                    if (fixture == 3)
                        x = static_cast<double>(static_cast<int>(rng() % 20001) - 10000) / 3000;
                    if (fixture == 4)
                        x = n < 2000 ? .1 * std::sin(2 * pi * .037 * n) : 3 * std::sin(2 * pi * .037 * n);
                    if (fixture == 5)
                        x = 1.4 * std::sin(2 * pi * .07 * n) + 1.3 * std::sin(2 * pi * .183 * n + .3) +
                            .9 * std::sin(2 * pi * .421 * n);
                    if (fixture == 6)
                        x = 2;
                    if (fixture == 7)
                        x = n % 2 ? 2 : -2;
                    if (fixture == 8)
                        x = (n == 100 || n == 339 || n == 580) ? -4 : 0;
                    if (fixture == 9) {
                        const double d = static_cast<double>(n) - 200.3;
                        x = std::abs(d) < 1e-12 ? 4 : 4 * std::sin(pi * .9 * d) / (pi * .9 * d);
                    }
                }
                l = static_cast<float>(x);
                r = -l * .7f;
                stage.process(l, r);
                out[n] = l;
                maximumSample = std::max(maximumSample, static_cast<double>(std::abs(l)));
                stereoError = std::max(stereoError, static_cast<double>(std::abs(r + l * .7f)));
            }
            REQUIRE(maximumSample <= gain(-1) + 1e-6);
            REQUIRE(stereoError < 1e-6);
            const double measured = db(oraclePeak(out));
            const double refined = db(oraclePeak(out, 128, 512));
            const double excess = refined + 1;
            INFO("oracle convergence dB " << refined - measured);
            REQUIRE(std::abs(refined - measured) < .01);
            if (sr == 48000 && (fixture == 2 || fixture == 3 || fixture == 7))
                REQUIRE(std::abs(db(oraclePeak(out, 128, 1024)) - refined) < .01);
            INFO("sample rate " << sr << " fixture " << fixture << " excess dB " << excess);
            REQUIRE(excess <= .10);
        }
    }
}
TEST_CASE("Clipper oversampling suppresses the aliased third harmonic", "[mastering]") {
    ClipperStage stage;
    stage.prepare(48000);
    ClipperSettings s;
    s.enabled = true;
    s.driveDb = 12;
    s.ceilingDb = 0;
    s.shape = 1;
    stage.setSettings(s);
    constexpr int count = 8192;
    constexpr int fundamental = 1707;
    constexpr int alias = count - 3 * fundamental;
    std::complex<double> os{}, raw{}, fundamentalAmplitude{};
    for (int n = 0; n < 2 * count; ++n) {
        float x = static_cast<float>(std::sin(2 * pi * fundamental * n / count));
        float l = x, r = x;
        stage.process(l, r);
        if (n >= count) {
            auto phase = std::polar(1., -2 * pi * alias * n / count);
            os += static_cast<double>(l) * phase;
            fundamentalAmplitude +=
                static_cast<double>(l) * std::polar(1., -2 * pi * fundamental * n / count);
            raw += std::clamp(static_cast<double>(x) * gain(12), -1., 1.) * phase;
        }
    }
    INFO("alias improvement dB " << db(std::abs(raw) / std::abs(os)));
    REQUIRE(db(std::abs(raw) / std::abs(os)) > 20);
    REQUIRE(std::abs(fundamentalAmplitude) > count * .55);
}
TEST_CASE("Nonlinear stage state is independent of caller block partitions", "[mastering]") {
    LimiterStage a, b;
    a.prepare(48000);
    b.prepare(48000);
    LimiterSettings s;
    s.enabled = true;
    s.driveDb = 10;
    a.setSettings(s);
    b.setSettings(s);
    std::vector<float> expected(4096), actual(4096);
    for (int i = 0; i < 4096; ++i) {
        float l = static_cast<float>(std::sin(i * .37)), r = l;
        a.process(l, r);
        expected[static_cast<size_t>(i)] = l;
    }
    constexpr std::array sizes{1, 7, 13, 64, 255, 511, 1024};
    size_t n = 0, block = 0;
    while (n < actual.size()) {
        const auto end = std::min(actual.size(), n + static_cast<size_t>(sizes[block++ % sizes.size()]));
        for (; n < end; ++n) {
            float l = static_cast<float>(std::sin(n * .37)), r = l;
            b.process(l, r);
            actual[n] = l;
        }
    }
    REQUIRE(actual == expected);
}

TEST_CASE("Clipper shape zero is soft and shape one is hard", "[mastering]") {
    std::array<float, 2> values{};
    for (int shape = 0; shape < 2; ++shape) {
        ClipperSettings s;
        s.enabled = true;
        s.ceilingDb = 0;
        s.shape = shape;
        ClipperStage stage;
        stage.setSettings(s);
        stage.prepare(48000);
        float l = 0, r = 0;
        for (int n = 0; n < 2000; ++n) {
            l = .9f;
            r = l;
            stage.process(l, r);
        }
        values[static_cast<size_t>(shape)] = l;
    }
    REQUIRE(std::abs(values[0] - .8875) < 1e-5);
    REQUIRE(std::abs(values[1] - .9) < 1e-5);
}
TEST_CASE("Limiter ceiling automation settles within its parameter ramp", "[mastering]") {
    LimiterSettings s;
    s.enabled = true;
    s.ceilingDb = -1;
    LimiterStage stage;
    stage.setSettings(s);
    stage.prepare(48000);
    float l = 0, r = 0;
    for (int n = 0; n < 4000; ++n) {
        l = 2;
        r = 1;
        stage.process(l, r);
    }
    s.ceilingDb = -12;
    stage.setSettings(s);
    for (int n = 0; n < 481; ++n) {
        l = 2;
        r = 1;
        stage.process(l, r);
    }
    REQUIRE(db(l) <= -12.0 + .1);
}
TEST_CASE("Clipper linear region retains latency and the audio passband", "[mastering]") {
    ClipperSettings s;
    s.enabled = true;
    s.ceilingDb = 0;
    s.shape = 1;
    for (double hz : {20., 1000., 20000., 21600.}) {
        INFO("frequency " << hz);
        REQUIRE(std::abs(toneGain<ClipperStage>(s, hz)) < .05);
    }
    ClipperStage stage;
    stage.setSettings(s);
    stage.prepare(48000);
    std::vector<float> impulse(512);
    for (int n = 0; n < 512; ++n) {
        float l = n == 0 ? .01f : 0.f, r = l;
        stage.process(l, r);
        impulse[static_cast<size_t>(n)] = l;
    }
    REQUIRE(std::distance(impulse.begin(), std::max_element(impulse.begin(), impulse.end())) ==
            stage.latencySamples());
}
TEST_CASE("Dynamic EQ reset clears gain and exiting solo restores the band mix", "[mastering]") {
    DynamicEqSettings s;
    s.enabled = true;
    s.soloBand = 1;
    s.bands[1].thresholdDb = -50;
    DynamicEqStage stage;
    stage.setSettings(s);
    stage.prepare(48000);
    for (int n = 0; n < 5000; ++n) {
        float l = static_cast<float>(.5 * std::sin(n * 2 * pi * 2500 / 48000)), r = -l;
        stage.process(l, r);
    }
    REQUIRE(stage.gainReductionDb() > 1);
    stage.reset();
    REQUIRE(stage.gainReductionDb() == 0);
    s.soloBand = -1;
    for (auto &b : s.bands)
        b.rangeDb = 0;
    stage.setSettings(s);
    for (int n = 0; n < 24000; ++n) {
        float l = .2f, r = -.1f;
        stage.process(l, r);
        if (n > 23000) {
            REQUIRE(std::abs(l - .2f) < 1e-6);
            REQUIRE(std::abs(r + .1f) < 1e-6);
        }
    }
}

TEST_CASE("True peak reconstruction follows analytical tones across phase and frequency", "[mastering]") {
    for (double f : {.01, .07, .125, .25, .375, .45, .47, .49})
        for (int phase = 0; phase < 64; ++phase) {
            TruePeakDetector meter;
            meter.prepare(48000);
            double peak = 0;
            for (int n = 0; n < 1600; ++n) {
                float x = static_cast<float>(std::sin(2 * pi * f * n + 2 * pi * phase / 64));
                const double p = meter.process(x, -x);
                if (n > 400)
                    peak = std::max(peak, p);
            }
            INFO("f/Fs " << f << " phase " << phase);
            REQUIRE(std::abs(db(peak)) < .05);
        }
    TruePeakDetector meter;
    meter.prepare(48000);
    double peak = 0;
    for (int n = 0; n < 400; ++n) {
        float x = (n == 50 || n == 51) ? 1.f : 0.f;
        peak = std::max(peak, static_cast<double>(meter.process(x, 0)));
    }
    REQUIRE(std::abs(peak - 4 / pi) < .002);
}
TEST_CASE("Limiter guard retains the audible passband and exact wet delay", "[mastering]") {
    LimiterSettings s;
    s.enabled = true;
    for (double hz : {20., 1000., 20000., 21600.}) {
        INFO("frequency " << hz);
        REQUIRE(std::abs(toneGain<LimiterStage>(s, hz)) < .05);
    }
    LimiterStage stage;
    stage.setSettings(s);
    stage.prepare(48000);
    std::vector<float> impulse(1000);
    for (int n = 0; n < 1000; ++n) {
        float l = n == 0 ? .01f : 0.f, r = l;
        stage.process(l, r);
        impulse[static_cast<size_t>(n)] = l;
    }
    REQUIRE(std::distance(impulse.begin(), std::max_element(impulse.begin(), impulse.end())) ==
            stage.latencySamples());
}
TEST_CASE("Compressor soft knee agrees with independently calculated levels", "[mastering]") {
    constexpr std::array inputDb{-24., -21., -18., -15., -12.};
    constexpr std::array expectedReduction{0., 0., .375, 1.5, 3.};
    for (size_t i = 0; i < inputDb.size(); ++i) {
        CompressorSettings s;
        s.enabled = true;
        s.thresholdDb = -18;
        s.ratio = 2;
        s.kneeDb = 6;
        s.sidechainHz = 0;
        s.attackMs = .1f;
        CompressorStage stage;
        stage.setSettings(s);
        stage.prepare(48000);
        float l = 0, r = 0;
        for (int n = 0; n < 4000; ++n) {
            l = static_cast<float>(gain(inputDb[i]));
            r = -l;
            stage.process(l, r);
        }
        REQUIRE(std::abs(inputDb[i] - db(l) - expectedReduction[i]) < .001);
    }
}
TEST_CASE("Saturation autogain normalizes small signal slope", "[mastering]") {
    for (int character = 0; character < 2; ++character) {
        SaturationSettings s;
        s.enabled = true;
        s.character = character;
        s.driveDb = 18;
        s.mix = 1;
        s.autoGain = true;
        SaturationStage stage;
        stage.setSettings(s);
        stage.prepare(48000);
        double in = 0, out = 0;
        for (int n = 0; n < 8000; ++n) {
            float l = static_cast<float>(.0001 * std::sin(n * 2 * pi * 1000 / 48000)), r = l;
            if (n > 4000)
                in += l * l;
            stage.process(l, r);
            if (n > 4000)
                out += l * l;
        }
        REQUIRE(std::abs(db(std::sqrt(out / in))) < .02);
    }
}

namespace {
// Coherent periodic sine clipping has only odd harmonics before sampling.
// Parseval minus those intended harmonics measures the remaining alias power.
double aliasResidualPower(const std::vector<double> &x, int fundamental) {
    double power = 0, dc = 0;
    for (double v : x) {
        power += v * v;
        dc += v;
    }
    const double count = static_cast<double>(x.size());
    power = power / count - dc * dc / (count * count);
    for (int h = 1; h * fundamental < static_cast<int>(x.size() / 2); h += 2) {
        std::complex<double> amplitude{};
        for (size_t n = 0; n < x.size(); ++n)
            amplitude += x[n] * std::polar(1., -2 * pi * h * fundamental * static_cast<double>(n) / count);
        power -= 2 * std::norm(amplitude) / (count * count);
    }
    return std::max(power, 1e-20);
}
} // namespace
TEST_CASE("Eightfold clipping reduces total alias residual across tone and drive fixtures", "[mastering]") {
    constexpr int count = 8192;
    for (int fundamental : {170, 853, 1707, 3071})
        for (float drive : {6.f, 12.f, 24.f}) {
            ClipperSettings s;
            s.enabled = true;
            s.driveDb = drive;
            s.ceilingDb = 0;
            s.shape = 1;
            ClipperStage stage;
            stage.setSettings(s);
            stage.prepare(48000);
            std::vector<double> raw(count), oversampled(count);
            for (int n = 0; n < 2 * count; ++n) {
                float l = static_cast<float>(std::sin(2 * pi * fundamental * n / count)), r = l;
                const double x = l;
                stage.process(l, r);
                if (n >= count) {
                    raw[static_cast<size_t>(n - count)] = std::clamp(x * gain(drive), -1., 1.);
                    oversampled[static_cast<size_t>(n - count)] = l;
                }
            }
            const double improvement = 10 * std::log10(aliasResidualPower(raw, fundamental) /
                                                       aliasResidualPower(oversampled, fundamental));
            INFO("frequency " << 48000. * fundamental / count << " drive " << drive << " improvement dB "
                              << improvement);
            REQUIRE(improvement > 20);
        }
}
