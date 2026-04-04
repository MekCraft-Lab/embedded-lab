"""
ESP32P4 CameraCtrl — PC端双向通信脚本

功能:
  - 接收 ESP32P4 的图像帧 + 遥测数据 (UWB距离, IMU角度)
  - 向 ESP32P4 发送控制指令 (轮速, 云台角速度)
  - 实时显示视频画面和遥测数据

使用方法:
  1. 查看串口日志获取 ESP32P4 的 IP 地址
  2. 修改下方 ESP_IP 为实际地址
  3. pip install opencv-python numpy
  4. python pc_receiver.py
"""

import socket
import struct
import threading
import time
import cv2
import numpy as np

ESP_IP   = "192.168.28.195"
ESP_PORT = 8888        # ESP image/telemetry port
PC_PORT  = 9999        # local port for receiving from ESP
CMD_PORT = 8889        # ESP command receive port

# Packet magics
MAGIC_IMG  = 0xA55A
MAGIC_TELE = 0xB66B
MAGIC_CMD  = 0xC77C

# Header formats
IMG_HDR_FMT  = "<HHHHH"     # magic, frame_id, chunk_idx, total_chunks, payload_len
IMG_HDR_SIZE = struct.calcsize(IMG_HDR_FMT)  # 10 bytes

TELE_FMT  = "<HH 4f f f I"  # magic, frame_id, uwb_d[4], yaw, pitch, timestamp_ms
TELE_SIZE = struct.calcsize(TELE_FMT)        # 28 bytes

CMD_FMT   = "<HH 4f f f"    # magic, seq, wheel_w[4], pitch_rate, yaw_rate
CMD_SIZE  = struct.calcsize(CMD_FMT)         # 28 bytes

# Shared state
latest_telemetry = {
    "uwb_d": [0.0] * 4,
    "yaw": 0.0,
    "pitch": 0.0,
    "timestamp_ms": 0,
}
latest_cmd = {
    "wheel_w": [0.0] * 4,
    "pitch_rate": 0.0,
    "yaw_rate": 0.0,
}
cmd_seq = 0
running = True


def cmd_sender(sock, esp_addr):
    """Thread: send command packets to ESP at ~30Hz."""
    global cmd_seq, running
    while running:
        cmd_seq = (cmd_seq + 1) % 65536
        data = struct.pack(CMD_FMT,
                           MAGIC_CMD, cmd_seq,
                           latest_cmd["wheel_w"][0], latest_cmd["wheel_w"][1],
                           latest_cmd["wheel_w"][2], latest_cmd["wheel_w"][3],
                           latest_cmd["pitch_rate"],
                           latest_cmd["yaw_rate"])
        try:
            sock.sendto(data, esp_addr)
        except Exception:
            pass
        time.sleep(1.0 / 30)


def main():
    global running

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", PC_PORT))
    sock.settimeout(5.0)

    # Windows: ignore ICMP Port Unreachable -> avoid WinError 10054 on UDP recvfrom
    if hasattr(socket, "SIO_UDP_CONNRESET"):
        sock.ioctl(socket.SIO_UDP_CONNRESET, struct.pack("I", 0))

    esp_addr = (ESP_IP, CMD_PORT)

    # Send start signal
    sock.sendto(b"START", (ESP_IP, ESP_PORT))
    print(f"[INFO] Sent START to {ESP_IP}:{ESP_PORT}")

    # Start command sender thread
    cmd_thread = threading.Thread(target=cmd_sender, args=(sock, esp_addr), daemon=True)
    cmd_thread.start()

    frames = {}       # frame_id -> {chunk_idx: bytes}
    last_displayed = -1
    cmd_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    while running:
        try:
            data, addr = sock.recvfrom(4096)
        except ConnectionResetError:
            # Remote may reboot during flashing; keep receiver alive.
            continue
        except socket.timeout:
            print("[WARN] No data, re-sending START...")
            sock.sendto(b"START", (ESP_IP, ESP_PORT))
            continue

        # --- Telemetry packet ---
        if len(data) >= TELE_SIZE:
            magic = struct.unpack_from("<H", data, 0)[0]
            if magic == MAGIC_TELE:
                vals = struct.unpack_from(TELE_FMT, data)
                frame_id = vals[1]
                latest_telemetry["uwb_d"] = list(vals[2:6])
                latest_telemetry["yaw"] = vals[6]
                latest_telemetry["pitch"] = vals[7]
                latest_telemetry["timestamp_ms"] = vals[8]
                if frame_id % 30 == 0:
                    print(f"[TELE] fid={frame_id} uwb={[f'{d:.2f}' for d in latest_telemetry['uwb_d']]} "
                          f"yaw={latest_telemetry['yaw']:.1f} pitch={latest_telemetry['pitch']:.1f}")
                continue

        # --- Image chunk packet ---
        if len(data) < IMG_HDR_SIZE:
            continue

        magic, fid, cidx, total, plen = struct.unpack_from(IMG_HDR_FMT, data)
        if magic != MAGIC_IMG:
            continue

        payload = data[IMG_HDR_SIZE:IMG_HDR_SIZE + plen]

        if fid not in frames:
            frames[fid] = {}
        frames[fid][cidx] = payload

        # Frame complete -> decode and display
        if len(frames[fid]) == total:
            jpeg_data = b"".join(frames[fid][i] for i in range(total))
            del frames[fid]

            img_array = np.frombuffer(jpeg_data, dtype=np.uint8)
            img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
            if img is not None:
                # Overlay telemetry text
                h, w = img.shape[:2]
                info_lines = [
                    f"Frame: {fid}",
                    f"UWB: [{latest_telemetry['uwb_d'][0]:.2f}, {latest_telemetry['uwb_d'][1]:.2f}, "
                    f"{latest_telemetry['uwb_d'][2]:.2f}, {latest_telemetry['uwb_d'][3]:.2f}] m",
                    f"Yaw: {latest_telemetry['yaw']:.1f} deg  Pitch: {latest_telemetry['pitch']:.1f} deg",
                    f"CMD: w=[{latest_cmd['wheel_w'][0]:.1f},{latest_cmd['wheel_w'][1]:.1f},"
                    f"{latest_cmd['wheel_w'][2]:.1f},{latest_cmd['wheel_w'][3]:.1f}]",
                ]
                for i, line in enumerate(info_lines):
                    cv2.putText(img, line, (5, 18 + i * 20),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.42, (0, 255, 0), 1, cv2.LINE_AA)

                cv2.imshow("ESP32P4 Stream", img)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
                if fid != last_displayed + 1 and last_displayed >= 0:
                    print(f"[WARN] Frame drop: expected {last_displayed+1}, got {fid}")
                last_displayed = fid

        # Cleanup old incomplete frames
        if len(frames) > 10:
            oldest = min(frames.keys())
            del frames[oldest]

    running = False
    cv2.destroyAllWindows()
    sock.close()


if __name__ == "__main__":
    main()
