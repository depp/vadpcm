import argparse
import pathlib
import re
import subprocess
import sys

from typing import List, Tuple

ALPHANUM = re.compile(r"[a-zA-Z0-9]+")


def simplify_name(name: str) -> str:
    return "_".join(ALPHANUM.findall(name.replace("'", "")))


def convert_to_pcm(
    *, input_file: pathlib.Path, output_file: pathlib.Path, rate: int
) -> None:
    cmd: List[str | pathlib.Path] = [
        "ffmpeg",
        # Quiet
        "-hide_banner",
        "-loglevel",
        "error",
        # Overwrite
        "-y",
        # Input
        "-i",
        input_file,
        # Downmix to mono
        "-ac",
        "1",
        # Resample
        "-ar",
        str(rate),
        # Specify desired sample format explicitly, even though FFmpeg seems to
        # use this by default.
        "-sample_fmt",
        "s16",
        # Output
        output_file,
    ]
    proc = subprocess.run(cmd)
    if proc.returncode:
        print(
            f"Error: ffmpeg failed with status {proc.returncode}",
            file=sys.stderr,
        )
        raise SystemExit(1)


def main() -> None:
    p = argparse.ArgumentParser("convert.py", allow_abbrev=False)
    p.add_argument("-input", required=True, help="Input directory", type=pathlib.Path)
    p.add_argument("-output", required=True, help="Output directory", type=pathlib.Path)
    args = p.parse_args()
    input_dir: pathlib.Path = args.input
    output_dir: pathlib.Path = args.output

    output_dir.mkdir(exist_ok=True)
    rates: List[int] = [44100, 32000, 22050, 16000, 11025]
    rate_dirs: List[Tuple[int, pathlib.Path]] = []
    for rate in rates:
        rate_dir = output_dir / str(rate)
        rate_dir.mkdir(exist_ok=True)
        rate_dirs.append((rate, rate_dir))
    names = set()
    for item in input_dir.iterdir():
        if item.name.startswith("."):
            continue
        name = simplify_name(item.stem)
        if not name:
            print(
                "Warning: Skipping file with special characters:",
                repr(name),
                file=sys.stderr,
            )
            continue
        if name in names:
            print("Warning: Skipping duplicate name:", name, file=sys.stderr)
            continue
        print(f"Converting: {name}", file=sys.stderr)
        names.add(name)
        pcm_name = name + ".pcm.aiff"
        for rate, rate_dir in rate_dirs:
            print(f"    {rate} Hz", file=sys.stderr)
            pcm_file = rate_dir / pcm_name
            convert_to_pcm(input_file=item, output_file=pcm_file, rate=rate)


if __name__ == "__main__":
    main()
