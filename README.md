# openMaster

A mastering processor with filtering, tone EQ, stereo shaping, compression, dynamic EQ, saturation, clipping, limiting, and loudness metering.

[Download VST3](https://github.com/alanmarcero/openMaster/releases/latest) · [Install with OpenVST Manager](https://github.com/alanmarcero/alan-m-OpenVSTManager/releases/latest)

## 1980s Studio Rack Interface

The entire editor follows the stereo LED meters: a generated charcoal aluminum faceplate, smoked display windows, green seven-segment value readouts, segmented knob scales, dark fluted knobs, and rectangular switches with green or amber status lamps. The numeric fields remain editable. Labels, controls, and meters render live; the background is embedded in the plugin.

![Actual openMaster Studio 84 editor](assets/native-ui.png)

Captured from the running VST3. See [screenshot provenance](docs/SCREENSHOTS.md).

The **FACEPLATE** selector offers five illumination palettes over the same rack hardware:

| Theme | Display / status colors |
| --- | --- |
| Studio 84 (default) | Green / amber |
| Silver | Soft white / amber |
| Phosphor | Green / pale lime |
| Ice | Ice blue / amber |
| Amber | Amber / warm red |

Meter level colors remain green, amber, and red in every theme. Theme indices are unchanged, so existing sessions retain their selected slot. The [artwork prompt](assets/ANALOG-CYBER.md) documents the generated faceplate.

---

## Mastering chain

The six cards expose the everyday controls. Click a card heading for its detailed
settings. Dynamics and Color each have a selector in the detail strip. Dynamic EQ
has a band selector, so all three bands share the same controls.

| Section | Main controls | Detail controls |
| --- | --- | --- |
| Filters | Low cut, high cut; individual switches | 12–96 dB/oct slopes |
| Tone | Low and high shelf gain | Tilt, shelf frequencies |
| Stereo | Low-band width, crossover | Low-band audition |
| Dynamics | Compressor threshold, dynamic EQ amount | Ratio, attack, release, knee, mix, makeup, detector HP; three EQ bands with frequency, Q, threshold, max cut, timing and audition |
| Color | Saturation drive, clipper drive | Tape/Transformer curves, mix, output and gain compensation; soft/hard clip, ceiling and mix |
| Output | Limiter drive, ceiling | Release and pre-limit master trim |

Signal order: low cut → low-band stereo → high cut → tone → compressor →
dynamic EQ → saturation → master trim → clipper → limiter.

New stages default **off**. Existing parameter IDs, order, ranges and defaults
remain intact. Old sessions retain their filtering/stereo settings and leave new
stages off. Oversampling and lookahead add a fixed latency, reported to the host;
stage switches and global/host bypass preserve that latency.

Saturation and clipping use fixed 8× oversampling. The limiter uses stereo-linked
lookahead and reconstructed-peak detection. The saturation character names describe
waveshaping flavors, not physical emulations of a particular tape machine or transformer.

## Meters and comparison

- Stereo 30-segment VU bars, held sample peaks in dBFS, clip lamps, phase correlation.
- Integrated and 3-second short-term LUFS; held reconstructed true peak in dBTP.
- Separate compressor, dynamic EQ, clipper and limiter gain-reduction displays.
- **RESET METERS** starts a new loudness/peak measurement without clearing the audio chain.
- **MATCH BYPASS** measures aligned wet/dry short-term loudness. After 3 seconds,
  it smoothly attenuates the louder branch, up to 12 dB. Use **POWER** to compare.
  Matching affects audible output and therefore exports; turn it off for final delivery.
  During silence, the last valid comparison is held. This is an audition aid, not
  automatic target-LUFS mastering.

VU uses sine-RMS calibration and a 99%-in-300-ms response. Its reference defaults to
−14 dBFS and can be set from −8 to −20 dBFS. Loudness measures actual output,
including bypass and comparison trim. Integrated loudness uses BS.1770 weighting
and gates with a bounded 0.001 LU histogram; boundary gating is approximate.

Algorithm sources, timing, and limitations: [DSP research notes](docs/MASTERING_DSP.md)
and [stage implementation notes](docs/MASTERING_STAGES.md). These tests are engineering
validation, not a claim of broadcast-meter certification.

---

## Install

Download and open [OpenVST Manager](https://github.com/alanmarcero/alan-m-OpenVSTManager/releases/latest), refresh the catalog, choose **openMaster**, and install VST3. Rescan plugins in your DAW.

Release packages contain a universal macOS VST3 (Apple Silicon and Intel, macOS 11+) or a Windows x64 VST3. For manual installation, extract the release ZIP and copy `openMaster.vst3` to `~/Library/Audio/Plug-Ins/VST3` on macOS or your DAW's VST3 folder on Windows (the standard system path is `C:\Program Files\Common Files\VST3`). Close the DAW before replacing a bundle, then reopen and rescan. macOS bundles are ad-hoc signed and are not notarized. Audio Unit is not included.

## Architecture & Code Structure

```
openMaster/
├── .github/
│   └── workflows/
│       └── build.yml             # macOS Universal + Windows CI, pluginval, artifact packaging
├── assets/
│   ├── banner.jpg                # Photorealistic 16:9 hero banner
│   ├── icon.jpg                  # High-resolution plugin icon
│   ├── analog-cyber-faceplate.png # Embedded charcoal rack texture
│   ├── native-ui.png             # Native Studio 84 editor capture
│   └── ANALOG-CYBER.md           # Image-generation prompt
├── Source/
│   ├── Core/                     # Pure C++20 DSP engine (zero GUI dependencies)
│   │   ├── BiquadFilter.h/.cpp
│   │   ├── ButterworthCoeffs.h/.cpp
│   │   ├── MultiPoleFilter.h/.cpp
│   │   ├── LinkwitzRiley.h/.cpp
│   │   ├── StereoFieldShaper.h/.cpp
│   │   └── MasterProcessorCore.h/.cpp
│   └── Plugin/                   # JUCE 8 VST3 Plugin implementation
│       ├── Parameters.h/.cpp     # APVTS parameter layout
│       ├── AnalogLookAndFeel.h/.cpp # Custom vintage analog LookAndFeel
│       ├── VintageMeter.h/.cpp   # Segmented stereo LED meters and digital peak readouts
│       ├── PluginProcessor.h/.cpp# JUCE AudioProcessor
│       └── PluginEditor.h/.cpp   # JUCE AudioProcessorEditor
├── Tests/                        # Catch2 unit test suite
│   ├── BiquadFilterTests.cpp
│   ├── ButterworthCoeffsTests.cpp
│   ├── StereoFieldShaperTests.cpp
│   └── MasterProcessorCoreTests.cpp
├── CMakeLists.txt
├── LICENSE
└── README.md
```

---

## Building Locally

### Prerequisites

- CMake 3.22 or newer
- C++20 compatible compiler (Clang 14+, GCC 12+, MSVC 2022+)
- Xcode command-line tools on macOS or Visual Studio C++ build tools on Windows
- Network access for CMake to fetch JUCE 8.0.9 and Catch2 3.9.1

### Build & Run Tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

For a universal macOS build add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`; on Windows configure with `-A x64`. The VST3 is written under `build/openMaster_artefacts/Release/VST3/`.

Optional native editor/state checks (requires a desktop session):
```bash
cmake -S . -B build -DAMS_BUILD_HOST_CHECKS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
# On a single-configuration build, optionally capture all detail views and themes:
./build/open_master_host_checks /tmp/openmaster-previews
```


---

## Continuous Integration & Release Pipelines

Automated multi-platform CI runs on code pushes to `main`, pull requests, release tags, and manual dispatch:
- **macOS (`macos-14`):** Universal binary (`arm64` Apple Silicon + `x86_64` Intel), ad-hoc codesigning, automated unit tests, and validation via `pluginval` at strictness level 5.
- **Windows (`windows-latest`):** Native MSVC build, automated unit tests, validation via `pluginval`, and zipped distribution packages.

---

## License

Project code is MIT licensed. JUCE and bundled dependencies retain their own license terms; release ZIPs include third-party notices. Copyright (c) 2026 Alan Marcero.
