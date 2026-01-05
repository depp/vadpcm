import argparse
import polars as pl

import load


def main():
    p = argparse.ArgumentParser()
    p.add_argument("infile")
    args = p.parse_args()
    infile = args.infile

    df = load.load(infile)
    print(df)


if __name__ == "__main__":
    main()
