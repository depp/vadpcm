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
