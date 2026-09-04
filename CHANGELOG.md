# Changelog

All notable changes to Vortex_stereo are documented here. Versions follow
[Semantic Versioning](https://semver.org/).

## 1.0.0 - 2026-09-03

First stable release, prepared for eventual public distribution.

Unofficial stereo adaptation of Vortex by wintocode, created for Giovanni
Lami's personal workflow. The original filter design and core DSP remain
credited to the upstream project.

### Added

- Selectable mono or true stereo processing.
- Independent filter state for the left and right channels.
- Left-to-right duplication when `Input R` is Off in Stereo mode.
- Version number in the Disting NT Algorithm menu.
- Host-level tests for stereo routing, channel isolation, and cutoff FM.
- GitHub Actions validation builds without automatic release publication.

### Retained

- All 12 original filter modes.
- Cutoff, resonance, drive, dry/wet mix, and their CV inputs.
- `FM Depth` and `Cutoff FM CV` behavior from the original Vortex.

### Optimized

- Coefficients are cached until a relevant control changes.
- Stereo channels and cascaded stages share calculated coefficients while
  retaining independent signal history.
- V/OCT and FM modulation use a single exponential calculation per sample.
- Denormal flushing is limited to active filter state.
