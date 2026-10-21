# Mastering expansion validation

Validated locally on 2026-10-21, Apple M4 Pro, Release build. No Windows build or
listening evaluation was performed in this session.

- **63/63 CTest checks passed**, including the native editor/state test.
- **Pluginval strictness 5 passed**, seed 8426, at 44.1/48/96/192 kHz.
- **18 DSP cases / 14,745 assertions passed** in standalone optimized and
  AddressSanitizer + UndefinedBehaviorSanitizer builds, with no diagnostics.
- Callback allocation instrumentation passed while automating all stage types,
  toggling bypass/matching, polling meters, and resetting measurements.
- Native checks exercised all **66 parameters**, eight detail views, five themes,
  typed frequency values, non-default state round-trip, old-session restoration,
  and transport reset. The committed preview is an actual native editor capture.
- The built VST3 passed `codesign --verify --deep --strict` with its local ad-hoc signature.

Signal qualification, sources, measured alias reduction, limiter ceiling margin,
bandwidth, CPU observations and loudness gating approximation are recorded in
[stage notes](MASTERING_STAGES.md) and [integration notes](MASTERING_DSP.md).

At 48 kHz, the prepared latency is **656 samples (13.67 ms)**. New stages default off.
Independent review found and verified fixes for transport-reset delay history and
measurement-reset true-peak history. Regression tests cover both.

Reproduce the main checks:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAMS_BUILD_HOST_CHECKS=ON
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
pluginval --strictness-level 5 --random-seed 8426 \
  --sample-rates 44100,48000,96000,192000 \
  --validate build/openMaster_artefacts/Release/VST3/openMaster.vst3
```

The optional host checks require a desktop session. Standard core tests can run
without the plugin by setting `AMS_BUILD_PLUGIN=OFF` and leaving host checks off.
