# VADPCM

This is an implementation of the VADPCM codec. The VADPCM codec is often used by Nintendo 64 games—this includes both homebrew games made using [LibDragon][libdragon] and commercial games made during the 1990s and 2000s with the proprietary SDK, [LibUltra][libultra].

[libdragon]: https://github.com/DragonMinded/libdragon
[libultra]: https://n64brew.dev/wiki/Libultra

VADPCM is licensed under the terms of the Mozilla Public License Version 2.0. See LICENSE.txt for details.

## Usage

Convert an AIFF file to use VADPCM:

    vadpcm encode input.aiff output.aifc

Convert a VADPCM file back to AIFF:

    vadpcm decode input.aifc output.aiff

VADPCM-encoded files are always AIFF-C files with the `.aifc` extension. The unencoded version can be AIFF, AIFF-C, or WAV.

## Project Status

The project is, as of January 2026, under active development. Version 1.0 is expected within the next six months.

## What is VADPCM?

VADPCM is a variant of ADPCM. It encodes blocks of 16 samples of audio into 9 bytes of data, giving 4.5 bits per sample. There are no settings or parameters to change the bit rate; the bit rate is always 4.5 bits per sample.

The audio quality of VADPCM is good, but this codec is not competitive with codecs like Opus, Vorbis, or even MP3.

### Technical Details

VADPCM uses a codebook of second-order linear predictors, and encodes the residuals using 4 bits per sample. Each block of 16 samples shares the same predictor and scaling factor. The predictors in the codebook are stored in a vector format, which makes it easy to write a high-performance decoder using SIMD instructions. The Nintendo 64 has a coprocessor, called the Reality Signal Processor, with SIMD capabilities.

## Building

### CMake

You can build using CMake:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
	make -C build -j4

This places the main program at `build/vadpcm`.

If you are doing development work, recommended CMake flags are:

- `-G Ninja` to use Ninja instead of Make,
- `-DENABLE_WARNINGS=ON` to enable compiler warnings,
- `-DENABLE_WERROR=ON` to treat warning sas errors,
- `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to generate the compilation database for Clangd,
- `DCMAKE_C_FLAGS='-fdiagnostics-color=always` for color warnings and errors.

### Bazel

You can build using Bazel:

    bazel build -c opt //vadpcm

This places the main program at `bazel-bin/vadpcm/vadpcm`.

If you are doing development work, generate the compilation database with hedron:

    bazel run @hedron_compile_commands//:refresh_all

Recommended Bazel flags are `--//bazel:warnings=error`.

## Development



For CMake, you may want the `compile_commands.json`, which 



Pass `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to CMake to generate the `compile_commands.json` file that Clangd uses. On Unix systems, this will be symlinked into the project source directory.
