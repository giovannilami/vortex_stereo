# Vortex_stereo

> **Unofficial derivative of [Vortex by wintocode](https://github.com/wintocode/Vortex).**
> The original filter design, modes, controls, and core DSP belong to the
> original project. This adaptation was made for Giovanni Lami's personal need
> to use Vortex with stereo sources. It is not presented as a new original
> filter and is not an official release by, or endorsed by, the original author.

A selectable mono/stereo adaptation of Vortex for the
[Expert Sleepers Disting NT](https://expert-sleepers.co.uk/distingNT.html).

Vortex_stereo offers 12 filter modes from gentle 6 dB/oct slopes to steep 24 dB/oct cascades, with pre-filter drive, dry/wet mix, and 7 CV inputs. Mono mode processes one channel; Stereo mode uses independent filter state for left and right while sharing all sound and CV controls.

The core filter DSP was ported by the original author from
[ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters) by Yuriy
Ivantsov — decramped IIR state-space filters with sigma frequency warping for
clean audio-rate modulation.

## Why this adaptation exists

The goal is deliberately narrow: retain the sound and main behavior of Vortex
while making it convenient to process a stereo signal without loading and
matching two separate mono instances. Mono operation remains selectable when
only one channel is needed.

For an exact separation between upstream work, stereo changes, and CPU-oriented
implementation changes, see [ORIGIN_AND_CHANGES.md](ORIGIN_AND_CHANGES.md).

## Installation

1. Download or build `vortex_stereo.o` and `plugin.json` from the `plugins/` directory.
2. Copy both files into a dedicated folder under `/programs/plug-ins/` on the Disting NT MicroSD card.
3. Power on the Disting NT — `Vortex_stereo v1.0.0` will appear in the **Algorithm** menu under **Effects**.

`Vortex_stereo` uses its own `VtxS` GUID, so it can coexist with the original mono
`Vortex`. This also prevents the host from reusing the old plug-in's parameter
layout when testing the stereo build.

## Signal Chain

```
Input L/Mono -> Drive -> Filter L -> Mix -> Output L/Mono
Input R      -> Drive -> Filter R -> Mix -> Output R
```

## Filter Modes

| #  | Mode    | Slope       | Description                    |
|----|---------|-------------|--------------------------------|
| 0  | LP 6dB  | -6 dB/oct   | 1st-order low-pass             |
| 1  | LP 12dB | -12 dB/oct  | 2nd-order low-pass             |
| 2  | LP 24dB | -24 dB/oct  | Cascaded 2nd-order low-pass    |
| 3  | HP 6dB  | +6 dB/oct   | 1st-order high-pass            |
| 4  | HP 12dB | +12 dB/oct  | 2nd-order high-pass            |
| 5  | HP 24dB | +24 dB/oct  | Cascaded 2nd-order high-pass   |
| 6  | BP      | bandpass    | 2nd-order band-pass            |
| 7  | BP+     | bandpass    | Cascaded (narrower)            |
| 8  | Notch   | band reject | 2nd-order notch                |
| 9  | Notch+  | band reject | Cascaded (deeper)              |
| 10 | AP      | allpass     | 2nd-order all-pass             |
| 11 | AP+     | allpass     | Cascaded (more phase rotation) |

The 6 dB modes are gentle 1st-order filters — no resonance control. The 12 dB and 24 dB modes are 2nd-order (or cascaded 2nd-order) with full resonance support up to near self-oscillation. The "+" variants (BP+, Notch+, AP+) cascade two filter stages for steeper response.

## Parameters

Parameters are organized into pages on the Disting NT display.

### I/O

| Parameter   | Range       | Default | Description |
|-------------|-------------|---------|-------------|
| Channels      | Mono/Stereo | Mono    | Select one-channel or two-channel processing |
| Input L/Mono  | Bus 0-28    | 1       | Left or mono input; when Off, falls back to Audio In CV |
| Input R       | Bus 0-28    | 2       | Right input; Off duplicates the left input in Stereo mode |
| Output L/Mono | Bus 1-28    | 13      | Left or mono output bus |
| Output R      | Bus 1-28    | 14      | Right output bus, used only in Stereo mode |
| Output Mode   | Add/Replace | Add     | Applied to both active outputs |

### Filter

| Parameter | Range       | Default | Description |
|-----------|-------------|---------|-------------|
| Mode      | 0-11        | LP 12dB | Filter type (see table above) |
| Cutoff    | 20-20000 Hz | ~632 Hz | Cutoff frequency — exponential scaling for even response across the audio range |
| Resonance | 0-100%      | 0%      | Filter resonance. 0% = Butterworth (flat passband), 100% = near self-oscillation. Only affects 12 dB and 24 dB modes. |
| Drive     | 0-100%      | 0%      | Pre-filter soft-clip saturation. Boosts the signal 1x-10x then applies a smooth rational saturator for warm overdrive without hard clipping. |

### Global

| Parameter | Range        | Default | Description |
|-----------|--------------|---------|-------------|
| Mix       | 0-100%       | 100%    | Dry/wet blend. 0% = fully dry (bypass), 100% = fully wet |
| FM Depth  | -100 to 100% | 0%      | Attenuverter for Cutoff FM CV. It has no effect unless that CV input is connected. Negative values invert the modulation. |
| Version   | read-only    | -       | Displays the current plug-in version, also shown in the Algorithm menu |

### CV Inputs

Each CV input can be assigned to any bus on the Disting NT (0 = disconnected).

| Input        | Effect |
|--------------|--------|
| Audio In     | Audio input signal. Used when no audio bus is selected on the I/O page. |
| Cutoff V/OCT | 1V/oct cutoff frequency tracking. Multiplies the base cutoff exponentially — 1V doubles the frequency. |
| Cutoff FM    | Exponential cutoff modulation, scaled by FM Depth. At +100%, 1V doubles the cutoff; at -100%, 1V halves it. |
| Resonance    | Modulates resonance amount (±20% of range per volt) |
| Mode         | CV selection of filter mode (±5V sweeps all 12 modes) |
| Drive        | Modulates drive amount (±20% of range per volt) |
| Mix          | Modulates dry/wet blend (±20% of range per volt) |

## Patching Tips

- **Subtractive synth** — Feed a sawtooth oscillator into Audio In, set LP 24dB, Resonance at 30-50%, and modulate Cutoff with an envelope via V/OCT CV for classic analog-style patches.
- **Acid bass** — LP 12dB with high resonance (70-90%), moderate drive (30-50%), and a fast envelope on cutoff. The resonance peak creates the characteristic squelch.
- **DJ filter sweep** — Use LP 24dB or HP 24dB with Mix at 100%. Sweep Cutoff manually or via CV for dramatic build-ups and breakdowns.
- **Parallel filtering** — Set Output Mode to Mix and use multiple Vortex instances with different modes (e.g. LP + HP) on the same bus for creative crossover effects.
- **Warm saturation** — Even without filtering, use Drive at 40-60% with Mix at 100% in AP mode for transparent soft-clip warmth.
- **Phaser effect** — AP or AP+ mode with cutoff modulated by a slow LFO creates phase-shifting effects. Mix dry and wet signals for comb filtering.

## Building from Source

Requirements:
- `arm-none-eabi-c++` (ARM GCC toolchain)
- The [Disting NT API](https://github.com/expertsleepersltd/distingNT_API) (included as `distingNT_API/`)

```bash
git clone --recurse-submodules <repository-url>
cd Vortex_stereo
make
```

The build creates `plugins/vortex_stereo.o` and `plugins/plugin.json`.

Run tests (desktop):

```bash
cd tests && make run
```

## GitHub publication

The repository does not publish releases automatically. Its GitHub Actions
workflow only runs the tests, builds the ARM object, and stores the result as a
private workflow artifact. Manual publication instructions are in
[PUBLISHING.md](PUBLISHING.md), and the prepared v1.0.0 presentation text is in
[RELEASE_NOTES.md](RELEASE_NOTES.md).

## CPU optimization scope

This adaptation avoids several redundant calculations, most importantly by
caching filter coefficients and copying a calculated coefficient set to the
right channel and cascaded stages. It also avoids processing the right channel
in Mono mode. These changes are intended to reduce CPU use without changing
the filter equations.

No percentage reduction is claimed yet: a trustworthy figure requires
repeatable measurements on a physical disting NT. Stereo mode necessarily does
more signal processing than Mono mode, and coefficient caching helps less when
cutoff or resonance changes every sample. Technical details and limitations are
documented in [ORIGIN_AND_CHANGES.md](ORIGIN_AND_CHANGES.md).

## License

MIT — see [LICENSE](LICENSE).

## Authors and attribution

- **Original Vortex plug-in:** [wintocode](https://github.com/wintocode/Vortex)
- **Stereo adaptation created for:** Giovanni Lami
- **Underlying filter work:** Yuriy Ivantsov,
  [ivantsov-filters](https://github.com/yIvantsov/ivantsov-filters)

Copyright and full MIT attribution are preserved in [LICENSE](LICENSE) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
