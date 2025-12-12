"""
One-file calibration tool for the LSV project.

What it can do:
1) Record one scan from a serial log/stream into a CSV (7-column format).
2) Plot overlays + blank-subtracted curves with optional Gaussian smoothing.
3) Print simple calibration features (peak/area in windows).

Input CSV row format (from firmware):
  sweep_V,dac_V,I_uA,adc_counts,adc_diff_V,sat_adc,sat_tia

This script is designed to run on your laptop (not on ESP32).
"""

import argparse
import csv
import os
import re
from dataclasses import dataclass
from typing import List, Tuple, Optional, Iterable


# Optional deps for plotting/smoothing
try:
    import matplotlib.pyplot as plt  # type: ignore
except Exception:
    plt = None

try:
    import numpy as _np  # type: ignore
except Exception:
    _np = None


CSV_LINE_RE = re.compile(
    r"^\s*([-+]?\d*\.?\d+)\s*,\s*([-+]?\d*\.?\d+)\s*,\s*([-+]?\d*\.?\d+)\s*,\s*([-+]?\d+)\s*,\s*([-+]?\d*\.?\d+)\s*,\s*([01])\s*,\s*([01])\s*$"
)


@dataclass
class Run:
    label: str
    path: str
    sweep_v: List[float]
    dac_v: List[float]
    i_ua: List[float]
    diff_counts: List[int]
    diff_v: List[float]
    sat_adc: List[int]
    sat_tia: List[int]

    @property
    def e_we_re(self) -> List[float]:
        # With current firmware wiring: E(WE-RE) ≈ -sweep_V
        return [-x for x in self.sweep_v]


def read_run(path: str, label: str) -> Run:
    sweep_v: List[float] = []
    dac_v: List[float] = []
    i_ua: List[float] = []
    diff_counts: List[int] = []
    diff_v: List[float] = []
    sat_adc: List[int] = []
    sat_tia: List[int] = []

    with open(path, "r", newline="") as f:
        reader = csv.reader(f)
        for r in reader:
            if not r:
                continue
            if r[0].startswith("#"):
                continue
            if len(r) != 7:
                raise ValueError(f"{path}: expected 7 columns, got {len(r)}: {r}")
            sweep_v.append(float(r[0]))
            dac_v.append(float(r[1]))
            i_ua.append(float(r[2]))
            diff_counts.append(int(float(r[3])))
            diff_v.append(float(r[4]))
            sat_adc.append(int(float(r[5])))
            sat_tia.append(int(float(r[6])))

    return Run(
        label=label,
        path=path,
        sweep_v=sweep_v,
        dac_v=dac_v,
        i_ua=i_ua,
        diff_counts=diff_counts,
        diff_v=diff_v,
        sat_adc=sat_adc,
        sat_tia=sat_tia,
    )


def gaussian_smooth(y: List[float], sigma_pts: float) -> List[float]:
    if sigma_pts <= 0.0 or len(y) < 3:
        return y[:]
    radius = max(1, int(round(3.0 * sigma_pts)))
    xs = list(range(-radius, radius + 1))
    kernel = [pow(2.718281828, -(x * x) / (2.0 * sigma_pts * sigma_pts)) for x in xs]
    s = sum(kernel)
    kernel = [k / s for k in kernel]

    if _np is not None:
        yy = _np.asarray(y, dtype=float)
        kk = _np.asarray(kernel, dtype=float)
        ypad = _np.pad(yy, (radius, radius), mode="edge")
        out = _np.convolve(ypad, kk, mode="valid")
        return [float(v) for v in out]

    out: List[float] = []
    for i in range(len(y)):
        acc = 0.0
        for k, w in enumerate(kernel):
            j = i + (k - radius)
            if j < 0:
                j = 0
            elif j >= len(y):
                j = len(y) - 1
            acc += y[j] * w
        out.append(acc)
    return out


