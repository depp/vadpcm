"""Analyze encoder error.

Requires a matching input file, encoded VADPCM file, and decoded file.
Displays the predictors used, the signal level, and the amount of encoding
error.
"""

import argparse
import numpy as np
import soundfile
import math
import struct
import sys


def die(*msg):
    print("\x1b[1;31mError:\x1b[0m", *msg, file=sys.stderr)
    raise SystemError(1)


class DataError(Exception):
    pass


def aiff_chunks(filename):
    chunks = []
    with open(filename, "rb") as fp:
        fp.seek(4)
        (size,) = struct.unpack(">I", fp.read(4))
        if size < 4:
            raise DataError("bad AIFF")
        fp.seek(12)
        pos = 4
        while pos < size:
            if pos + 8 > size:
                raise DataError("bad AIFF")
            chunk_head = fp.read(8)
            if len(chunk_head) < 8:
                raise DataError("bad AIFF")
            chunk_type = chunk_head[:4]
            (chunk_size,) = struct.unpack(">I", chunk_head[4:])
            chunk_data = fp.read(chunk_size)
            if len(chunk_head) < 8:
                raise DataError("bad AIFF")
            pos += 8 + chunk_size
            chunks.append((chunk_type, chunk_data))
            if chunk_size & 1:
                pos += 1
                fp.seek(pos)
    return chunks


def aiff_vadpcm_data(chunks):
    """Get the VADPCM data from an AIFF."""
    for name, data in chunks:
        if name == b"SSND":
            if len(data) < 8 or (len(data) - 8) % 9:
                raise DataError(f"bad VADPCM data length: {len(data)}")
            return np.frombuffer(data[8:], dtype=np.uint8).reshape((-1, 9))
    raise DataError("no SSND chunk")


def delta(*, original, encoded, decoded):
    # Load files.
    aiff = aiff_chunks(encoded)
    vadpcm = aiff_vadpcm_data(aiff)
    n = len(vadpcm)
    with soundfile.SoundFile(original) as fp:
        data1 = fp.read()
    data1 = np.pad(data1, (0, (-len(data1)) & 15))
    if len(data1) != n * 16:
        die("mismatched length")
    with soundfile.SoundFile(decoded) as fp:
        data2 = fp.read()
    if len(data1) != len(data2):
        die("mismatched length:", len(data1), len(data2))

    # Reshape to same chunk size as VADPCM (16 samples).
    data1 = data1.reshape((-1, 16))
    data2 = data2.reshape((-1, 16))

    # Calculate SNR
    delta = data2 - data1
    dsum = (data2 + data1) * 0.5
    print("SNR:", 10 * math.log10(np.sum(data1 * data1) / np.sum(delta * delta)), "dB")

    # Calculate error for plotting.
    delta2 = np.sum(delta * delta, axis=1)
    dsum2 = np.sum(dsum * dsum, axis=1)

    import matplotlib.pyplot as plt
    import matplotlib as mpl

    colors = mpl.color_sequences["tab10"]
    x = np.arange(n)
    fig, (ax1, ax2, ax3) = plt.subplots(3, sharex=True, height_ratios=[1, 2, 2])
    predictors = vadpcm[:, 0] & 15
    ax1.eventplot(
        [np.where(predictors == i)[0] for i in range(4)],
        colors=mpl.color_sequences["tab10"][:4],
    )
    ax1.set_yticks(np.arange(0, 4, 2))
    ax1.set_yticks(np.arange(0, 4), minor=True)
    ax1.set(xlim=(0, n - 1))
    ax1.set_ylabel("Predictor")
    ax2.plot(
        x,
        np.sqrt(np.sum(data1 * data1, axis=1) * (1.0 / 16.0)),
        linewidth=1.0,
        color=colors[0],
    )
    ax2.set_ylabel("Signal")
    ax3.plot(x, np.sqrt(delta2 / dsum2), linewidth=1.0, color=colors[1])
    ax3.set_ylabel("Error")
    plt.show()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("original")
    p.add_argument("encoded")
    p.add_argument("decoded")
    args = p.parse_args()

    delta(original=args.original, encoded=args.encoded, decoded=args.decoded)


if __name__ == "__main__":
    main()
