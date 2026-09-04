# Origin, purpose, and changes

## Project status

Vortex_stereo is an unofficial derivative of
[Vortex](https://github.com/wintocode/Vortex), originally programmed and
published by **wintocode** under the MIT License.

This adaptation was created for **Giovanni Lami's personal workflow**. Its
purpose is practical rather than conceptual: process stereo equipment through
Vortex without using two separate mono instances and manually matching them.
The project does not claim ownership of the original filter concept, modes,
control layout, or core filter implementation, and it is not presented as an
official update from the original author.

The starting point for this adaptation was Vortex v1.0.3. The derivative uses
a separate name and GUID so that it can coexist with the original plug-in.

## What comes from the original Vortex

- The multimode filter concept and all 12 filter modes.
- Cutoff, resonance, drive, dry/wet mix, and mode behavior.
- V/OCT, cutoff FM, resonance, mode, drive, and mix CV control.
- `FM Depth` as an attenuverter for `Cutoff FM CV`.
- The soft-clipping drive stage and the core filter equations.
- The original Disting NT plug-in structure and parameter organization.

The original Vortex credits Yuriy Ivantsov's
[ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters) for the
decramped state-space filter implementation. That attribution and the relevant
MIT license text are retained in this repository.

## Functional changes in Vortex_stereo

- Added a selectable `Channels` parameter with Mono and Stereo modes.
- Added explicit left/right input and output routing.
- Added independent filter memory for the left and right channels, preventing
  signal history or resonance from leaking between them.
- When `Input R` is Off in Stereo mode, the left input is duplicated to the
  right path for convenient dual-mono operation.
- Mono mode skips right-channel signal processing.
- Added a distinct `VtxS` GUID so Vortex_stereo can coexist with Vortex.
- The Algorithm menu name includes the plug-in version.

The two channels intentionally share mode, cutoff, resonance, drive, mix, and
CV modulation. This is linked stereo processing, not two independently
controlled filters.

## CPU-oriented implementation changes

These changes are intended to remove redundant work while preserving the
original filter equations and principal sound behavior:

1. **Coefficient caching** — Filter coefficients are recalculated only when
   mode, cutoff, resonance, or sample rate changes. A static patch therefore
   avoids repeating square roots and divisions for every audio sample.
2. **Coefficient sharing** — One coefficient set is calculated and copied to
   the right channel. Cascaded modes also copy coefficients to their second
   stage. State memory remains independent.
3. **Combined exponential modulation** — V/OCT and Cutoff FM offsets are summed
   before one `powf` calculation instead of using two exponent calculations.
4. **Active-state denormal handling** — Only state belonging to the selected
   filter mode is checked and flushed.
5. **Dry/wet endpoint shortcuts** — At exactly 0% or 100% Mix, unnecessary
   blend arithmetic is skipped.
6. **Mono short path** — The entire right filter path is skipped in Mono mode.
7. **Speed-oriented build** — The ARM object is compiled with `-O3` and safe
   floating-point flags appropriate to this implementation.

## Limits of the CPU claims

- No numerical CPU-reduction claim is made without repeatable measurements on
  a physical disting NT.
- Stereo mode necessarily processes a second audio channel and will normally
  cost more than Mono mode.
- If cutoff or resonance is modulated at audio rate and changes every sample,
  coefficient caching will miss frequently and provide less benefit.
- The regression suite checks DSP behavior, routing, modulation, and channel
  isolation, but it is not a substitute for listening tests or hardware CPU
  measurements.

## Intent

This repository is shared in case the same stereo convenience is useful to
other Vortex users. It should be understood first as a respectful, narrowly
scoped adaptation of wintocode's work.