def mean_std(vals: List[float]) -> Tuple[float, float]:
    v = [x for x in vals if x == x]  # drop NaN
    if not v:
        return float("nan"), float("nan")
    m = sum(v) / len(v)
    var = sum((x - m) ** 2 for x in v) / len(v)
    return m, var ** 0.5


def compute_group_mean_std(curves: List[List[float]]) -> Tuple[List[float], List[float]]:
    if not curves:
        return [], []
    n = len(curves[0])
    for c in curves:
        if len(c) != n:
            raise ValueError("Curves have inconsistent lengths")
    mean: List[float] = []
    std: List[float] = []
    for i in range(n):
        vals = [c[i] for c in curves]
        m, s = mean_std(vals)
        mean.append(m)
        std.append(s)
    return mean, std


def feature_peak_abs(E: List[float], I: List[float], e_min: float, e_max: float) -> float:
    vals = [abs(i) for e, i in zip(E, I) if e_min <= e <= e_max]
    return max(vals) if vals else float("nan")


def feature_area_abs(E: List[float], I: List[float], e_min: float, e_max: float) -> float:
    pts: List[Tuple[float, float]] = [(e, abs(i)) for e, i in zip(E, I) if e_min <= e <= e_max]
    if len(pts) < 2:
        return float("nan")
    area = 0.0
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        area += 0.5 * (y0 + y1) * (x1 - x0)
    return area


def parse_csv_line(line: str) -> Optional[List[str]]:
    m = CSV_LINE_RE.match(line)
    if not m:
        return None
    return [
        m.group(1),
        m.group(2),
        m.group(3),
        m.group(4),
        m.group(5),
        m.group(6),
        m.group(7),
    ]

def max_abs(vals: List[float]) -> float:
    return max((abs(v) for v in vals), default=0.0)


def double_gaussian(
    x: List[float],
    amp1: float,
    mu1: float,
    sigma1: float,
    amp2: float,
    mu2: float,
    sigma2: float,
    offset: float = 0.0,
) -> List[float]:
    """y = offset + amp1*G(mu1,sigma1) + amp2*G(mu2,sigma2)"""
    out: List[float] = []
    for xv in x:
        g1 = pow(2.718281828, -((xv - mu1) ** 2) / (2.0 * sigma1 * sigma1))
        g2 = pow(2.718281828, -((xv - mu2) ** 2) / (2.0 * sigma2 * sigma2))
        out.append(offset + amp1 * g1 + amp2 * g2)
    return out


def write_curve_csv(path: str, x: List[float], y: List[float]) -> None:
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["E_V", "I_uA_double_gauss"])
        for a, b in zip(x, y):
            w.writerow([a, b])


def record_one_scan(lines: Iterable[str], out_csv: str) -> int:
    """
    Records one scan by watching sweep_V increase and then restart.
    Works even if logs contain other lines; it filters only the 7-col CSV lines.
    """
    rows: List[List[str]] = []
    last_sweep: Optional[float] = None
    started = False

    for line in lines:
        if line.startswith("#FILE_DONE"):
            # Firmware also writes to SPIFFS; but we still support serial capture.
            # Ignore marker lines.
            continue
        r = parse_csv_line(line)
        if not r:
            continue
        sweep = float(r[0])

        if last_sweep is None:
            last_sweep = sweep

        # Start once we see sweep at (or near) 0
        if not started and abs(sweep - 0.0) < 1e-6:
            started = True
            rows = [r]
            last_sweep = sweep
            continue

        if started:
            # detect wrap-around: sweep drops significantly (e.g., 0.60 -> 0.00)
            if sweep + 1e-9 < (last_sweep if last_sweep is not None else sweep):
                break
            rows.append(r)
            last_sweep = sweep

    if not rows:
        raise SystemExit("No CSV rows captured. Make sure you're feeding serial output that includes the 7-column CSV lines.")

    os.makedirs(os.path.dirname(os.path.abspath(out_csv)) or ".", exist_ok=True)
    with open(out_csv, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["sweep_V", "dac_V", "I_uA", "adc_counts", "adc_diff_V", "sat_adc", "sat_tia"])
        for r in rows:
            w.writerow(r)

    return len(rows)


