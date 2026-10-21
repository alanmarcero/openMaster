# Mastering DSP research and integration

The new processing was researched before implementation. The companion
[stage notes](MASTERING_STAGES.md) describe the compressor, equalizers, nonlinear
stages, limiter and their measurements. Source code remains pure C++20 in the core;
JUCE handles host parameters and the editor.

## Loudness

[ITU-R BS.1770-5](https://www.itu.int/rec/R-REC-BS.1770) Annex 1 defines the two K-weighting
filters, channel-power sum, −0.691 calibration, 400 ms integrated blocks at 100 ms
hops, −70 LUFS absolute gate, and relative gate 10 LU below the absolute-gated mean.
[EBU Tech 3341](https://tech.ebu.ch/docs/tech/tech3341.pdf) defines the 3-second
short-term window and meter timing/calibration requirements.

`LoudnessMeter` sums separately squared channel outputs; antiphase stereo therefore
has the same loudness as in-phase stereo. True mono counts only one channel. The
arbitrary-rate K-filter fit follows
[libebur128's filter initialization](https://github.com/jiixyj/libebur128/blob/67b33abe1558160ed76ada1322329b0e9e058b02/ebur128/ebur128.c)
(pinned source inspected during research). The RLB numerator is deliberately not
renormalized. Attribution/license is in `third_party/notices/libebur128.txt`;
there is no runtime dependency on that library.

Energy is retained in 300 ten-millisecond buckets. Readings begin after complete
windows: 400 ms for integrated and 3 s for short-term. Common audio sample rates
have exact bucket lengths; unusual rates round each bucket to the nearest sample.
Live short-term updates every 10 ms. Integrated blocks are added every 100 ms.
Silence reads negative infinity, rendered as dashes in the editor.

Integrated history uses fixed 0.001 LU bins from −70 to +70 LUFS, each storing its
count and actual energy sum. Absolute gate classification is exact; the bin crossing
the relative gate is classified by its mean. A prefix-sum tree gives logarithmic updates and queries; epoch tags clear history
without a large audio-thread memory sweep. Only the integrated output meter allocates
the approximately 6.4 MiB history. This bounds memory and callback work,
but can change final integrated loudness near a heavily populated gate boundary.
There is no universal 0.001 LU final-error guarantee. Extreme overload above the range
shares an overflow bin. This is an engineering meter, not a certified implementation.

Tests calculate expected frequency response from the published 48 kHz coefficients,
independently of production coefficient generation. They cover 31.5–10000 Hz,
44.1/48/96/192 kHz calibration, +3.0103 LU stereo versus mono, antiphase, warmup,
silence, reset, and absolute/relative gates. Official EBU test WAVs are not bundled.

## Signal order, latency, and comparison

Low cut → low-band stereo → high cut → tone → compressor → dynamic EQ → saturation
→ master trim → clipper → limiter. Master trim precedes final peak control.

All delayed stages maintain fixed latency when disabled. An equal-length dry delay
feeds global/host bypass and the comparison meter. Power changes use a 10 ms linear
crossfade. The processed branch runs continuously while bypassed. The host receives
the sum of the prepared stage delays, so switching a stage never changes host PDC.

Two ungated short-term meters compare aligned dry and processed branches before
comparison gain. Once both are ready and above −60 LUFS, match attenuates the louder
branch by at most 12 dB, with a 500 ms time constant. It never boosts the limited
output. Silence freezes the last match. Turning match off smoothly returns both
branches to unity. Output meters observe the final emitted audio.

Audio state is owned by the processing thread; state loading only updates parameter
atomics. Meter fields are independent atomics. A UI reset requests an audio-thread
measurement reset, clearing true-peak reconstruction history for a fresh measurement
window while preserving audio filter/delay state. Histories and coefficient
storage are bounded. Preparation allocates delay buffers; callbacks do not.

## Independent loudness comparison

A local comparison against the pinned libebur128 **exact-history** mode used eight
30-second stepped multitone programmes (mono/stereo at 44.1/48/96/192 kHz). The
maximum short-term difference was 0.000004 LU. Integrated differences were normally
below 0.000003 LU, with a maximum 0.080003 LU when a population of almost-identical
blocks straddled the relative gate. Finer bins improved one such case but cannot
remove the general boundary ambiguity. The regression suite retains the independently
computed 44.1 kHz boundary result; the maximum across all rates is disclosed here.
