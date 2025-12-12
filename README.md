# LSV (Linear Sweep Voltammetry) — Metallurgy + Minimal Electronics Context

![Screenshot](./images/Screenshot%202025-12-12%20at%205.17.37%20PM.png)

This folder contains **LSV runs** exported as CSV and supporting analysis/plots.

## What the chemistry/metallurgy is doing (the point of the scan)
LSV sweeps electrode potential and measures current. When a metal becomes thermodynamically able to **oxidize (dissolve)** at the working electrode, you see a **rise/peak in anodic current**.

- **Copper (Cu)**: \( \mathrm{Cu \rightarrow Cu^{2+} + 2e^-} \)
- **Silver (Ag)**: \( \mathrm{Ag \rightarrow Ag^{+} + e^-} \)
- **Gold (Au)** (often via oxide formation/complex pathways depending on electrolyte): simplified as
  \( \mathrm{Au \rightarrow Au^{3+} + 3e^-} \) (effective electron count depends on conditions)

**How to read the curve**
- **Peak/feature position (potential)**: helps identify *which* metal reaction dominates there.
- **Peak/feature size (charge = area under current vs potential/time)**: roughly relates to *how much* of that electroactive species is present (after baseline/blank subtraction and with consistent geometry + electrolyte).
- **Blank subtraction matters**: electrolyte/background reactions add current; subtracting a blank run isolates metal-driven features.

## What the electronics/firmware is doing (very short)
The LSV “potentiostat” side is just:
- **Generate a voltage sweep** (DAC sets/ramps the commanded potential).
- **Hold electrode potential** (control loop keeps the working/reference relationship near the target).
- **Measure current** with a **TIA (transimpedance amplifier)** + **ADC** and log it.

The CSVs in this folder are 7 columns, per row:
`sweep_V, dac_V, I_uA, adc_counts, adc_diff_V, sat_adc, sat_tia`

Common plotting choice used in this repo:
- \(E(\mathrm{WE-RE}) \approx -\texttt{sweep\_V}\)

## Hardware Photos

**PCB Board**

![PCB Board](./images/pcb%20.webp)

**LSV Circuit**

![LSV Circuit](./images/lsv%20circuit.jpg)


