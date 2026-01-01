# VADPCM

This is an implementation of the VADPCM codec. The VADPCM codec is often used by Nintendo 64 games—this includes both homebrew games made using [LibDragon][libdragon] and commercial games made during the 1990s and 2000s with the proprietary SDK, [LibUltra][libultra].

[libdragon]: https://github.com/DragonMinded/libdragon
[libultra]: https://n64brew.dev/wiki/Libultra

VADPCM is licensed under the terms of the Mozilla Public License Version 2.0. See LICENSE.txt for details.

## Project Status

The project is, as of September 2023, under active development. Version 1.0 is expected within the next six months.

## What is VADPCM?

VADPCM is a variant of ADPCM. It encodes blocks of 16 samples of audio into 9 bytes of data, giving 4.5 bits per sample. There are no settings or parameters to change the bit rate; the bit rate is always 4.5 bits per sample.

The audio quality of VADPCM is good, but this codec is not competitive with codecs like Opus, Vorbis, or even MP3.

### Technical Details

VADPCM uses a codebook of second-order linear predictors, and encodes the residuals using 4 bits per sample. Each block of 16 samples shares the same predictor and scaling factor. The predictors in the codebook are stored in a vector format, which makes it easy to write a high-performance decoder using SIMD instructions. The Nintendo 64 has a coprocessor, called the Reality Signal Processor, with SIMD capabilities.

## Development

This project supports two build systems: Bazel and CMake. You can pick your preferred build system, although CMake support is currently incomplete.

### Bazel

The primary encoder tool is `//vadpcm`. It is writtin in Go.

    bazel build -c opt //vadpcm

The encoder / decoder library, written in C, is `//codec`.

    bazel build -c opt //codec

Generate the `compile_commands.json` file:

    bazel run @hedron_compile_commands//:refresh_all

### CMake

You can build using CMake:

    mkdir build
	cd build
	cmake ..
	make
	make test

The Ninja generator, enabled by passing `-G Ninja` to CMake, is recommended.

Pass `-DCMAKE_BUILD_TYPE=Release` to CMake for a release build.

Pass `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to CMake to generate the `compile_commands.json` file that Clangd uses. On Unix systems, this will be symlinked into the project source directory.
