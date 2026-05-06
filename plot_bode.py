"""
plot_bode.py
Generates a publication-quality Bode plot from LTspice AC simulation export.

Usage:
    1. In LTspice: run AC simulation on full_afe_chain.asc
    2. File > Export Data > save as 'ac_data.txt' (tab-separated)
    3. Run: python plot_bode.py

Author: Karan Kumar | MUET 2025
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

# ── Synthetic data (replace with real LTspice export) ──────────
# Format: frequency(Hz), magnitude(dB), phase(degrees)
# Generated to match expected Butterworth 2nd-order response

def butterworth_2nd_order(f, fc=500.0):
    """Transfer function magnitude (dB) for 2nd-order Butterworth LPF."""
    w  = f / fc
    H  = 1.0 / np.sqrt(1.0 + w**4)       # Butterworth |H(jw)|
    return 20 * np.log10(H)

def butterworth_phase(f, fc=500.0):
    """Phase response (degrees) for 2nd-order Butterworth LPF."""
    w     = 2 * np.pi * f
    wc    = 2 * np.pi * fc
    phase = -np.degrees(np.arctan2(2 * (w/wc), 1 - (w/wc)**2))
    return phase

freqs     = np.logspace(0, 5, 500)   # 1 Hz to 100 kHz
magnitude = butterworth_2nd_order(freqs)
phase     = butterworth_phase(freqs)

# ── If LTspice export exists, load it instead ──────────────────
ltspice_file = "ac_data.txt"
if os.path.exists(ltspice_file):
    data      = np.loadtxt(ltspice_file, skiprows=1)
    freqs     = data[:, 0]
    magnitude = data[:, 1]
    phase     = data[:, 2]
    print(f"Loaded LTspice data: {len(freqs)} points")
else:
    print("Using synthetic Butterworth response (no LTspice export found)")

# ── Plot ───────────────────────────────────────────────────────
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
fig.suptitle("Sallen-Key 2nd-Order Butterworth Low-Pass Filter\n"
             "Analog Front-End for STM32 Temperature Acquisition",
             fontsize=13, fontweight='bold', y=0.98)

# Magnitude plot
ax1.semilogx(freqs, magnitude, 'b-', linewidth=2, label='|H(f)| dB')
ax1.axvline(x=500, color='r', linestyle='--', linewidth=1.2, label='fc = 500 Hz')
ax1.axhline(y=-3, color='gray', linestyle=':', linewidth=1, label='−3 dB')
ax1.axhline(y=-40, color='orange', linestyle=':', linewidth=1, label='−40 dB')

# Annotate -3dB point
ax1.annotate('−3 dB @ 500 Hz', xy=(500, -3), xytext=(100, -15),
             arrowprops=dict(arrowstyle='->', color='red'),
             fontsize=9, color='red')

ax1.set_ylabel('Magnitude (dB)', fontsize=11)
ax1.set_ylim(-80, 5)
ax1.set_yticks([0, -3, -10, -20, -40, -60, -80])
ax1.grid(True, which='both', alpha=0.3)
ax1.legend(fontsize=9, loc='lower left')
ax1.set_title('Magnitude Response', fontsize=10, loc='left', pad=4)

# Phase plot
ax2.semilogx(freqs, phase, 'g-', linewidth=2, label='Phase')
ax2.axvline(x=500, color='r', linestyle='--', linewidth=1.2)
ax2.axhline(y=-90, color='gray', linestyle=':', linewidth=1, label='−90° at fc')
ax2.annotate('−90° @ fc', xy=(500, -90), xytext=(50, -110),
             arrowprops=dict(arrowstyle='->', color='green'),
             fontsize=9, color='green')

ax2.set_xlabel('Frequency (Hz)', fontsize=11)
ax2.set_ylabel('Phase (degrees)', fontsize=11)
ax2.set_ylim(-200, 10)
ax2.set_yticks([0, -45, -90, -135, -180])
ax2.grid(True, which='both', alpha=0.3)
ax2.legend(fontsize=9, loc='lower left')
ax2.set_title('Phase Response', fontsize=10, loc='left', pad=4)

ax2.xaxis.set_major_formatter(ticker.FuncFormatter(
    lambda x, _: f'{int(x):,} Hz' if x < 1000 else f'{x/1000:.0f} kHz'))

plt.tight_layout()
plt.savefig('../docs/waveforms/bode_plot.png', dpi=150, bbox_inches='tight')
print("Saved: docs/waveforms/bode_plot.png")
plt.show()

# ── Print key values ───────────────────────────────────────────
fc_idx = np.argmin(np.abs(freqs - 500))
print(f"\nKey simulation results:")
print(f"  Magnitude at 100 Hz :  {magnitude[np.argmin(np.abs(freqs-100))]:+.2f} dB")
print(f"  Magnitude at 500 Hz :  {magnitude[fc_idx]:+.2f} dB  (target: -3.0 dB)")
print(f"  Magnitude at 5 kHz  :  {magnitude[np.argmin(np.abs(freqs-5000))]:+.2f} dB  (target: ~-40 dB)")
print(f"  Phase    at 500 Hz  :  {phase[fc_idx]:+.1f}°  (expected: -90°)")
