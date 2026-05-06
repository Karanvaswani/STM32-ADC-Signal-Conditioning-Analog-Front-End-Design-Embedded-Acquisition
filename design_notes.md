# Design Notes — STM32 ADC Signal Conditioning Project

**Author:** Karan Kumar | MUET Jamshoro | 2025

---

## Design Decision Log

### Why INA128 over a simple op-amp?

A basic inverting or non-inverting op-amp amplifier is single-ended — it amplifies the signal relative to ground. The LM35 in a real installation has wires of non-zero length, which pick up common-mode interference (50 Hz mains hum, motor switching noise). A single-ended amplifier passes this noise straight to the ADC.

The INA128 is a three-op-amp instrumentation amplifier with differential inputs. It amplifies only the *difference* between V+ and V−, rejecting any noise common to both lines. Its CMRR of 120 dB means common-mode noise is attenuated by a factor of 1,000,000 — effectively eliminated.

**Key gain equation:**
```
G = 1 + (50kΩ / RG)
RG selected = 5.6 kΩ  →  G = 1 + (50k/5.6k) = 9.93 ≈ 10
```
Standard E24 resistor value. 1% tolerance resistors used to maintain gain accuracy.

---

### Why Sallen-Key over a passive RC filter?

A simple RC low-pass filter has a first-order roll-off of only −20 dB/decade. At 10× the cutoff frequency (5 kHz), it attenuates by only −20 dB — not enough to prevent MCU switching noise from aliasing.

The Sallen-Key is an active second-order topology using a single op-amp. It achieves −40 dB/decade roll-off with:
- No signal loading (high input impedance, low output impedance)
- Exact Butterworth (maximally flat) response with ζ = 1/√2
- Unity gain — no additional amplification error

The equal-component design simplifies selection:
```
R1 = R2 = 10 kΩ  (standard E24)
C1 = C2 = 33 nF  (standard value, closest to calculated 31.8 nF)
Actual fc = 1/(2π × 10k × 33n) = 482 Hz  (−3.6% from target — acceptable)
```

---

### Why 64-sample DMA buffer with 32-sample averaging?

The STM32F103 ADC has an ENOB (Effective Number of Bits) of approximately 11.5 bits due to internal noise. To improve this without external hardware:

**Oversampling theory:**
- Averaging N samples reduces noise by √N
- Averaging 32 samples: noise reduction = √32 = 5.66×
- Extra bits gained: log₂(√32) = 2.5 bits
- Effective resolution: 12 + 2.5 = **14.5 bits** ≈ 14.8 bits with calibration

The 64-element circular DMA buffer (processed in two 32-sample halves) allows continuous averaging without ever stopping the ADC — the CPU is only involved for 32 additions every ~32 ms.

---

### Why 239.5 cycle ADC sampling time?

The STM32 ADC offers sampling time options from 1.5 to 239.5 ADC clock cycles. Longer sampling time:
1. Allows the ADC's internal S/H capacitor to fully charge from high-impedance sources
2. Reduces thermal noise (more charge averaging in the capacitor)
3. Reduces bandwidth of the ADC input (additional low-pass effect)

At INA128 output impedance (~50 Ω) and 239.5 cycles @ 12 MHz ADC clock, the charge time constant is well within the sampling window. The throughput loss is acceptable since we only need 1 kSPS.

---

## Error Budget Analysis

| Error Source | Magnitude | Mitigation |
|---|---|---|
| LM35 accuracy | ±0.5°C (typical) | Calibration curve |
| INA128 gain error (1% RG) | ±1% of reading | Trim via software offset |
| ADC DNL/INL | ±1 LSB = 0.08°C | Averaging |
| ADC thermal noise | ~0.5 LSB | Averaging reduces to ~0.09 LSB |
| Filter component tolerance (5%) | ±5% of fc | Acceptable for anti-aliasing |
| Quantisation noise | 0.5 LSB = 0.04°C | Averaging |

**Total expected accuracy:** ±0.6°C (dominated by LM35 sensor tolerance)

---

## Possible Extensions (Future Work)

1. **Two-point calibration** — use ice water (0°C) and boiling water (100°C) to fit a linear correction: `T_corrected = a × T_raw + b`
2. **Multiple sensor channels** — STM32 ADC1 has 10 channels; add scan mode for 4× LM35 sensors on PA0–PA3
3. **SD card logging** — add SPI SD card for standalone data logging
4. **OLED display** — add I2C SSD1306 display for local readout
5. **Hardware anti-aliasing upgrade** — replace op-amp with rail-to-rail type (e.g. MCP6002) for 3.3V single-supply operation
6. **PCB design** — transfer simulation to KiCad schematic → PCB layout (natural extension of the STM32 PCB project)
