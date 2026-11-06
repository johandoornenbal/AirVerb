# Third-party provenance

## Airwindows reverb engines

Source: https://github.com/airwindows/airwindows
Commit: `7fd5f5cae262a41e6dd7d9a83d60c46e84d3092a` (retrieved 2026-11-06).
The original `plugins/WinVST/kRockstar2`, `plugins/WinVST/kRockstar` and
`plugins/WinVST/kCyberCity` headers,
wrappers and processing sources are retained byte-for-byte under
`third_party/airwindows/`, together with the upstream MIT license.
Copyright Chris Johnson / Airwindows.

`Source/*Engine.*` replaces VST2 metadata and state management with standalone
engines. Float/double processing bodies are unchanged apart from method signatures.
Delay constants are scoped to their engine class, initialization is exposed as
`reset(sampleRate)`, and setters clamp normalized parameters to 0–1. Native defaults
are unchanged. Noise seeds are fixed at `0x12345678` / `0x87654321` for deterministic
tests; this changes noise/dither sequences relative to randomly seeded instances.
The original room-tone generation is retained.

Tests compile untouched original sources behind the minimal independent VST2 test
interface in `tests/shim/audioeffectx.h`; no VST2 SDK is used. Matching initial seeds
allow exact sample comparisons against the adapted engines.

Input/output trims, gain linking, output metering, LUFS matching and Original/Processed
comparison are separate wrapper features. Unity trims with no learned match and
Processed selected return the native engine output. No additional limiter, tail gate,
EQ or reverb is inserted. The interface and installer follow AirEq/AirComp patterns.
AirEq source baseline: `78c3719` (MIT), https://github.com/johandoornenbal/AirEq.

## Tape / ButterComp UI and utilities

The JUCE wrapper, rotary/analogue meter style, metering, level matching, gesture
linking, test structure, and build/sign/install scripts are adapted from
https://github.com/johandoornenbal/tape at
`aae9dbe86de5334558ca6c9e3f7cd89ef5874499` (MIT).
Its copyright notice is retained in LICENSE.
Tape's UI originated in ButterComp Studio,
https://github.com/johandoornenbal/codex-tryout,
commit `3d2daf0d2f350d73640bd03d3f9210cd1b09bbf6` (MIT).
ButterComp's level matching originated at
`c60fa1b798b02b158287f14783efa18aaa85b721`.
Its license notice is retained in `third_party/ButterComp-LICENSE`.

## Loudness reference

Stereo BS.1770 K-weighting filter design follows libebur128 by Jan Kokemueller
(MIT), pinned at `67b33abe1558160ed76ada1322329b0e9e058b02` from
https://github.com/jiixyj/libebur128.
The original C source and license are retained under `third_party/libebur128`
as an independent test oracle, not linked into the plugin. The queue compatibility
code retains its embedded BSD notices. `Source/LoudnessMatch.h` uses bounded
400 ms windows, 100 ms hops, a −70 LUFS absolute gate and −10 LU relative gate.

## JUCE and VST3

JUCE 8.0.12: `29396c22c93392d6738e021b83196283d6e4d850`.
https://github.com/juce-framework/JUCE/blob/8.0.12/LICENSE.md

JUCE is dual-licensed under AGPLv3 and commercial terms. Distribution must comply
with the applicable JUCE license, including corresponding source obligations when
using AGPLv3, or use an appropriate commercial license. The wrapper's MIT license
does not relicense JUCE. Retain the VST3 SDK notices bundled with JUCE.
This is an independent adaptation, not an official Airwindows or JUCE release.
