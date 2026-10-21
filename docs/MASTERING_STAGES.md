# Mastering DSP stages

`MasteringStages.h/.cpp` implement six independent stereo stages and a reconstructed
peak detector in C++20. They have no JUCE dependency. All settings and enable changes
use 10 ms, sample-counted ramps, so callback size does not change automation timing.
`setSettings` ignores an identical snapshot. `prepare` allocates; `process`,
`setSettings`, and `reset` neither allocate nor lock. Settings belong to the audio
thread. `reset` clears histories and envelopes and snaps ramps to their targets.

All new stages default off. Call `prepare(sampleRate)` before processing. Settings
may also be supplied before preparation. Mono callers pass the same value as left
and right and use one output; stereo dynamics use maximum-channel linking, which
also detects opposite-polarity signals. Preparation clamps invalid sample rates to
a finite 8–384 kHz range; release qualification covers 44.1, 48, 96, and 192 kHz.

## Tone

Low and high shelves use the RBJ shelf equations with slope S=1. Tilt combines
opposite shelves at 1 kHz, giving the selected positive tilt as a low-frequency cut
and high-frequency boost. Coefficients interpolate over the control ramp. This is
ordinary digital EQ, with no hardware-model claim. See the primary
[W3C Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook/).

## Compressor

The compressor is a feedforward, stereo-linked peak design. Its sidechain has an
optional first-order high-pass. For level above threshold `d`, ratio `R`, and knee
width `W`, the requested reduction is zero below `-W/2`,
`(1-1/R)*(d+W/2)^2/(2W)` inside the knee, and `(1-1/R)*d` above it. A zero-width knee
uses the hard-knee limit. Attack/release smooth the reduction in dB; their time
constants mean 63.2% settling. Makeup and parallel mix follow gain application.
The equations follow [Giannoulis, Massberg and Reiss, 2012](https://www.eecs.qmul.ac.uk/~josh/documents/2012/GiannoulisMassbergReiss-dynamicrangecompression-JAES2012.pdf).

## Dynamic EQ

Three cascaded sections use an RBJ constant-peak bandpass to isolate a band. Each
section adds `(gain-1)*band` to its input. At the band's center the attenuation is
exactly the smoothed gain; away from the band it approaches unity. This construction
keeps a single section between unity and its downward range without coefficient
modulation by the detector. Detector reduction is the positive threshold excess,
capped at the band's range, multiplied by Amount, then smoothed with that band's
attack/release. Overlapping sections may combine their cuts. Stereo uses one shared
gain per band. Audition ramps between the normal result and the selected processed
band. The reduction meter reports the largest band reduction, not their sum.

## Saturation and clipping

Both stages use the same fixed 8x audio oversampler: three cascaded 2x, 129-tap
halfband FIRs for interpolation and decimation. Coefficients are reproducibly
computed during prepare from an ideal halfband sinc and the four-term
Blackman–Harris window (`.35875, -.48829, .14128, -.01168`). Odd taps are normalized
to sum to 0.5, with a center tap of 0.5 and the remaining even taps exactly zero.
Sparse polyphase evaluation avoids multiplying the inserted zero samples.

The measured round-trip delay is 112 source samples. The dry path uses an exact
112-sample delay, including at Mix=0 and when disabled. The wet path is flat within
0.05 dB through 0.45 Fs in the 48 kHz response fixtures. The filter rolls off near
Nyquist. The design follows the multirate requirements discussed by
[Kahles, Esqueda and Välimäki, 2019](https://acris.aalto.fi/ws/portalfiles/portal/34665796/ELEC_Kahles2019_Oversampling_JAES.pdf);
it is an original implementation, not copied reference code.

Saturation character 0 is `tanh(x)` and character 1 is `x/(1+abs(x))`, with driven
input and a smooth transition between them. These are symmetric mathematical
waveshapers, not circuit emulations. Auto Gain divides by drive to preserve the
small-signal slope. It does not measure perceived loudness or promise equal LUFS.
Output gain applies to the wet signal.

Clipper shape 0 is Soft; shape 1 is Hard. Soft uses an identity region through
`0.8*ceiling`, a quadratic transition ending at `1.2*ceiling`, and a flat plateau
at the ceiling. The transition has continuous first derivative. Hard is symmetric
clamping. Drive is intentional gain and is not normalized away. The clipper ceiling
bounds the oversampled nonlinear samples; decimation can ring above it. The clipper
alone is not a true-peak limiter.

## Reconstructed peaks and limiter

The peak detector uses a 16-phase, 128-source-tap, normalized Blackman-windowed sinc
bank, accumulated in double precision. Phase sums use a SIMD-friendly layout without
fast-math reassociation. It returns the largest stereo reconstructed value for a
frame and includes the current raw sample peak. Reconstruction is centered 64 source
samples behind the input. The meter does not delay audio. This follows the
reconstruction principle in [ITU-R BS.1770-5, Annex 2](https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.1770-5-202311-I!!PDF-E.pdf),
but is not represented as certified BS.1770 conformance.

The limiter protects its wet path with a 129-tap, normalized Blackman-windowed sinc
low-pass, cutoff 0.475 Fs and delay 64 samples. This bandwidth guard is flat within
0.05 dB through 0.45 Fs in the tested response, including 20 kHz at 48 kHz. Its rolloff
near Nyquist is intentional. A finite exact-Nyquist burst can have reconstructed
endpoint peaks that increase with interpolation support; merely increasing a short
peak detector's phase count did not solve this. Qualification caught that failure
before the bandwidth guard was introduced.

Let `A=ceil(.005*Fs)` and `D=A+128`. The filtered, driven stereo peak feeds a moving
maximum with support `D+1`. A release follower stays at or above that maximum.
The limiter averages the reciprocal follower over `A+1` samples, multiplies by the
current ramped ceiling, caps gain at unity, and applies the result to audio delayed
by D. Every peak window contributing to this gain includes the delayed sample, so
the construction enforces the instantaneous sample ceiling. This is an inverse-peak
variant of the moving extrema plus finite smoothing construction described by
[Geraint Luff, 2022](https://signalsmith-audio.co.uk/writing/2022/limiter/).
Keeping the follower independent of ceiling avoids additional ceiling-automation lag.
Release is at least 50 ms. The deque is amortized O(1), with a finite O(D) worst case.

That sample constraint is **not a proof of an arbitrary DAC's reconstructed ceiling**.
The internal ceiling reserves 0.5 dB, and the exported waveform is separately
qualified with independent 64x/256-tap and 128x/512-tap Hann-sinc reconstruction.
Difficult fixtures also use 128x/1024 taps. Qualification requires agreement within
0.01 dB and final reconstructed output no higher than the requested ceiling +0.10 dB.
The protected, fully enabled limiter passed the committed fixture suite. No claim
is made for every possible signal or reconstruction filter.

Bypass intentionally returns delayed raw audio. During enable/bypass crossfades,
the dry contribution can exceed the limiter ceiling; the ceiling qualification
applies to the settled enabled path. Lowering the ceiling settles over the stated
10 ms control ramp, not instantaneously at receipt of a parameter change. Output
boosts applied after the limiter would also invalidate its protection.

## Fixed latency

Tone, compressor, and dynamic EQ report zero samples. Saturation and clipping each
report 112, independent of their enabled state. Limiter latency includes its
bandwidth guard and detector alignment: `ceil(.005*Fs)+192` samples. The exact dry
delays and wet impulse peak positions are tested.

| Rate | Limiter | All added stages |
| --- | ---: | ---: |
| 44.1 kHz | 413 | 637 |
| 48 kHz | 432 | 656 |
| 96 kHz | 672 | 896 |
| 192 kHz | 1152 | 1376 |

## Measured qualification

`Tests/MasteringStagesTests.cpp` exercises exact delayed bypass, wet delay, shelf and
tilt responses, compressor hard/soft knees and stereo linking, dynamic-band range
and audition/reset, saturation small-signal normalization, clipping shapes, alias
power, true-peak reconstruction, limiting, and sample/block determinism.

Forty limiter fixtures cover ten signal types at four rates: positive and negative
impulses/pairs, a two-up/two-down square, 0.49 Fs sine, seeded noise, abrupt sine-level
change, multitone, DC, exact alternating polarity, and a fractional bandlimited
impulse. Tests include startup, complete tails, and both channels. At 48 kHz with a
-1 dB requested ceiling, the largest measured refined-oracle peak was -1.4678 dBTP;
the largest 64x/256 versus 128x/512 difference was 0.000613 dB. These are fixture
measurements, not universal margins. A separate regression checks downward ceiling
automation after its ramp.

Coherent hard-clipping fixtures at approximately 996, 4998, 10002, and 17994 Hz with
+6/+12/+24 dB drive measure total residual power after excluding intended in-band odd
harmonics. Compared with 1x clipping, the 8x path improved residual power by
20.207–47.688 dB across those twelve fixtures. This is a tonal alias measurement,
not a broadband guarantee or a two-tone intermodulation reference comparison.

On the development machine, an optimized standalone build processed one second of
stereo audio through all six enabled stages plus the output peak detector in about
0.093 seconds at 48 kHz and 0.381 seconds at 192 kHz. These wall-clock observations
exclude the plugin wrapper, loudness meter, UI, and other plugins; they are not a
host scheduling guarantee. The independent integration tests instrument allocation
and cover callback partitioning, parameter changes, and reset.
