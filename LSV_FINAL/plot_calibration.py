import argparse
import csv
import glob
import os
from dataclasses import dataclass
from typing import List, Tuple, Optional

# This script *prefers* matplotlib for plots, but can run without extra deps:
# - If matplotlib is missing, it will still write merged CSV + print features.
# - If numpy is missing, it will fall back to pure-Python lists.

try:
    import matplotlib.pyplot as plt  # type: ignore
except Exception:  # pragma: no cover
    plt = None

try:
    import numpy as _np  # type: ignore
except Exception:  # pragma: no cover
    _np = None


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
        # With the current firmware wiring: V_RE ~ DAC, V_WE ~ VREF (CELL_BIAS_V),
        # so E(WE-RE) ≈ V_WE - V_RE ≈ -sweep_V.
        return [-x for x in self.sweep_v]


def read_run(path: str, label: str) -> Run:
    rows = []
    with open(path, "r", newline="") as f:
        reader = csv.reader(f)
        for r in reader:
            if not r:
                continue
            # allow comment lines like "#SAT,..."
            if r[0].startswith("#"):
                continue
            if len(r) != 7:
                raise ValueError(f"{path}: expected 7 columns, got {len(r)}: {r}")
            rows.append(r)

    sweep_v: List[float] = []
    dac_v: List[float] = []
    i_ua: List[float] = []
    diff_counts: List[int] = []
    diff_v: List[float] = []
    sat_adc: List[int] = []
    sat_tia: List[int] = []

    for rr in rows:
        sweep_v.append(float(rr[0]))
        dac_v.append(float(rr[1]))
        i_ua.append(float(rr[2]))
        diff_counts.append(int(float(rr[3])))
        diff_v.append(float(rr[4]))
        sat_adc.append(int(float(rr[5])))
        sat_tia.append(int(float(rr[6])))

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


def interp_to(x_src: List[float], y_src: List[float], x_dst: List[float]) -> List[float]:
    # Linear interpolation. Assumes x_src monotonic increasing and x_dst within bounds.
    out: List[float] = []
    j = 0
    n = len(x_src)
    for x in x_dst:
        while j + 1 < n and x_src[j + 1] < x:
            j += 1
        if j + 1 >= n:
            out.append(y_src[-1])
            continue
        x0, x1 = x_src[j], x_src[j + 1]
        y0, y1 = y_src[j], y_src[j + 1]
        if x1 == x0:
            out.append(y0)
        else:
            t = (x - x0) / (x1 - x0)
            out.append(y0 + t * (y1 - y0))
    return out


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


def mean_std(vals: List[float]) -> Tuple[float, float]:
    v = [x for x in vals if x == x]  # drop NaN
    if not v:
        return float("nan"), float("nan")
    m = sum(v) / len(v)
    var = sum((x - m) ** 2 for x in v) / len(v)
    return m, var ** 0.5


def gaussian_smooth(y: List[float], sigma_pts: float) -> List[float]:
    """
    Gaussian smoothing with sigma expressed in samples (points).
    Uses numpy if available; otherwise pure-python convolution.
    """
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
        pad = radius
        ypad = _np.pad(yy, (pad, pad), mode="edge")
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


