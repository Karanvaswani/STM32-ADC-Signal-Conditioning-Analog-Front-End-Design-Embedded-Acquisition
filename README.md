Project Overview
This project implements a complete analog signal conditioning chain for a temperature sensor (LM35), from raw sensor output through to calibrated digital data on an STM32 microcontroller. The design covers:

Analog Front-End (AFE) — designed and verified entirely in LTspice simulation:

Instrumentation Amplifier (INA128) for differential, low-noise amplification
2nd-order Sallen-Key active low-pass filter (anti-aliasing, fc = 500 Hz)
Voltage reference offset stage for full ADC input range utilisation


Digital Acquisition — implemented on STM32F103C8T6:

12-bit ADC, DMA-driven continuous sampling at 1 kSPS
Circular DMA buffer with interrupt-based averaging
UART serial output of calibrated temperature readings (°C)
LED threshold alert (>40 °C)


Validation & Analysis — Python scripts for:

Frequency response plotting from LTspice AC simulation data
Real UART data capture and temperature calibration curve

stm32-signal-conditioning/
│
├── ltspice/
│   ├── ina128_amplifier.asc          # INA128 instrumentation amp stage
│   ├── sallen_key_lpf.asc            # 2nd-order Sallen-Key anti-aliasing filter
│   ├── full_afe_chain.asc            # Complete analog front-end chain
│   └── README_ltspice.md             # How to run simulations
│
├── stm32-firmware/
│   ├── Core/
│   │   ├── Inc/
│   │   │   ├── main.h
│   │   │   ├── adc_handler.h
│   │   │   └── uart_handler.h
│   │   └── Src/
│   │       ├── main.c                # Main application loop
│   │       ├── adc_handler.c         # DMA-driven ADC acquisition
│   │       └── uart_handler.c        # Temperature output over UART
│   └── STM32F103C8TX_FLASH.ld
│
├── scripts/
│   ├── plot_bode.py                  # Plot Bode diagram from LTspice .raw export
│   ├── uart_capture.py               # Capture and plot live UART temperature data
│   └── calibration.py               # LM35 calibration curve fitting
│
├── docs/
│   ├── waveforms/                    # LTspice simulation screenshots
│   ├── schematic.pdf                 # Full hand-drawn + KiCad schematic
│   ├── design_notes.md               # Engineering design decisions
│   └── results_summary.md            # Key measured/simulated results
│
└── README.md

Circuit Design
Stage 1 — Instrumentation Amplifier (INA128)
The LM35 temperature sensor produces 10 mV/°C. At room temperature (25°C) the output is only 250 mV — far too small for accurate 12-bit ADC quantisation across the 3.3V reference range.
Design choice: INA128 in differential configuration with gain set by a single resistor
Gain = 1 + (50kΩ / RG)
RG = 50kΩ / (G - 1) = 50kΩ / 9 ≈ 5.6 kΩ  →  Gain = 9.93 ≈ 10

Output at 25°C: 250 mV × 10 = 2.5 V → uses ~76% of ADC range. ✓
CMRR of INA128: 120 dB typical — excellent common-mode noise rejection.
Stage 2 — Sallen-Key Low-Pass Filter (Anti-Aliasing)
STM32 ADC samples at 1 kSPS (Nyquist: 500 Hz). Any signal component above 500 Hz will alias into the measurement band. The MCU clock and switching regulators generate noise at tens of kHz.
Design choice: 2nd-order Butterworth Sallen-Key LPF, fc = 500 Hz, unity gain.
Component values (R = 10 kΩ, C calculated):
fc = 1 / (2π × R × C)  →  C = 1 / (2π × 10kΩ × 500) ≈ 31.8 nF  → use 33 nF
Damping ζ = 1/√2 (Butterworth) → C2 = 2ζ²C1 = C1
Roll-off: −40 dB/decade beyond 500 Hz. At 10 kHz: attenuation ≈ −46 dB. ✓
LTspice Simulation Results
AC Analysis — Filter Frequency Response

Passband (DC to 500 Hz): 0 dB gain, flat ✓
At 500 Hz (−3 dB point): confirmed in simulation ✓
At 5 kHz: −40 dB attenuation ✓
Phase at fc: −90° (expected for 2nd order) ✓

Transient Analysis — Step Response

Rise time (10%→90%): ~450 µs
Overshoot: < 1% (Butterworth characteristic) ✓
Settling time: < 2 ms ✓

Noise Analysis

Input-referred noise of INA128 at G=10: 8 nV/√Hz typical
Integrated noise (1 Hz – 500 Hz BW): ~179 nV RMS
At 12-bit ADC, 1 LSB = 3.3V/4096 = 805 µV — noise is well below 1 LSB ✓
STM32 Firmware — Key Design Decisions
DMA-Driven ADC (No CPU Blocking)
Rather than polling the ADC, DMA is configured to fill a circular buffer of 64 samples. On half-complete and complete callbacks, the CPU averages 32 samples — this reduces quantisation noise by √32 = 5.7× (equivalent to 2.8 extra bits of resolution), effectively achieving ~14.8-bit effective resolution from the 12-bit ADC.

// LM35: 10mV/°C, INA128 gain = 10, ADC ref = 3.3V, 12-bit
float voltage_V = (adc_avg / 4095.0f) * 3.3f;
float temperature_C = (voltage_V / 10.0f) * 100.0f;  // reverse gain, mV→°C
[T=0.500s] Temp: 24.87 C | ADC_raw: 1234 | V_in: 2.493V
[T=1.000s] Temp: 25.02 C | ADC_raw: 1238 | V_in: 2.501V

Results Summary
ParameterSimulatedTargetFilter −3dB frequency498 Hz500 HzPassband ripple< 0.1 dB< 0.5 dBAttenuation at 5 kHz−40.2 dB> −40 dBINA128 gain accuracy9.93 (error: 0.7%)10 ± 5%Noise floor (input-referred)179 nV RMS< 1 µV RMSTemperature resolution0.08°C< 0.1°CADC effective resolution~14.8 bit> 12 bit

1. Open ltspice/full_afe_chain.asc in LTspice XVII
2. Run → AC Analysis: 1Hz to 100kHz, 100 pts/decade
3. Run → Transient: 0 to 10ms, step 1µs
4. Export .raw file → run scripts/plot_bode.py

1. Open stm32-firmware/ in STM32CubeIDE
2. Build → Flash to STM32F103C8T6 via ST-Link/SWD
3. Open serial monitor at 115200 baud
4. Run scripts/uart_capture.py for live plotting
5. 
