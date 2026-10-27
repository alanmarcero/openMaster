#include "MasteringStages.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace analog_master::core {
namespace {
constexpr double pi = std::numbers::pi;
constexpr int oversamplingLatency = 112, peakTaps = 128, peakPhases = 16;
double bounded(double v, double lo, double hi, double fallback = 0) noexcept {
    return std::clamp(std::isfinite(v) ? v : fallback, lo, hi);
}
double rate(double sr) noexcept {
    return bounded(sr, 8000, 384000, 48000);
}
double amp(double db) noexcept {
    return std::pow(10., db * .05);
}
double decibels(double x) noexcept {
    return 20 * std::log10(std::max(x, 1e-12));
}
double timeCoefficient(double ms, double sr) noexcept {
    return std::exp(-1. / (.001 * ms * sr));
}
double clean(double x) noexcept {
    return std::abs(x) < 1e-28 ? 0 : x;
}
struct Ramp {
    double value = 0, target = 0, step = 0;
    int remaining = 0, duration = 480;
    void prepare(double sr, double initial) noexcept {
        duration = std::max(1, static_cast<int>(.01 * sr));
        snap(initial);
    }
    void snap(double v) noexcept {
        value = target = v;
        step = 0;
        remaining = 0;
    }
    void set(double v) noexcept {
        if (v != target) {
            target = v;
            remaining = duration;
            step = (target - value) / remaining;
        }
    }
    double next() noexcept {
        if (remaining > 0) {
            value += step;
            if (--remaining == 0)
                value = target;
        }
        return value;
    }
};
struct Stereo {
    double l = 0, r = 0;
};
struct Delay {
    std::vector<Stereo> data;
    size_t position = 0;
    void prepare(int size) {
        data.assign(static_cast<size_t>(std::max(1, size)), {});
        position = 0;
    }
    void reset() noexcept {
        std::fill(data.begin(), data.end(), Stereo{});
        position = 0;
    }
    Stereo process(Stereo x) noexcept {
        auto y = data[position];
        data[position] = x;
        if (++position == data.size())
            position = 0;
        return y;
    }
};
struct Coefficients {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
};
struct Filter {
    Coefficients c;
    std::array<double, 2> z1{}, z2{};
    void reset() noexcept {
        z1 = {};
        z2 = {};
    }
    double process(double x, int channel) noexcept {
        auto i = static_cast<size_t>(channel);
        const double y = c.b0 * x + z1[i];
        z1[i] = clean(c.b1 * x - c.a1 * y + z2[i]);
        z2[i] = clean(c.b2 * x - c.a2 * y);
        return y;
    }
    Stereo process(Stereo x) noexcept { return {process(x.l, 0), process(x.r, 1)}; }
};
Coefficients shelf(double hz, double db, bool high, double sr) noexcept {
    const double a = amp(db * .5), w = 2 * pi * bounded(hz, 10, sr * .45) / sr, c = std::cos(w),
                 s = std::sin(w);
    const double beta = std::sqrt(2 * a) * s;
    double a0, b0, b1, b2, a1, a2;
    if (high) {
        b0 = a * ((a + 1) + (a - 1) * c + beta);
        b1 = -2 * a * ((a - 1) + (a + 1) * c);
        b2 = a * ((a + 1) + (a - 1) * c - beta);
        a0 = (a + 1) - (a - 1) * c + beta;
        a1 = 2 * ((a - 1) - (a + 1) * c);
        a2 = (a + 1) - (a - 1) * c - beta;
    } else {
        b0 = a * ((a + 1) - (a - 1) * c + beta);
        b1 = 2 * a * ((a - 1) - (a + 1) * c);
        b2 = a * ((a + 1) - (a - 1) * c - beta);
        a0 = (a + 1) + (a - 1) * c + beta;
        a1 = -2 * ((a - 1) + (a + 1) * c);
        a2 = (a + 1) + (a - 1) * c - beta;
    }
    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}
Coefficients bandpass(double hz, double q, double sr) noexcept {
    const double w = 2 * pi * bounded(hz, 20, sr * .45) / sr,
                 alpha = std::sin(w) / (2 * bounded(q, .2, 12, 1)), a0 = 1 + alpha;
    return {alpha / a0, 0, -alpha / a0, -2 * std::cos(w) / a0, (1 - alpha) / a0};
}
struct SmoothFilter {
    Filter filter;
    std::array<Ramp, 5> coefficients;
    void prepare(double sr) noexcept {
        const std::array<double, 5> c{1, 0, 0, 0, 0};
        for (size_t i = 0; i < 5; ++i)
            coefficients[i].prepare(sr, c[i]);
    }
    void set(Coefficients c) noexcept {
        const std::array v{c.b0, c.b1, c.b2, c.a1, c.a2};
        for (size_t i = 0; i < 5; ++i)
            coefficients[i].set(v[i]);
    }
    void reset() noexcept {
        filter.reset();
        for (auto &p : coefficients)
            p.snap(p.target);
    }
    Stereo process(Stereo x) noexcept {
        filter.c = {coefficients[0].next(), coefficients[1].next(), coefficients[2].next(),
                    coefficients[3].next(), coefficients[4].next()};
        return filter.process(x);
    }
};
struct Halfband {
    std::array<double, 64> odd{};
    std::array<double, 130> upHistory{};
    std::array<double, 258> downHistory{};
    int upPos = 0, downPos = 0;
    void prepare() noexcept {
        double sum = 0;
        for (int j = 0; j < 64; ++j) {
            const int k = 2 * j + 1;
            const double d = k - 64;
            const double window = .35875 - .48829 * std::cos(2 * pi * k / 128) +
                                  .14128 * std::cos(4 * pi * k / 128) - .01168 * std::cos(6 * pi * k / 128);
            odd[static_cast<size_t>(j)] = std::sin(pi * .5 * d) / (pi * d) * window;
            sum += odd[static_cast<size_t>(j)];
        }
        for (auto &c : odd)
            c *= .5 / sum;
        reset();
    }
    void reset() noexcept {
        upHistory = {};
        downHistory = {};
        upPos = downPos = 0;
    }
    std::array<double, 2> up(double x) noexcept {
        upHistory[static_cast<size_t>(upPos)] = upHistory[static_cast<size_t>(upPos + 65)] = x;
        const double even = upHistory[static_cast<size_t>(upPos + 65 - 32)];
        double oddValue = 0;
        for (int j = 0; j < 64; ++j)
            oddValue += odd[static_cast<size_t>(j)] * upHistory[static_cast<size_t>(upPos + 65 - j)];
        if (++upPos == 65)
            upPos = 0;
        return {even, 2 * oddValue};
    }
    void push(double x) noexcept {
        downHistory[static_cast<size_t>(downPos)] = downHistory[static_cast<size_t>(downPos + 129)] = x;
    }
    double down(double even, double oddValue) noexcept {
        push(even);
        double y = .5 * downHistory[static_cast<size_t>(downPos + 129 - 64)];
        for (int j = 0; j < 64; ++j)
            y += odd[static_cast<size_t>(j)] * downHistory[static_cast<size_t>(downPos + 128 - 2 * j)];
        if (++downPos == 129)
            downPos = 0;
        push(oddValue);
        if (++downPos == 129)
            downPos = 0;
        return y;
    }
};
struct Oversampler {
    std::array<std::array<Halfband, 3>, 2> filters;
    void prepare() noexcept {
        for (auto &channel : filters)
            for (auto &f : channel)
                f.prepare();
    }
    void reset() noexcept {
        for (auto &channel : filters)
            for (auto &f : channel)
                f.reset();
    }
    template <class Shape> double process(double input, int channel, Shape &&shape) noexcept {
        auto &f = filters[static_cast<size_t>(channel)];
        auto x2 = f[0].up(input);
        std::array<double, 4> x4{};
        std::array<double, 8> x8{};
        for (int j = 0; j < 2; ++j) {
            auto p = f[1].up(x2[static_cast<size_t>(j)]);
            x4[static_cast<size_t>(2 * j)] = p[0];
            x4[static_cast<size_t>(2 * j + 1)] = p[1];
        }
        for (int j = 0; j < 4; ++j) {
            auto p = f[2].up(x4[static_cast<size_t>(j)]);
            x8[static_cast<size_t>(2 * j)] = shape(p[0]);
            x8[static_cast<size_t>(2 * j + 1)] = shape(p[1]);
        }
        for (int j = 0; j < 4; ++j)
            x4[static_cast<size_t>(j)] =
                f[2].down(x8[static_cast<size_t>(2 * j)], x8[static_cast<size_t>(2 * j + 1)]);
        for (int j = 0; j < 2; ++j)
            x2[static_cast<size_t>(j)] =
                f[1].down(x4[static_cast<size_t>(2 * j)], x4[static_cast<size_t>(2 * j + 1)]);
        return f[0].down(x2[0], x2[1]);
    }
};
// Bandlimit the protected wet signal before peak sensing. Exact-Nyquist finite
// bursts otherwise have reconstructed endpoint peaks that grow with the oracle
// support, so a short detector cannot qualify their final exported waveform.
struct NyquistGuard {
    static constexpr int taps = 129, latency = 64;
    std::array<double, taps> coefficients{};
    std::array<std::array<double, taps * 2>, 2> history{};
    int position = 0;
    void prepare() noexcept {
        double sum = 0;
        for (int j = 0; j < taps; ++j) {
            const double d = j - latency;
            const double w =
                .42 - .5 * std::cos(2 * pi * j / (taps - 1)) + .08 * std::cos(4 * pi * j / (taps - 1));
            const double h = d == 0 ? .95 : std::sin(.95 * pi * d) / (pi * d);
            coefficients[static_cast<size_t>(j)] = h * w;
            sum += h * w;
        }
        for (auto &c : coefficients)
            c /= sum;
        reset();
    }
    void reset() noexcept {
        history = {};
        position = 0;
    }
    Stereo process(Stereo x) noexcept {
        std::array<double, 2> samples{x.l, x.r}, out{};
        for (size_t channel = 0; channel < 2; ++channel) {
            auto &h = history[channel];
            h[static_cast<size_t>(position)] = h[static_cast<size_t>(position + taps)] = samples[channel];
            double y = coefficients[latency] * h[static_cast<size_t>(position + taps - latency)];
            for (int j = 0; j < latency; ++j)
                y += coefficients[static_cast<size_t>(j)] *
                     (h[static_cast<size_t>(position + taps - j)] +
                      h[static_cast<size_t>(position + taps - (taps - 1 - j))]);
            out[channel] = y;
        }
        if (++position == taps)
            position = 0;
        return {out[0], out[1]};
    }
};
struct PeakBank {
    std::array<std::array<double, peakPhases>, peakTaps> coefficients{};
    std::array<double, peakTaps * 2> left{}, right{};
    int position = 0;
    void prepare() noexcept {
        for (int p = 0; p < peakPhases; ++p) {
            double sum = 0;
            for (int j = 0; j < peakTaps; ++j) {
                const double d = j - peakTaps / 2 + static_cast<double>(p) / peakPhases;
                const double window =
                    std::abs(d) < peakTaps / 2
                        ? .42 + .5 * std::cos(2 * pi * d / peakTaps) + .08 * std::cos(4 * pi * d / peakTaps)
                        : 0;
                const double h = std::abs(d) < 1e-12 ? 1 : std::sin(pi * d) / (pi * d);
                coefficients[static_cast<size_t>(j)][static_cast<size_t>(p)] = h * window;
                sum += h * window;
            }
            for (auto &row : coefficients)
                row[static_cast<size_t>(p)] /= sum;
        }
        reset();
    }
    void reset() noexcept {
        left = {};
        right = {};
        position = 0;
    }
    double process(double l, double r) noexcept {
        left[static_cast<size_t>(position)] = left[static_cast<size_t>(position + peakTaps)] = l;
        right[static_cast<size_t>(position)] = right[static_cast<size_t>(position + peakTaps)] = r;
        double peak = std::max(std::abs(l), std::abs(r));
        // Independent phase accumulators permit SIMD without reassociating FIR
        // sums or enabling unsafe floating-point compiler flags.
        std::array<double, peakPhases> yl{}, yr{};
        for (int j = 0; j < peakTaps; ++j) {
            const auto k = static_cast<size_t>(position + peakTaps - j);
            const auto &c = coefficients[static_cast<size_t>(j)];
            for (size_t p = 0; p < peakPhases; ++p) {
                yl[p] += c[p] * left[k];
                yr[p] += c[p] * right[k];
            }
        }
        for (size_t p = 0; p < peakPhases; ++p)
            peak = std::max({peak, std::abs(yl[p]), std::abs(yr[p])});
        if (++position == peakTaps)
            position = 0;
        return peak;
    }
};
Stereo mix(Stereo dry, Stereo wet, double amount) noexcept {
    return {dry.l + amount * (wet.l - dry.l), dry.r + amount * (wet.r - dry.r)};
}
void output(Stereo x, float &l, float &r) noexcept {
    l = static_cast<float>(x.l);
    r = static_cast<float>(x.r);
}
} // namespace

