"""Save first N frames from ESP32P4 stream to disk for verification."""
import socket, struct, sys

ESP_IP   = "192.168.28.168"
ESP_PORT = 8888
PC_PORT  = 9999
HDR_FMT  = "<HHHHH"
HDR_SIZE = struct.calcsize(HDR_FMT)

NUM_FRAMES = 10
out_dir = "D:/Program/MekCraft-Labs/embedded-lab/multi-arch/ScanningCar/CameraCtrl/frames"

import os
os.makedirs(out_dir, exist_ok=True)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", PC_PORT))
sock.settimeout(10.0)

sock.sendto(b"START", (ESP_IP, ESP_PORT))
print(f"[INFO] Sent START to {ESP_IP}:{ESP_PORT}")

frames = {}
saved = 0

while saved < NUM_FRAMES:
    try:
        data, addr = sock.recvfrom(4096)
    except socket.timeout:
        print("[WARN] Timeout, re-sending START...")
        sock.sendto(b"START", (ESP_IP, ESP_PORT))
        continue

    if len(data) < HDR_SIZE:
        continue

    magic, fid, cidx, total, plen = struct.unpack_from(HDR_FMT, data)
    if magic != 0xA55A:
        continue

    payload = data[HDR_SIZE:HDR_SIZE + plen]
    if fid not in frames:
        frames[fid] = {}
    frames[fid][cidx] = payload

    if len(frames[fid]) == total:
        jpeg = b"".join(frames[fid][i] for i in range(total))
        del frames[fid]
        path = os.path.join(out_dir, f"frame_{saved:04d}.jpg")
        with open(path, "wb") as f:
            f.write(jpeg)
        print(f"[OK] Frame {fid}: {len(jpeg)} bytes -> {path}")
        saved += 1

    if len(frames) > 20:
        oldest = min(frames.keys())
        del frames[oldest]

print(f"[DONE] Saved {saved} frames to {out_dir}")
sock.close()
