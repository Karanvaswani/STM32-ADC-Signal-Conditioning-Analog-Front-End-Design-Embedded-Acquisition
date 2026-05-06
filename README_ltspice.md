# LTspice Simulation Guide

## Files

| File | Simulation Type | Purpose |
|------|----------------|---------|
| `ina128_amplifier.asc` | DC, Transient | Verify gain = 10, CMRR |
| `sallen_key_lpf.asc` | AC Analysis | Bode plot, verify fc = 500 Hz |
| `full_afe_chain.asc` | AC + Transient + Noise | Complete front-end verification |

---

## How to Run Each Simulation

### 1. AC Analysis (Bode Plot — Frequency Response)

1. Open `full_afe_chain.asc`
2. Click **Run** → Edit Simulation Command → **AC Analysis**
3. Settings:
   - Type of sweep: **Decade**
   - Points per decade: **100**
   - Start frequency: **1 Hz**
   - Stop frequency: **100 kHz**
4. Click OK → Run
5. Click on output node wire → select **V(out)**
6. Right-click plot → **Add Plot Pane** → plot **Phase(V(out))**

**Expected results:**
- Flat 0 dB from 1 Hz to ~400 Hz
- −3 dB at exactly 500 Hz
- −40 dB/decade rolloff beyond fc
- Phase = −90° at fc

---

### 2. Transient Analysis (Step Response)

1. Edit Simulation → **Transient**
2. Stop time: **10ms**, Time step: **1µs**
3. Change V_LM35 source to PULSE(0 2.5 1m 1n 1n 5m 10m)
4. Run → plot V(out)

**Expected results:**
- Rise time ~450 µs
- Overshoot < 1%
- Settling time < 2 ms

---

### 3. Noise Analysis

1. Edit Simulation → **Noise**
2. Output: `V(out)`
3. Input source: `V_LM35`
4. Frequency range: 1 Hz to 10 kHz, 100 pts/decade
5. Run → View → SPICE Error Log (shows integrated noise)

**Expected:**
- Input-referred noise < 200 nV RMS over 500 Hz bandwidth

---

## Exporting Data for Python Plotting

1. After AC simulation, File → **Export Data** → save as `ac_data.txt`
2. Run `../scripts/plot_bode.py` to generate publication-quality Bode plot

---

## Component Notes

| Component | Value | Reason |
|-----------|-------|--------|
| RG | 5.6 kΩ | Sets INA128 gain = 1 + 50k/5.6k = 9.93 ≈ 10 |
| R1, R2 | 10 kΩ | Sallen-Key equal-component design |
| C1, C2 | 33 nF | fc = 1/(2π×10k×33n) = 482 Hz ≈ 500 Hz |
| V_LM35 | SINE(0.25 0.01 50) | Simulates LM35 at 25°C with 50 Hz noise |