struct ToneStage::Impl {
    double sr;
    Ramp enabled;
    std::array<SmoothFilter, 4> filters;
    explicit Impl(double sampleRate) : sr(rate(sampleRate)) {
        enabled.prepare(sr, 0);
        for (auto &f : filters)
            f.prepare(sr);
    }
    void set(const ToneSettings &s) noexcept {
        enabled.set(s.enabled ? 1 : 0);
        filters[0].set(shelf(bounded(s.lowHz, 20, sr * .4, 100), bounded(s.lowDb, -12, 12), false, sr));
        filters[1].set(shelf(bounded(s.highHz, 100, sr * .45, 10000), bounded(s.highDb, -12, 12), true, sr));
        filters[2].set(shelf(1000, -bounded(s.tiltDb, -6, 6), false, sr));
        filters[3].set(shelf(1000, bounded(s.tiltDb, -6, 6), true, sr));
    }
    void reset() noexcept {
        enabled.snap(enabled.target);
        for (auto &f : filters)
            f.reset();
    }
    void process(float &l, float &r) noexcept {
        const Stereo dry{l, r};
        Stereo wet = dry;
        for (auto &f : filters)
            wet = f.process(wet);
        output(mix(dry, wet, enabled.next()), l, r);
    }
    float reduction() const noexcept { return 0; }
    int latency() const noexcept { return 0; }
};
struct CompressorStage::Impl {
    double sr, reductionDb = 0;
    Ramp enabled, threshold, ratio, knee, attack, release, makeup, mixAmount, sidechain;
    std::array<double, 2> previousInput{}, previousOutput{};
    explicit Impl(double sampleRate) : sr(rate(sampleRate)) {
        enabled.prepare(sr, 0);
        threshold.prepare(sr, -18);
        ratio.prepare(sr, 2);
        knee.prepare(sr, 6);
        attack.prepare(sr, 30);
        release.prepare(sr, 200);
        makeup.prepare(sr, 0);
        mixAmount.prepare(sr, 1);
        sidechain.prepare(sr, 80);
    }
    void set(const CompressorSettings &s) noexcept {
        enabled.set(s.enabled ? 1 : 0);
        threshold.set(bounded(s.thresholdDb, -60, 0, -18));
        ratio.set(bounded(s.ratio, 1, 20, 2));
        knee.set(bounded(s.kneeDb, 0, 24, 6));
        attack.set(bounded(s.attackMs, .1, 200, 30));
        release.set(bounded(s.releaseMs, 10, 2000, 200));
        makeup.set(bounded(s.makeupDb, -12, 24));
        mixAmount.set(bounded(s.mix, 0, 1, 1));
        sidechain.set(bounded(s.sidechainHz, 0, 1000, 80));
    }
    void reset() noexcept {
        reductionDb = 0;
        previousInput = {};
        previousOutput = {};
        for (auto *p :
             {&enabled, &threshold, &ratio, &knee, &attack, &release, &makeup, &mixAmount, &sidechain})
            p->snap(p->target);
    }
    void process(float &l, float &r) noexcept {
        const Stereo dry{l, r};
        const double hz = sidechain.next(), a = std::exp(-2 * pi * hz / sr);
        std::array<double, 2> x{l, r};
        double peak = 0;
        for (size_t c = 0; c < 2; ++c) {
            const double y = hz > 0 ? a * (previousOutput[c] + x[c] - previousInput[c]) : x[c];
            previousInput[c] = x[c];
            previousOutput[c] = clean(y);
            peak = std::max(peak, std::abs(y));
        }
        const double d = decibels(peak) - threshold.next(), w = knee.next(), slope = 1 - 1 / ratio.next();
        double requested = 0;
        if (w > 0 && d > -w * .5 && d < w * .5)
            requested = slope * (d + w * .5) * (d + w * .5) / (2 * w);
        else if (d >= w * .5)
            requested = slope * d;
        const double at = attack.next(), rel = release.next();
        const double aEnv = timeCoefficient(requested > reductionDb ? at : rel, sr);
        reductionDb = clean(aEnv * reductionDb + (1 - aEnv) * requested);
        const double g = amp(makeup.next() - reductionDb);
        output(mix(dry, {l * g, r * g}, enabled.next() * mixAmount.next()), l, r);
    }
    float reduction() const noexcept { return static_cast<float>(reductionDb * enabled.value); }
    int latency() const noexcept { return 0; }
};
struct DynamicEqStage::Impl {
    struct Band {
        SmoothFilter filter;
        Ramp threshold, range, attack, release;
        double reduction = 0;
    };
    double sr;
    Ramp enabled, amount;
    std::array<Ramp, 4> audition;
    std::array<Band, 3> bands;
    explicit Impl(double sampleRate) : sr(rate(sampleRate)) {
        enabled.prepare(sr, 0);
        amount.prepare(sr, 1);
        for (size_t i = 0; i < 4; ++i)
            audition[i].prepare(sr, i == 0 ? 1 : 0);
        for (auto &b : bands) {
            b.filter.prepare(sr);
            b.threshold.prepare(sr, -24);
            b.range.prepare(sr, 6);
            b.attack.prepare(sr, 10);
            b.release.prepare(sr, 150);
        }
    }
    void set(const DynamicEqSettings &s) noexcept {
        enabled.set(s.enabled ? 1 : 0);
        amount.set(bounded(s.amount, 0, 1, 1));
        const int solo = std::clamp(s.soloBand, -1, 2) + 1;
        for (int i = 0; i < 4; ++i)
            audition[static_cast<size_t>(i)].set(i == solo ? 1 : 0);
        for (size_t i = 0; i < 3; ++i) {
            auto &b = bands[i];
            const auto &p = s.bands[i];
            b.filter.set(bandpass(p.frequency, p.q, sr));
            b.threshold.set(bounded(p.thresholdDb, -60, 0, -24));
            b.range.set(bounded(p.rangeDb, 0, 18, 6));
            b.attack.set(bounded(p.attackMs, .1, 200, 10));
            b.release.set(bounded(p.releaseMs, 10, 2000, 150));
        }
    }
    void reset() noexcept {
        enabled.snap(enabled.target);
        amount.snap(amount.target);
        for (auto &a : audition)
            a.snap(a.target);
        for (auto &b : bands) {
            b.filter.reset();
            b.reduction = 0;
            for (auto *p : {&b.threshold, &b.range, &b.attack, &b.release})
                p->snap(p->target);
        }
    }
    void process(float &l, float &r) noexcept {
        const Stereo dry{l, r};
        Stereo wet = dry, solo{};
        const double strength = amount.next();
        for (size_t i = 0; i < 3; ++i) {
            auto &b = bands[i];
            const auto bp = b.filter.process(wet);
            const double peak = std::max(std::abs(bp.l), std::abs(bp.r));
            const double requested =
                std::clamp(decibels(peak) - b.threshold.next(), 0., b.range.next()) * strength;
            const double at = b.attack.next(), rel = b.release.next();
            const double a = timeCoefficient(requested > b.reduction ? at : rel, sr);
            b.reduction = clean(a * b.reduction + (1 - a) * requested);
            const double g = amp(-b.reduction);
            wet.l += (g - 1) * bp.l;
            wet.r += (g - 1) * bp.r;
            const double aud = audition[i + 1].next();
            solo.l += bp.l * g * aud;
            solo.r += bp.r * g * aud;
        }
        const double normal = audition[0].next();
        wet = {normal * wet.l + solo.l, normal * wet.r + solo.r};
        output(mix(dry, wet, enabled.next()), l, r);
    }
    float reduction() const noexcept {
        double v = 0;
        for (const auto &b : bands)
            v = std::max(v, b.reduction);
        return static_cast<float>(v * enabled.value);
    }
    int latency() const noexcept { return 0; }
};
struct SaturationStage::Impl {
    double sr;
    Ramp enabled, drive, mixAmount, outputGain, character, autoGain;
    Delay dryDelay;
    Oversampler oversampler;
    explicit Impl(double sampleRate) : sr(rate(sampleRate)) {
        enabled.prepare(sr, 0);
        drive.prepare(sr, 6);
        mixAmount.prepare(sr, .5);
        outputGain.prepare(sr, 0);
        character.prepare(sr, 0);
        autoGain.prepare(sr, 1);
        dryDelay.prepare(oversamplingLatency);
        oversampler.prepare();
    }
    void set(const SaturationSettings &s) noexcept {
        enabled.set(s.enabled ? 1 : 0);
        drive.set(bounded(s.driveDb, 0, 30, 6));
        mixAmount.set(bounded(s.mix, 0, 1, .5));
        outputGain.set(bounded(s.outputDb, -24, 12));
        character.set(s.character == 1 ? 1 : 0);
        autoGain.set(s.autoGain ? 1 : 0);
    }
    void reset() noexcept {
        dryDelay.reset();
        oversampler.reset();
        for (auto *p : {&enabled, &drive, &mixAmount, &outputGain, &character, &autoGain})
            p->snap(p->target);
    }
    void process(float &l, float &r) noexcept {
        const auto dry = dryDelay.process({l, r});
        const double d = amp(drive.next()), ch = character.next(),
                     normalization = std::pow(d, -autoGain.next()), out = amp(outputGain.next());
        auto shape = [=](double x) {
            const double driven = x * d;
            return ((1 - ch) * std::tanh(driven) + ch * driven / (1 + std::abs(driven))) * normalization *
                   out;
        };
        const Stereo wet{oversampler.process(l, 0, shape), oversampler.process(r, 1, shape)};
        output(mix(dry, wet, enabled.next() * mixAmount.next()), l, r);
    }
    float reduction() const noexcept { return 0; }
    int latency() const noexcept { return oversamplingLatency; }
};
struct ClipperStage::Impl {
    double sr, reductionDb = 0;
    Ramp enabled, drive, ceiling, mixAmount, shapeAmount;
    Delay dryDelay;
    Oversampler oversampler;
    explicit Impl(double sampleRate) : sr(rate(sampleRate)) {
        enabled.prepare(sr, 0);
        drive.prepare(sr, 0);
        ceiling.prepare(sr, -1);
        mixAmount.prepare(sr, 1);
        shapeAmount.prepare(sr, 1);
        dryDelay.prepare(oversamplingLatency);
        oversampler.prepare();
    }
    void set(const ClipperSettings &s) noexcept {
        enabled.set(s.enabled ? 1 : 0);
        drive.set(bounded(s.driveDb, 0, 30));
        ceiling.set(bounded(s.ceilingDb, -24, 0, -1));
        mixAmount.set(bounded(s.mix, 0, 1, 1));
        shapeAmount.set(s.shape == 1 ? 0 : 1);
    }
    void reset() noexcept {
        dryDelay.reset();
        oversampler.reset();
        reductionDb = 0;
        for (auto *p : {&enabled, &drive, &ceiling, &mixAmount, &shapeAmount})
            p->snap(p->target);
    }
    void process(float &l, float &r) noexcept {
        const auto dry = dryDelay.process({l, r});
        const double d = amp(drive.next()), c = amp(ceiling.next()), soft = shapeAmount.next();
        double reduction = 0;
        auto shape = [&](double x) {
            const double driven = x * d, a = std::abs(driven), hard = std::min(a, c), k = .8 * c, e = 1.2 * c;
            double smooth = a;
            if (a > k) {
                const double u = std::clamp((a - k) / (e - k), 0., 1.);
                smooth = k + (c - k) * (2 * u - u * u);
            }
            const double y = hard + soft * (smooth - hard);
            if (a > 1e-12)
                reduction = std::max(reduction, -decibels(std::max(y, 1e-12) / a));
            return std::copysign(y, driven);
        };
        const Stereo wet{oversampler.process(l, 0, shape), oversampler.process(r, 1, shape)};
        reductionDb = std::max(reduction, reductionDb * std::exp(-1 / (.1 * sr)));
        output(mix(dry, wet, enabled.next() * mixAmount.next()), l, r);
    }
    float reduction() const noexcept {
        return static_cast<float>(reductionDb * enabled.value * mixAmount.value);
    }
    int latency() const noexcept { return oversamplingLatency; }
};
struct LimiterStage::Impl {
    struct Entry {
        double value = 1;
        long long index = 0;
    };
    double sr, releasePeak = 0, sum = 0, reductionDb = 0;
    int lookahead, delay;
    Ramp enabled, drive, ceiling, release;
    Delay dryDelay, wetDelay;
    NyquistGuard guard;
    PeakBank peak;
    std::vector<Entry> deque;
    size_t head = 0, tail = 0;
    long long frame = 0;
    std::vector<double> smooth;
    size_t smoothPosition = 0;
    explicit Impl(double sampleRate)
        : sr(rate(sampleRate)), lookahead(static_cast<int>(std::ceil(.005 * sr))),
          delay(lookahead + peakTaps) {
        enabled.prepare(sr, 0);
        drive.prepare(sr, 0);
        ceiling.prepare(sr, -1);
        release.prepare(sr, 100);
        dryDelay.prepare(delay + NyquistGuard::latency);
        wetDelay.prepare(delay);
        guard.prepare();
        peak.prepare();
        deque.resize(static_cast<size_t>(delay + 2));
        smooth.assign(static_cast<size_t>(lookahead + 1), 10000);
        sum = 10000 * static_cast<double>(smooth.size());
    }
    void set(const LimiterSettings &s) noexcept {
        enabled.set(s.enabled ? 1 : 0);
        drive.set(bounded(s.driveDb, 0, 30));
        ceiling.set(bounded(s.ceilingDb, -24, 0, -1));
        release.set(bounded(s.releaseMs, 50, 2000, 100));
    }
    void reset() noexcept {
        dryDelay.reset();
        wetDelay.reset();
        guard.reset();
        peak.reset();
        head = tail = 0;
        frame = 0;
        smoothPosition = 0;
        releasePeak = 0;
        reductionDb = 0;
        std::fill(smooth.begin(), smooth.end(), 10000);
        sum = 10000 * static_cast<double>(smooth.size());
        for (auto *p : {&enabled, &drive, &ceiling, &release})
            p->snap(p->target);
    }
    void process(float &l, float &r) noexcept {
        const auto dry = dryDelay.process({l, r});
        const double d = amp(drive.next());
        const Stereo driven = guard.process({l * d, r * d});
        const auto delayed = wetDelay.process(driven);
        // The guard is empirically qualified against an independent 64x output
        // reconstruction suite; input true-peak detection alone is not a proof.
        const double c = amp(ceiling.next() - .5), detected = peak.process(driven.l, driven.r);
        const double constraint = detected;
        while (head != tail && deque[head].index < frame - delay)
            head = (head + 1) % deque.size();
        while (head != tail) {
            const size_t back = (tail + deque.size() - 1) % deque.size();
            if (deque[back].value > constraint)
                break;
            tail = back;
        }
        deque[tail] = {constraint, frame++};
        tail = (tail + 1) % deque.size();
        const double maximum = deque[head].value;
        const double a = timeCoefficient(release.next(), sr);
        releasePeak = std::max(maximum, a * releasePeak + (1 - a) * maximum);
        const double inverse = 1 / std::max(1e-4, releasePeak);
        sum += inverse - smooth[smoothPosition];
        smooth[smoothPosition] = inverse;
        if (++smoothPosition == smooth.size())
            smoothPosition = 0;
        const double g = std::clamp(c * sum / static_cast<double>(smooth.size()), 0., 1.);
        reductionDb = -decibels(g);
        output(mix(dry, {delayed.l * g, delayed.r * g}, enabled.next()), l, r);
    }
    float reduction() const noexcept { return static_cast<float>(reductionDb * enabled.value); }
    int latency() const noexcept { return delay + NyquistGuard::latency; }
};

