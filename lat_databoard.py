#!/usr/bin/env python3
"""Plot KXO latency histogram after removing top‑5 % outliers

This script reads a latency log (default: /tmp/lat.log), keeps the fastest 95 % of
samples, and draws a histogram.  The resulting PNG is **saved in the same
folder as this script**, using the same basename with a *.png* extension.

Usage
-----
    python3 lat_databoard_q95.py [LOG_PATH] [--bins N]

Arguments
~~~~~~~~~
* **LOG_PATH**   Optional. Path to the log file written by `cat …/latency_ns`.
                 Defaults to */tmp/lat.log*.
* **--bins N**   Optional. Number of histogram bins (default 60).
"""

from __future__ import annotations

import argparse
import pathlib
import sys

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot KXO latency histogram after trimming the slowest 5 %",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "log",
        nargs="?",
        default="/tmp/lat.log",
        help="path to latency log (one value per line, in ns)",
    )
    parser.add_argument("--bins", type=int, default=60, help="histogram bins")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    log_path = pathlib.Path(args.log)
    if not log_path.exists():
        sys.exit(f"log file {log_path} not found")

    # 1. read log and convert to µs
    df = pd.read_csv(log_path, header=None, names=["ns"])
    df = df[df["ns"] > 0]  # drop empty slots
    df["us"] = df["ns"] / 1_000  # ns → µs

    # 2. keep fastest 20 %
    percentile = 0.20
    p95 = df["us"].quantile(percentile)
    df = df[df["us"] <= p95]
    kept = len(df)
    if kept == 0:
        sys.exit("no data after 95th‑percentile trimming – nothing to plot")
    print(f"kept {kept} samples (≤ {p95:.2f} µs) for histogram")

    # 3. build bins
    lo = df["us"].min()
    bins = np.linspace(lo, p95, args.bins)

    # 4. plot
    fig, ax = plt.subplots()
    ax.hist(df["us"], bins=bins)
    ax.set_xlabel("Latency (µs)")
    ax.set_ylabel("Frequency")
    ax.set_title(f"KXO latency ≤{percentile}th percentile (cut at {p95:.2f} µs)")
    fig.tight_layout()

    # 5. save PNG next to this script
    out_png = pathlib.Path(__file__).resolve().with_suffix(".png")
    fig.savefig(out_png, dpi=150)
    print(f"histogram saved to {out_png}")


if __name__ == "__main__":
    main()




