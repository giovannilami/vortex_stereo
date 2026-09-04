# Contributing

Bug reports and pull requests are welcome.

## Development setup

Clone the repository with its Disting NT API submodule:

```bash
git clone --recurse-submodules <repository-url>
cd Vortex_stereo
```

Run the desktop regression tests:

```bash
make test
```

Build the hardware object with an ARM GNU toolchain:

```bash
make
```

If the compiler is not on `PATH`, set it explicitly:

```bash
make ARM_CXX=/path/to/arm-none-eabi-c++
```

Please keep audio processing free of memory allocation and locks, preserve the
`VtxS` GUID, and add regression coverage for DSP or routing changes.