#define OPEN_MASTER_STAGE_IMPL(Name, Settings)                                                               \
    Name::Name() = default;                                                                                  \
    Name::~Name() = default;                                                                                 \
    void Name::prepare(double sr) {                                                                          \
        impl_ = std::make_unique<Impl>(sr);                                                                  \
        impl_->set(settings_);                                                                               \
        impl_->reset();                                                                                      \
    }                                                                                                        \
    void Name::reset() noexcept {                                                                            \
        if (impl_)                                                                                           \
            impl_->reset();                                                                                  \
    }                                                                                                        \
    void Name::setSettings(const Settings &s) noexcept {                                                     \
        if (settings_ == s)                                                                                  \
            return;                                                                                          \
        settings_ = s;                                                                                       \
        if (impl_)                                                                                           \
            impl_->set(s);                                                                                   \
    }                                                                                                        \
    void Name::process(float &l, float &r) noexcept {                                                        \
        if (impl_)                                                                                           \
            impl_->process(l, r);                                                                            \
    }                                                                                                        \
    int Name::latencySamples() const noexcept {                                                              \
        return impl_ ? impl_->latency() : 0;                                                                 \
    }                                                                                                        \
    float Name::gainReductionDb() const noexcept {                                                           \
        return impl_ ? impl_->reduction() : 0;                                                               \
    }
