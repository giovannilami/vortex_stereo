# Vortex_stereo v1.0.0

First stable release of Vortex_stereo for the Expert Sleepers disting NT.

Vortex_stereo is an unofficial, narrowly scoped adaptation of
[Vortex by wintocode](https://github.com/wintocode/Vortex). It was created for
Giovanni Lami's personal need to use the original filter conveniently with
stereo sources. The original filter concept, modes, controls, and core DSP are
credited to wintocode; this release is not an official upstream version.

Vortex_stereo expands the original Vortex multimode filter with selectable
mono or true stereo processing. In Stereo mode, the left and right channels
use independent filter state while sharing cutoff, resonance, drive, mix, mode,
and CV modulation controls.

## Highlights

- Selectable Mono/Stereo operation.
- Independent left and right filter processing.
- 12 filter modes: 6, 12, and 24 dB/oct low-pass and high-pass, plus band-pass,
  notch, all-pass, and cascaded variants.
- Pre-filter drive and dry/wet mix.
- Cutoff V/OCT and Cutoff FM inputs.
- FM Depth attenuverter with positive and inverted modulation.
- CPU optimizations for coefficient calculation and denormal handling.
- Version displayed directly as `Vortex_stereo v1.0.0` in the Algorithm menu.

The CPU changes remove redundant calculations but this release does not claim
a measured percentage improvement. Stereo mode necessarily costs more than
Mono mode; formal on-device measurements are still required.

## Installation

Copy `vortex_stereo.o` and `plugin.json` into a dedicated folder under
`/programs/plug-ins/` on the disting NT MicroSD card, then restart or remount
the card.

For a clean first test, select Stereo mode and route inputs 1/2 to outputs
13/14 with Output Mode set to Replace.
