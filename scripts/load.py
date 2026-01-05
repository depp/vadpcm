import polars as pl


SCHEMA = pl.Schema(
    {"file": pl.String(), "signal_rms": pl.Float64(), "error_rms": pl.Float64()}
)


def load(src):
    return (
        pl.read_csv(src, schema=SCHEMA)
        .with_columns(
            (pl.col("signal_rms") / pl.col("error_rms")).alias("rel_error"),
            pl.col("file")
            .str.split_exact("/", 1)
            .struct.rename_fields(["rate", "file"])
            .alias("file_parts"),
        )
        .drop("file")
        .unnest("file_parts")
        .with_columns(
            pl.col("rate").str.to_integer(),
            pl.col("file").str.strip_suffix(".aiff").str.replace_all("_", " "),
        )
    )
