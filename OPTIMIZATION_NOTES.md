# Vortex_stereo 1.0.0

Selectable mono/stereo CPU-optimized build of Vortex for Disting NT. This
revision uses a clean, contiguous stereo I/O parameter layout matching the
official API examples and a separate `VtxS` GUID. The original Vortex can
therefore remain installed while this version is tested.

This is an unofficial derivative of Vortex by wintocode, made for Giovanni
Lami's personal need to process stereo sources. It does not claim authorship of
the original filter design or core DSP. See `ORIGIN_AND_CHANGES.md` for the
complete attribution and technical comparison.

## Mono/stereo operation

- `Channels = Mono` processes only `Input L/Mono` and writes `Output L/Mono`.
- `Channels = Stereo` processes left and right through independent filter state.
- Both channels share mode, cutoff, resonance, drive, mix, and CV modulation.
- When `Input R` is Off, the left signal is duplicated to the right channel.
- The existing Output Mode setting applies to both active outputs.
- Default routing is inputs 1/2 to outputs 13/14.

## Cutoff FM

- `FM Depth` and `Cutoff FM CV` retain the behavior of the original Vortex.
- `FM Depth` only scales `Cutoff FM CV`; changing it with no CV connected is
  therefore expected to be inaudible.
- `Cutoff V/OCT CV` remains available for exponential cutoff modulation.

## Transparent optimizations

- Cache filter coefficients until mode, cutoff, resonance, or sample rate changes.
- Configure cascaded modes once and copy coefficients to the second stage.
- Reuse the duplicated square root in the all-pass coefficient calculation.
- Combine V/OCT and exponential FM into one `powf` call.
- Flush denormals only for the active filter state.
- Use direct dry or wet output at the mix endpoints.
- Compile for speed with `-O3`; hardware square-root instructions replace
  external `sqrtf` calls on Cortex-M7.

Audio-rate cutoff and resonance modulation are still processed per sample.
There is no control-rate decimation in this build.

These changes are expected to reduce redundant CPU work, but no percentage
improvement is claimed until repeatable measurements are made on hardware.
Stereo mode necessarily performs more signal processing than Mono mode.

## Validation

- 36/36 desktop DSP tests pass with AddressSanitizer and UndefinedBehaviorSanitizer.
- Host-level routing tests verify the parameter layout, distinct L/R dry
  routing, and wet-channel isolation across separate Disting NT buses.
- A host-level modulation test verifies that FM Depth changes the response when
  Cutoff FM CV is connected.
- A stereo isolation regression confirms that left and right filter memories do
  not leak into one another.
- Static coefficient caching and cascaded coefficient copying are bit-identical
  to the original calculations in the regression tests.
- The ARM output is an ELF32 little-endian, hard-float Cortex-M7 relocatable
object and uses the new `VtxS` GUID.

## Installation

Copy `vortex_stereo.o` and `plugin.json` into a dedicated folder under the Disting NT
SD card plug-ins directory. The original Vortex may remain installed because
the algorithms have different GUIDs and names.

For performance comparisons, measure CPU use in Mono and Stereo for LP 12dB,
LP 24dB, and AP+ with and without cutoff CV.