OPEN_MASTER_STAGE_IMPL(ToneStage, ToneSettings)
OPEN_MASTER_STAGE_IMPL(CompressorStage, CompressorSettings)
OPEN_MASTER_STAGE_IMPL(DynamicEqStage, DynamicEqSettings)
OPEN_MASTER_STAGE_IMPL(SaturationStage, SaturationSettings)
OPEN_MASTER_STAGE_IMPL(ClipperStage, ClipperSettings)
OPEN_MASTER_STAGE_IMPL(LimiterStage, LimiterSettings)
#undef OPEN_MASTER_STAGE_IMPL
struct TruePeakDetector::Impl {
    PeakBank bank;
    Impl() { bank.prepare(); }
};
TruePeakDetector::TruePeakDetector() = default;
TruePeakDetector::~TruePeakDetector() = default;
void TruePeakDetector::prepare(double) {
    impl_ = std::make_unique<Impl>();
}
void TruePeakDetector::reset() noexcept {
    if (impl_)
        impl_->bank.reset();
}
float TruePeakDetector::process(float l, float r) noexcept {
    return impl_ ? static_cast<float>(impl_->bank.process(l, r)) : std::max(std::abs(l), std::abs(r));
}
int TruePeakDetector::latencySamples() const noexcept {
    return impl_ ? peakTaps / 2 : 0;
}
} // namespace analog_master::core