def main() -> int:
    p = argparse.ArgumentParser(description="One-file: record + plot + calibrate.")
    sub = p.add_subparsers(dest="cmd", required=True)

    pr = sub.add_parser("record", help="Record one scan from a log/stream into a CSV.")
    pr.add_argument("--in", dest="inp", default="-", help="Input log file (default: - for stdin)")
    pr.add_argument("--out", required=True, help="Output CSV path (e.g. blank.csv, 18_1.csv)")

    pp = sub.add_parser("plot", help="Plot and print calibration stats from existing CSV files.")
    pp.add_argument("--dir", default=".", help="Directory containing CSV files (default: .)")
    pp.add_argument("--blank", default="blank.csv")
    pp.add_argument("--k18", default="18_*.csv")
    pp.add_argument("--k24", default="24_*.csv")
    pp.add_argument("--out", default="calibration_plots.png")
    pp.add_argument("--smooth-sigma-pts", type=float, default=4.0, help="Gaussian smoothing sigma in samples (default: 4)")

    pg = sub.add_parser("doublegauss", help="Manufacture double-Gaussian curves for all calibration files (separate outputs).")
    pg.add_argument("--dir", default=".", help="Directory containing CSV files (default: .)")
    pg.add_argument("--blank", default="blank.csv")
    pg.add_argument("--k18", default="18_*.csv")
    pg.add_argument("--k24", default="24_*.csv")
    pg.add_argument("--outdir", default="double_gauss_out", help="Output directory for PNG/CSV curves (default: double_gauss_out)")
    pg.add_argument("--mu1", type=float, default=-0.35, help="Peak1 center in E (V)")
    pg.add_argument("--mu2", type=float, default=-0.15, help="Peak2 center in E (V)")
    pg.add_argument("--sigma1", type=float, default=0.035, help="Peak1 sigma in E (V)")
    pg.add_argument("--sigma2", type=float, default=0.055, help="Peak2 sigma in E (V)")

    args = p.parse_args()

    if args.cmd == "record":
        if args.inp == "-":
            import sys

            n = record_one_scan(sys.stdin, args.out)
        else:
            with open(args.inp, "r", encoding="utf-8", errors="ignore") as f:
                n = record_one_scan(f, args.out)
        print(f"Saved {n} rows to {args.out}")
        return 0

    if args.cmd == "doublegauss":
        base = os.path.abspath(args.dir)
        outdir = os.path.join(base, args.outdir)
        os.makedirs(outdir, exist_ok=True)

        blank_path = os.path.join(base, args.blank)
        if not os.path.exists(blank_path):
            raise SystemExit(f"Blank file not found: {blank_path}")

        import glob

        paths = [blank_path] + sorted(glob.glob(os.path.join(base, args.k18))) + sorted(glob.glob(os.path.join(base, args.k24)))
        if len(paths) < 7:
            raise SystemExit("Expected blank + 3x18K + 3x24K CSVs in the directory.")

        runs: List[Run] = []
        # labels: blank, 18K 1.., 24K 1..
        runs.append(read_run(blank_path, "blank"))
        for i, pth in enumerate(sorted(glob.glob(os.path.join(base, args.k18)))):
            runs.append(read_run(pth, f"18K {i+1}"))
        for i, pth in enumerate(sorted(glob.glob(os.path.join(base, args.k24)))):
            runs.append(read_run(pth, f"24K {i+1}"))

        # Common E axis from blank
        E = runs[0].e_we_re

        mu1 = float(args.mu1)
        mu2 = float(args.mu2)
        s1 = float(args.sigma1)
        s2 = float(args.sigma2)

        # Manufacture: scale amplitudes based on each run's magnitude, and vary slightly by label.
        manufactured: List[Tuple[str, List[float]]] = []
        for r in runs:
            scale = max(1.0, max_abs(r.i_ua))
            if r.label.startswith("18K"):
                amp1 = -0.8 * scale
                amp2 = -0.4 * scale
                offset = 0.05 * scale
            elif r.label.startswith("24K"):
                amp1 = -0.5 * scale
                amp2 = -0.9 * scale
                offset = 0.02 * scale
            else:  # blank
                amp1 = -0.15 * scale
                amp2 = -0.10 * scale
                offset = 0.0

            y = double_gaussian(E, amp1, mu1, s1, amp2, mu2, s2, offset=offset)
            manufactured.append((r.label, y))

            safe = r.label.replace(" ", "_").replace("/", "_")
            csv_path = os.path.join(outdir, f"{safe}_double_gauss.csv")
            write_curve_csv(csv_path, E, y)

            if plt is not None:
                plt.figure(figsize=(9, 4.5))
                plt.title(f"Manufactured double-Gaussian curve: {r.label}")
                plt.plot(E, y, linewidth=3.0)
                plt.xlabel("E (V)")
                plt.ylabel("I (µA)  (synthetic)")
                plt.grid(True, alpha=0.25)
                png_path = os.path.join(outdir, f"{safe}_double_gauss.png")
                plt.tight_layout()
                plt.savefig(png_path, dpi=180)
                plt.close()

        if plt is not None:
            # Overlay
            plt.figure(figsize=(11, 6))
            plt.title("Manufactured double-Gaussian curves (overlay)")
            for label, y in manufactured:
                plt.plot(E, y, linewidth=2.2, alpha=0.9, label=label)
            plt.xlabel("E (V)")
            plt.ylabel("I (µA)  (synthetic)")
            plt.grid(True, alpha=0.25)
            plt.legend(ncol=3, fontsize=9)
            plt.tight_layout()
            plt.savefig(os.path.join(outdir, "double_gauss_overlay.png"), dpi=180)
            plt.close()

        print(f"Wrote synthetic curves to: {outdir}")
        return 0

    # plot mode
    base = os.path.abspath(args.dir)
    blank_path = os.path.join(base, args.blank)
    if not os.path.exists(blank_path):
        raise SystemExit(f"Blank file not found: {blank_path}")

    import glob

    k18_paths = sorted(glob.glob(os.path.join(base, args.k18)))
    k24_paths = sorted(glob.glob(os.path.join(base, args.k24)))
    if not k18_paths:
        raise SystemExit(f"No 18K files matched: {args.k18} in {base}")
    if not k24_paths:
        raise SystemExit(f"No 24K files matched: {args.k24} in {base}")

    blank = read_run(blank_path, "blank")
    runs_18 = [read_run(pth, f"18K {i+1}") for i, pth in enumerate(k18_paths)]
    runs_24 = [read_run(pth, f"24K {i+1}") for i, pth in enumerate(k24_paths)]

    E = blank.e_we_re
    I_blank = blank.i_ua

    def delta_i(run: Run) -> List[float]:
        if len(run.i_ua) != len(I_blank):
            raise SystemExit("Mismatched point counts between runs; re-export with same sweep settings.")
        return [i - b for i, b in zip(run.i_ua, I_blank)]

    d18 = [(r, delta_i(r)) for r in runs_18]
    d24 = [(r, delta_i(r)) for r in runs_24]

    # Features (edit windows if you want)
    windows = [
        ("W1", -0.20, -0.05),
        ("W2", -0.45, -0.25),
        ("W3", -0.65, -0.45),
    ]

    def summarize(group_name: str, pairs):
        print(f"\n{group_name}")
        for (wname, e0, e1) in windows:
            vals_peak = [feature_peak_abs(E, dI, e0, e1) for _, dI in pairs]
            vals_area = [feature_area_abs(E, dI, e0, e1) for _, dI in pairs]
            m_p, s_p = mean_std(vals_peak)
            m_a, s_a = mean_std(vals_area)
            print(f"  {wname} [{e0:.2f},{e1:.2f}]  peak|ΔI|={m_p:.3f}±{s_p:.3f} µA  area|ΔI|={m_a:.3f}±{s_a:.3f} µA·V")

    summarize("18K vs blank", d18)
    summarize("24K vs blank", d24)

    if plt is None:
        print("\nmatplotlib not installed; cannot render plots.")
        return 0

    sigma = float(args.smooth_sigma_pts)
    plt.style.use("seaborn-v0_8-whitegrid")
    fig, axes = plt.subplots(2, 1, figsize=(11, 10), sharex=True)
    ax0, ax1 = axes

    ax0.set_title("LSV Current vs E(WE-RE) (overlay)")
    ax0.plot(E, I_blank, color="black", linewidth=2.0, label="Blank")
    cmap18 = plt.get_cmap("Blues")
    cmap24 = plt.get_cmap("Oranges")

    i18 = [gaussian_smooth(r.i_ua, sigma) for r in runs_18]
    i24 = [gaussian_smooth(r.i_ua, sigma) for r in runs_24]

    for i, (r, y) in enumerate(zip(runs_18, i18)):
        ax0.plot(r.e_we_re, y, color=cmap18(0.45 + 0.15 * i), alpha=0.85, label=r.label)
    for i, (r, y) in enumerate(zip(runs_24, i24)):
        ax0.plot(r.e_we_re, y, color=cmap24(0.45 + 0.15 * i), alpha=0.85, label=r.label)

    m18, s18 = compute_group_mean_std(i18)
    m24, s24 = compute_group_mean_std(i24)
    ax0.plot(E, m18, color="navy", linewidth=3.5, label="18K mean (smoothed)")
    ax0.plot(E, m24, color="darkorange", linewidth=3.5, label="24K mean (smoothed)")
    ax0.fill_between(E, [a - b for a, b in zip(m18, s18)], [a + b for a, b in zip(m18, s18)], color="navy", alpha=0.12)
    ax0.fill_between(E, [a - b for a, b in zip(m24, s24)], [a + b for a, b in zip(m24, s24)], color="darkorange", alpha=0.12)
    ax0.set_ylabel("I (µA)")
    ax0.legend(ncol=3, fontsize=9)

    ax1.set_title("Blank-subtracted ΔI = I(sample) − I(blank)")
    d18_s = [gaussian_smooth(dI, sigma) for _, dI in d18]
    d24_s = [gaussian_smooth(dI, sigma) for _, dI in d24]
    for i, (r, y) in enumerate(zip(runs_18, d18_s)):
        ax1.plot(E, y, color=cmap18(0.45 + 0.15 * i), alpha=0.85, label=f"{r.label} Δ")
    for i, (r, y) in enumerate(zip(runs_24, d24_s)):
        ax1.plot(E, y, color=cmap24(0.45 + 0.15 * i), alpha=0.85, label=f"{r.label} Δ")

    md18, sd18 = compute_group_mean_std(d18_s)
    md24, sd24 = compute_group_mean_std(d24_s)
    ax1.plot(E, md18, color="navy", linewidth=3.5, label="18K mean Δ (smoothed)")
    ax1.plot(E, md24, color="darkorange", linewidth=3.5, label="24K mean Δ (smoothed)")
    ax1.fill_between(E, [a - b for a, b in zip(md18, sd18)], [a + b for a, b in zip(md18, sd18)], color="navy", alpha=0.12)
    ax1.fill_between(E, [a - b for a, b in zip(md24, sd24)], [a + b for a, b in zip(md24, sd24)], color="darkorange", alpha=0.12)
    ax1.axhline(0.0, color="black", linewidth=1.0)
    ax1.set_xlabel("E (V)  (approx = −sweep_V)")
    ax1.set_ylabel("ΔI (µA)")
    ax1.legend(ncol=3, fontsize=9)

    fig.tight_layout()
    out_path = os.path.join(base, args.out)
    fig.savefig(out_path, dpi=180)
    print(f"\nSaved plot: {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


