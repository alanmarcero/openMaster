# Complete mastering chain

User scope: add tone EQ, stereo-linked bus compression, three-band dynamic EQ,
saturation, oversampled clipping, true-peak limiting, LUFS metering and loudness-
matched bypass. Preserve the Studio 84 LED visual direction; keep everyday controls
simple and expose detailed settings only for the selected section.

Constraints: preserve existing parameter IDs/order/defaults. New stages default off.
Mono/stereo supported; all latency reported and bypass paths aligned. No allocation,
locks or IO in processing. DSP methods must cite researched primary sources and have
independent signal tests. Existing unrelated UI work stays intact. No push/release.

1. DSP: pure C++ tone/dynamic EQ, compressor, oversampled color and clipper, limiter,
   reconstructed peak detector. Per-sample stereo processing, fixed prepared latency.
2. Metering/integration: K-weighted short-term and gated integrated loudness, append
   host parameters, chain order and aligned bypass, safe loudness comparison trim.
3. Editor: six sections, compact primary controls, selected detail strip, persistent
   LED output/loudness/reduction meters. Update documentation and native preview.
4. Verification: build VST3, DSP fixtures, state/automation/latency/bypass tests,
   native UI interactions, independent review and fixes.

Interfaces: MasteringSettings.h owns settings. MasteringStages.h owns stage API.
MasterProcessorCore owns audio state and publishes atomic UI readings. prepare may
allocate; process/reset must not. Settings are consumed on the audio thread.