def main() -> int:
    p = argparse.ArgumentParser(description="Plot and calibrate LSV runs (blank subtraction + overlays).")
    p.add_argument("--dir", default=".", help="Directory containing CSV files (default: .)")
    p.add_argument("--blank", default="blank.csv", help="Blank CSV filename (default: blank.csv)")
    p.add_argument("--k18", default="18_*.csv", help="Glob for 18K runs (default: 18_*.csv)")
    p.add_argument("--k24", default="24_*.csv", help="Glob for 24K runs (default: 24_*.csv)")
    p.add_argument("--out", default="calibration_plots.png", help="Output image file (default: calibration_plots.png)")
    p.add_argument("--merged", default="calibration_merged.csv", help="Merged CSV output (default: calibration_merged.csv)")
    p.add_argument("--smooth-sigma-pts", type=float, default=0.0, help="Gaussian smoothing sigma in samples (default: 0 = off). Try 2-5.")
    p.add_argument("--xaxis", choices=["E", "sweep", "dac"], default="E", help="X axis for plots: E=-sweep (default), sweep_V, or dac_V")
    args = p.parse_args()

    base = os.path.abspath(args.dir)
    blank_path = os.path.join(base, args.blank)
    k18_paths = sorted(glob.glob(os.path.join(base, args.k18)))
    k24_paths = sorted(glob.glob(os.path.join(base, args.k24)))

    if not os.path.exists(blank_path):
        raise SystemExit(f"Blank file not found: {blank_path}")
    if not k18_paths:
        raise SystemExit(f"No 18K files matched: {args.k18} in {base}")
    if not k24_paths:
        raise SystemExit(f"No 24K files matched: {args.k24} in {base}")

    blank = read_run(blank_path, "blank")
    runs_18 = [read_run(pth, f"18K {i+1}") for i, pth in enumerate(k18_paths)]
    runs_24 = [read_run(pth, f"24K {i+1}") for i, pth in enumerate(k24_paths)]

    # Common grid from blank (assumes all runs share same sweep steps)
    E = blank.e_we_re
    sweep = blank.sweep_v
    dac = blank.dac_v
    I_blank = blank.i_ua

    if args.xaxis == "E":
        X = E
        x_label = "E (V)  (approx = −sweep_V)"
        x_key = "E_V"
    elif args.xaxis == "sweep":
        X = sweep
        x_label = "sweep_V (V)"
        x_key = "sweep_V"
    else:
        X = dac
        x_label = "dac_V (V)"
        x_key = "dac_V"

    # Build ΔI runs on the common grid
    def delta_i(run: Run) -> List[float]:
        I = run.i_ua
        e_run = run.e_we_re
        if len(e_run) != len(E) or any(abs(a - b) > 1e-9 for a, b in zip(e_run, E)):
            I = interp_to(e_run, I, E)
        return [i - b for i, b in zip(I, I_blank)]

    d18 = [(r, delta_i(r)) for r in runs_18]
    d24 = [(r, delta_i(r)) for r in runs_24]

    # Always write a merged CSV (works without extra deps; open in Excel/Sheets if needed)
    merged_path = os.path.join(base, args.merged)
    with open(merged_path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow([x_key, "I_blank_uA"] + [r.label.replace(" ", "_") for r in runs_18 + runs_24])
        for idx, x in enumerate(X):
            row = [x, I_blank[idx]]
            for r in runs_18 + runs_24:
                if len(r.i_ua) == len(X):
                    row.append(r.i_ua[idx])
                else:
                    # fallback: interpolate using E grid (best-effort)
                    row.append(interp_to(r.e_we_re, r.i_ua, E)[idx])
            w.writerow(row)
    print(f"Wrote merged CSV: {merged_path}")

    # Plot if matplotlib is available
    if plt is not None:
        sigma_pts = float(args.smooth_sigma_pts)
        plt.style.use("seaborn-v0_8-whitegrid")
        fig, axes = plt.subplots(2, 1, figsize=(11, 10), sharex=True)

        ax0, ax1 = axes
        ax0.set_title("LSV Current overlay")
        ax0.plot(X, I_blank, color="black", linewidth=2.0, label="Blank")

        cmap18 = plt.get_cmap("Blues")
        cmap24 = plt.get_cmap("Oranges")

        i18_curves: List[List[float]] = []
        i24_curves: List[List[float]] = []

        for i, r in enumerate(runs_18):
            y = gaussian_smooth(r.i_ua, sigma_pts)
            i18_curves.append(y)
            ax0.plot(X, y, color=cmap18(0.45 + 0.15 * i), alpha=0.85, label=r.label)
        for i, r in enumerate(runs_24):
            y = gaussian_smooth(r.i_ua, sigma_pts)
            i24_curves.append(y)
            ax0.plot(X, y, color=cmap24(0.45 + 0.15 * i), alpha=0.85, label=r.label)

        # Big smoothed mean curves (thick) + std band
        m18, s18 = compute_group_mean_std(i18_curves)
        m24, s24 = compute_group_mean_std(i24_curves)
        if m18:
            ax0.plot(X, m18, color="navy", linewidth=3.5, label="18K mean (smoothed)")
            ax0.fill_between(X, [a - b for a, b in zip(m18, s18)], [a + b for a, b in zip(m18, s18)],
                             color="navy", alpha=0.12)
        if m24:
            ax0.plot(X, m24, color="darkorange", linewidth=3.5, label="24K mean (smoothed)")
            ax0.fill_between(X, [a - b for a, b in zip(m24, s24)], [a + b for a, b in zip(m24, s24)],
                             color="darkorange", alpha=0.12)

        ax0.set_ylabel("I (µA)")
        ax0.legend(ncol=3, fontsize=9)

        ax1.set_title("Blank-subtracted ΔI = I(sample) − I(blank)")
        d18_curves: List[List[float]] = []
        d24_curves: List[List[float]] = []

        for i, (r, dI) in enumerate(d18):
            y = gaussian_smooth(dI, sigma_pts)
            d18_curves.append(y)
            ax1.plot(X, y, color=cmap18(0.45 + 0.15 * i), alpha=0.85, label=f"{r.label} Δ")
        for i, (r, dI) in enumerate(d24):
            y = gaussian_smooth(dI, sigma_pts)
            d24_curves.append(y)
            ax1.plot(X, y, color=cmap24(0.45 + 0.15 * i), alpha=0.85, label=f"{r.label} Δ")

        md18, sd18 = compute_group_mean_std(d18_curves)
        md24, sd24 = compute_group_mean_std(d24_curves)
        if md18:
            ax1.plot(X, md18, color="navy", linewidth=3.5, label="18K mean Δ (smoothed)")
            ax1.fill_between(X, [a - b for a, b in zip(md18, sd18)], [a + b for a, b in zip(md18, sd18)],
                             color="navy", alpha=0.12)
        if md24:
            ax1.plot(X, md24, color="darkorange", linewidth=3.5, label="24K mean Δ (smoothed)")
            ax1.fill_between(X, [a - b for a, b in zip(md24, sd24)], [a + b for a, b in zip(md24, sd24)],
                             color="darkorange", alpha=0.12)

        ax1.axhline(0.0, color="black", linewidth=1.0)
        ax1.set_xlabel(x_label)
        ax1.set_ylabel("ΔI (µA)")
        ax1.legend(ncol=3, fontsize=9)

        fig.tight_layout()
        out_path = os.path.join(base, args.out)
        fig.savefig(out_path, dpi=180)
        print(f"Saved plot: {out_path}")
    else:
        print("matplotlib not installed; skipping plot generation.")
        print("Install with: python3 -m pip install matplotlib")

    # Simple numeric features to judge separation (edit windows as needed)
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

    return 0


if __name__ == "__main__":
    raise SystemExit(main())


