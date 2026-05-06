"""
uart_capture.py
Captures live temperature data from STM32 over UART and plots in real-time.

Requirements:
    pip install pyserial matplotlib

Usage:
    python uart_capture.py --port COM3        (Windows)
    python uart_capture.py --port /dev/ttyUSB0  (Linux)

Author: Karan Kumar | MUET 2025
"""

import argparse
import re
import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# ── Try to import serial — gracefully handle missing dependency ──
try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False
    print("pyserial not installed. Running in DEMO mode (simulated data).")

# ── Config ─────────────────────────────────────────────────────
BAUD_RATE    = 115200
MAX_POINTS   = 120      # 2 minutes of 1 Hz data
ALERT_TEMP   = 40.0

# Regex to parse STM32 output:
# [T=12.000s] Temp: 24.87 C | ADC_raw: 1234 | V_in: 2.493V
PATTERN = re.compile(
    r'\[T=(\d+\.\d+)s\]\s+Temp:\s+([\d.]+)\s+C\s+\|\s+ADC_raw:\s+(\d+)\s+\|\s+V_in:\s+([\d.]+)V'
)

# ── Data buffers ───────────────────────────────────────────────
times        = deque(maxlen=MAX_POINTS)
temperatures = deque(maxlen=MAX_POINTS)
voltages     = deque(maxlen=MAX_POINTS)

# ── Matplotlib setup ───────────────────────────────────────────
fig, (ax_temp, ax_volt) = plt.subplots(2, 1, figsize=(12, 7), sharex=True)
fig.suptitle('STM32 Real-Time Temperature Acquisition\n'
             'LM35 + INA128 AFE + 12-bit ADC + DMA Averaging',
             fontsize=12, fontweight='bold')

line_temp, = ax_temp.plot([], [], 'b-o', markersize=3, linewidth=1.5, label='Temperature (°C)')
alert_line  = ax_temp.axhline(y=ALERT_TEMP, color='r', linestyle='--',
                               linewidth=1, label=f'Alert threshold ({ALERT_TEMP}°C)')
line_volt, = ax_volt.plot([], [], 'g-', linewidth=1.5, label='V_in (V)')

ax_temp.set_ylabel('Temperature (°C)', fontsize=10)
ax_temp.set_ylim(0, 60)
ax_temp.grid(True, alpha=0.3)
ax_temp.legend(fontsize=9)
ax_temp.set_title('Temperature', fontsize=10, loc='left')

ax_volt.set_ylabel('ADC Input Voltage (V)', fontsize=10)
ax_volt.set_xlabel('Time (s)', fontsize=10)
ax_volt.set_ylim(0, 3.3)
ax_volt.grid(True, alpha=0.3)
ax_volt.legend(fontsize=9)
ax_volt.axhline(y=3.3, color='orange', linestyle=':', linewidth=1, label='ADC ref 3.3V')

# Stats text box
stats_text = ax_temp.text(0.02, 0.95, '', transform=ax_temp.transAxes,
                           fontsize=9, verticalalignment='top',
                           bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

# ── Serial / Demo ──────────────────────────────────────────────
ser = None
demo_t = 0.0

def get_next_reading(port=None):
    """Read one line from serial or generate demo data."""
    global ser, demo_t

    if SERIAL_AVAILABLE and port and ser is None:
        try:
            ser = serial.Serial(port, BAUD_RATE, timeout=2)
            print(f"Connected to {port} @ {BAUD_RATE} baud")
        except Exception as e:
            print(f"Serial error: {e} — switching to demo mode")

    if ser and ser.is_open:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            m = PATTERN.search(line)
            if m:
                return float(m.group(1)), float(m.group(2)), int(m.group(3)), float(m.group(4))
        except Exception:
            pass
        return None

    # Demo mode: simulate 25°C ± noise with slow drift
    demo_t += 1.0
    import math, random
    temp  = 25.0 + 5.0 * math.sin(demo_t / 30.0) + random.gauss(0, 0.2)
    volt  = temp * 0.01 * 10   # reverse of ADC chain
    raw   = int((volt / 3.3) * 4095)
    return demo_t, round(temp, 2), raw, round(volt, 3)

def animate(frame):
    result = get_next_reading()
    if result is None:
        return line_temp, line_volt

    t, temp, raw, volt = result
    times.append(t)
    temperatures.append(temp)
    voltages.append(volt)

    line_temp.set_data(list(times), list(temperatures))
    line_volt.set_data(list(times), list(voltages))

    if times:
        ax_temp.set_xlim(max(0, times[-1] - MAX_POINTS), times[-1] + 2)
        ax_volt.set_xlim(max(0, times[-1] - MAX_POINTS), times[-1] + 2)

    # Update stats
    if len(temperatures) > 1:
        arr = list(temperatures)
        stats_text.set_text(
            f'Current: {temp:.2f}°C\n'
            f'Min: {min(arr):.2f}°C  Max: {max(arr):.2f}°C\n'
            f'Mean: {sum(arr)/len(arr):.2f}°C\n'
            f'ADC raw: {raw}  V_in: {volt:.3f}V'
        )

    # Colour point red if above threshold
    if temp > ALERT_TEMP:
        ax_temp.set_facecolor('#fff0f0')
    else:
        ax_temp.set_facecolor('white')

    return line_temp, line_volt, stats_text

# ── Main ────────────────────────────────────────────────────────
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='STM32 UART Temperature Monitor')
    parser.add_argument('--port', type=str, default=None,
                        help='Serial port (e.g. COM3 or /dev/ttyUSB0)')
    args = parser.parse_args()

    ani = animation.FuncAnimation(fig, animate, interval=1000,
                                  blit=False, cache_frame_data=False)
    plt.tight_layout()
    plt.show()

    if ser and ser.is_open:
        ser.close()
