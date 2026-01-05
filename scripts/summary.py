import argparse
import polars as pl

import load


def main():
    p = argparse.ArgumentParser()
    p.add_argument("infile")
    args = p.parse_args()
    infile = args.infile

    df = load.load(infile)
    df = (
        df.group_by("rate")
        .agg(pl.col("rel_error").pow(2).mean().log10().mul(10))
        .sort("rate")
    )
    print(df)


if __name__ == "__main__":
    main()
