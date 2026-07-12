"""ESP-IDF 串口监控脚本（基于 pyserial，替代 esp_idf_monitor）

用法：
  python monitor.py COM8                     # 监控 COM8 直到 Ctrl+C
  python monitor.py COM8 15                  # 监控 COM8 15 秒后自动退出
  python monitor.py COM8 15 | tee log.log    # 同时保存日志
"""
import serial
import sys
import time

if len(sys.argv) < 2:
    print("Usage: python monitor.py <PORT> [DURATION_SEC]")
    print("  DURATION_SEC omitted = run until Ctrl+C")
    sys.exit(1)

PORT = sys.argv[1]
BAUD = 115200
DURATION = int(sys.argv[2]) if len(sys.argv) >= 3 else None

try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
except Exception as e:
    print(f"[ERROR] Cannot open {PORT}: {e}")
    sys.exit(1)

if DURATION:
    print(f"[INFO] Listening on {PORT}@{BAUD} for {DURATION}s ... (Ctrl+C to stop early)")
else:
    print(f"[INFO] Listening on {PORT}@{BAUD} ... (Ctrl+C to stop)")

start = time.time()
try:
    while True:
        if DURATION and (time.time() - start) >= DURATION:
            break
        line = ser.readline()
        if line:
            try:
                print(line.decode("utf-8", errors="replace").rstrip())
            except Exception:
                print(f"[HEX] {line.hex()}")
except KeyboardInterrupt:
    print("\n[INFO] Stopped by user")
finally:
    ser.close()
    print(f"[INFO] Closed {PORT}")
