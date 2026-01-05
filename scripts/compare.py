import argparse
import polars as pl

import load


def by_rate(df):
    return df.group_by("rate").agg(pl.col("rel_error").pow(2).mean().log10().mul(10))


def main():
    p = argparse.ArgumentParser()
    p.add_argument("in1")
    p.add_argument("in2")
    args = p.parse_args()
    df1 = load.load(args.in1)
    df2 = load.load(args.in2)

    df1 = df1.select("rate", "file", "rel_error").rename({"rel_error": "before"})
    df2 = df2.select("rate", "file", "rel_error").rename({"rel_error": "after"})
    df = df1.join(df2, ["rate", "file"])
    before = pl.col("before").log10().mul(20)
    after = pl.col("after").log10().mul(20)
    delta = (after - before).alias("delta")
    print(df.with_columns(before, after, delta))
    df = (
        df.group_by("rate")
        .agg(
            pl.col("before").pow(2).mean().sqrt(),
            pl.col("after").pow(2).mean().sqrt(),
        )
        .sort("rate")
        .with_columns(before, after, delta)
    )
    print(df)


if __name__ == "__main__":
    main()
