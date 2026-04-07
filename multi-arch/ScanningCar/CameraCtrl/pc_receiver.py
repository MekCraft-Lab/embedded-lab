"""
PC receiver for CameraCtrl UDP JPEG stream.

Protocol from current firmware (`video_packet_header_t`):
- uint32 magic     = 0x4A504547 ("JPEG")
- uint32 frame_id
- uint32 total_len
- uint32 offset
- payload bytes
"""

import argparse
import os
import socket
import struct
import time

import cv2
import numpy as np

MAGIC = 0x4A504547
HDR_FMT = "<IIII"
HDR_SIZE = struct.calcsize(HDR_FMT)


class PendingFrame:
    def __init__(self, total_len: int) -> None:
        self.total_len = total_len
        self.chunks = {}
        self.received = 0
        self.last_update = time.time()

    def add_chunk(self, offset: int, payload: bytes) -> None:
        if offset in self.chunks:
            return
        end = offset + len(payload)
        if offset < 0 or end > self.total_len:
            return
        self.chunks[offset] = payload
        self.received += len(payload)
        self.last_update = time.time()

    def is_complete(self) -> bool:
        return self.received >= self.total_len

    def to_bytes(self) -> bytes:
        out = bytearray(self.total_len)
        written = 0
        for off in sorted(self.chunks.keys()):
            chunk = self.chunks[off]
            out[off: off + len(chunk)] = chunk
            written += len(chunk)
        if written < self.total_len:
            raise ValueError("incomplete frame")
        return bytes(out)


def main() -> None:
    parser = argparse.ArgumentParser(description="Receive UDP JPEG stream from ESP32-P4")
    parser.add_argument("--listen-ip", default="0.0.0.0")
    parser.add_argument("--listen-port", type=int, default=5000)
    parser.add_argument("--save-dir", default="")
    parser.add_argument("--no-display", action="store_true")
    args = parser.parse_args()

    if args.save_dir:
        os.makedirs(args.save_dir, exist_ok=True)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.listen_ip, args.listen_port))
    sock.settimeout(1.0)

    # Windows: suppress UDP ICMP reset issues.
    if hasattr(socket, "SIO_UDP_CONNRESET"):
        sock.ioctl(socket.SIO_UDP_CONNRESET, struct.pack("I", 0))

    pending = {}
    frames_ok = 0
    last_fid = -1
    stat_t0 = time.time()

    print(f"[INFO] Listening on {args.listen_ip}:{args.listen_port}")

    while True:
        try:
            packet, _ = sock.recvfrom(2048)
        except socket.timeout:
            now = time.time()
            stale = [fid for fid, fr in pending.items() if now - fr.last_update > 2.0]
            for fid in stale:
                del pending[fid]
            continue

        if len(packet) < HDR_SIZE:
            continue

        magic, fid, total_len, offset = struct.unpack_from(HDR_FMT, packet, 0)
        if magic != MAGIC:
            continue

        payload = packet[HDR_SIZE:]
        if total_len == 0 or not payload:
            continue

        fr = pending.get(fid)
        if fr is None or fr.total_len != total_len:
            fr = PendingFrame(total_len)
            pending[fid] = fr

        fr.add_chunk(offset, payload)

        if not fr.is_complete():
            continue

        try:
            jpeg = fr.to_bytes()
        except ValueError:
            del pending[fid]
            continue

        del pending[fid]
        frames_ok += 1

        npbuf = np.frombuffer(jpeg, dtype=np.uint8)
        img = cv2.imdecode(npbuf, cv2.IMREAD_UNCHANGED)
        if img is None:
            print(f"[WARN] decode failed, fid={fid}, len={len(jpeg)}")
            continue

        if args.save_dir:
            out_path = os.path.join(args.save_dir, f"frame_{fid:06d}.jpg")
            with open(out_path, "wb") as f:
                f.write(jpeg)

        if not args.no_display:
            now = time.time()
            fps = frames_ok / max(now - stat_t0, 1e-6)
            cv2.putText(img, f"fid={fid} fps={fps:.1f}", (8, 24),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 1, cv2.LINE_AA)
            cv2.imshow("CameraCtrl Stream", img)
            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

        if last_fid >= 0 and fid != last_fid + 1:
            print(f"[WARN] frame jump: expected {last_fid + 1}, got {fid}")
        last_fid = fid

        if frames_ok % 30 == 0:
            elapsed = time.time() - stat_t0
            print(f"[INFO] frames={frames_ok}, avg_fps={frames_ok / max(elapsed, 1e-6):.2f}, pending={len(pending)}")

    cv2.destroyAllWindows()
    sock.close()


if __name__ == "__main__":
    main()
