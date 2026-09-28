#!/usr/bin/env python3
"""STM32 -> Luckfox -> 电脑(Wi-Fi/TCP) 串口转TCP转发脚本"""
import sys
import socket
import serial

SERIAL_DEV = "/dev/ttyS3"   # Luckfox Pico Ultra W 的 UART3
BAUD       = 115200          # 要和 STM32 端一致
PORT       = 9000            # 电脑连接的 TCP 端口

def main():
    dev  = sys.argv[1] if len(sys.argv) > 1 else SERIAL_DEV
    port = int(sys.argv[2]) if len(sys.argv) > 2 else PORT

    try:
        ser = serial.Serial(dev, BAUD, timeout=1)
    except serial.SerialException as e:
        print(f"[!] 打开串口失败: {dev} ({e})")
        print("    请检查: 1) 设备是否存在 (ls /dev/ttyS*)  2) UART3 是否已开启")
        sys.exit(1)

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("0.0.0.0", port))
    srv.listen(1)
    print(f"[*] 串口 {dev} @ {BAUD} -> TCP :{port} 就绪，等待电脑连接...")

    while True:
        conn, addr = srv.accept()
        print(f"[+] 客户端已连接: {addr}")
        try:
            while True:
                line = ser.readline()   # 读到 \n 结尾的一整行
                if line:
                    conn.sendall(line)
        except (BrokenPipeError, ConnectionResetError, OSError) as e:
            print(f"[-] 客户端断开: {e}")
        finally:
            conn.close()
            print("[*] 回到等待连接状态...")

if __name__ == "__main__":
    main()