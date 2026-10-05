#!/usr/bin/env python3
# pylint: disable=no-member,invalid-name,missing-function-docstring,missing-class-docstring
"""
mjpeg_server.py  -  myOS Video Stream Server
=============================================
Runs on your laptop, streams a video file (or webcam) as MJPEG
over HTTP so the ESP32 can display it.

Requirements:
    pip install opencv-python

Usage:
    python mjpeg_server.py                      # streams sample.mp4
    python mjpeg_server.py --source 0           # streams webcam
    python mjpeg_server.py --source video.mp4 --port 8080 --fps 12

Then on the ESP32 Video app, enter your laptop's local IP.
"""

import argparse
import time
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer

try:
    import cv2  # type: ignore[import-untyped]
except ImportError:
    print("Install opencv-python first:  pip install opencv-python")
    raise SystemExit(1)

# ── Config defaults ───────────────────────────────────────────
DEFAULT_SOURCE = "sample.mp4"
DEFAULT_PORT   = 8080
TARGET_W       = 320
TARGET_H       = 240
JPEG_QUALITY   = 60     # lower = smaller frames, higher FPS

# ── Runtime-mutable FPS (set by CLI args in main) ─────────────
# Kept as a module-level variable so threads read the final value.
_fps: int = 12

# ── Shared frame (thread-safe with lock) ──────────────────────
_current_frame: bytes | None = None
_frame_lock   = threading.Lock()
_stop_event   = threading.Event()


def capture_thread(source: str) -> None:
    """Read frames from source, resize, JPEG-encode, store latest."""
    global _current_frame  # pylint: disable=global-statement

    # Accept numeric string (webcam index) or file path
    try:
        src: str | int = int(source)
    except ValueError:
        src = source

    cap = cv2.VideoCapture(src)  # type: ignore[attr-defined]
    if not cap.isOpened():
        print(f"[ERROR] Cannot open source: {source}")
        _stop_event.set()
        return

    print(f"[INFO] Capture started: {source}  ->  {TARGET_W}x{TARGET_H} @ {_fps} FPS")

    while not _stop_event.is_set():
        ret, frame = cap.read()
        if not ret:
            # Loop video file back to beginning
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)  # type: ignore[attr-defined]
            continue

        frame = cv2.resize(frame, (TARGET_W, TARGET_H))  # type: ignore[attr-defined]
        ret2, encoded = cv2.imencode(                      # type: ignore[attr-defined]
            ".jpg", frame,
            [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY]       # type: ignore[attr-defined]
        )
        if ret2:
            with _frame_lock:
                _current_frame = encoded.tobytes()

        time.sleep(1.0 / _fps)

    cap.release()
    print("[INFO] Capture thread stopped")


# ── HTTP handler ──────────────────────────────────────────────
class MJPEGHandler(BaseHTTPRequestHandler):
    """HTTP request handler that serves an MJPEG stream on GET /."""

    def log_message(self, format: str, *args: object) -> None:  # noqa: A002
        """Suppress per-request console logs (very noisy at 12 FPS)."""

    def do_GET(self) -> None:  # pylint: disable=invalid-name
        """Handle GET request: serve MJPEG multipart stream."""
        if self.path != "/":
            self.send_response(404)
            self.end_headers()
            return

        self.send_response(200)
        self.send_header(
            "Content-Type",
            "multipart/x-mixed-replace; boundary=frame"
        )
        self.end_headers()

        print(f"[INFO] Client connected: {self.client_address[0]}")
        try:
            while not _stop_event.is_set():
                with _frame_lock:
                    frame = _current_frame

                if frame is None:
                    time.sleep(0.05)
                    continue

                self.wfile.write(b"--frame\r\n")
                self.wfile.write(b"Content-Type: image/jpeg\r\n\r\n")
                self.wfile.write(frame)
                self.wfile.write(b"\r\n")
                time.sleep(1.0 / _fps)

        except (BrokenPipeError, ConnectionResetError):
            print(f"[INFO] Client disconnected: {self.client_address[0]}")


# ── Entry point ───────────────────────────────────────────────
def main() -> None:
    """Parse CLI args, start capture thread, start HTTP server."""
    global _fps  # pylint: disable=global-statement

    parser = argparse.ArgumentParser(description="myOS MJPEG server")
    parser.add_argument(
        "--source", default=DEFAULT_SOURCE,
        help="Video file path or webcam index (default: sample.mp4)"
    )
    parser.add_argument(
        "--port", type=int, default=DEFAULT_PORT,
        help="HTTP port (default: 8080)"
    )
    parser.add_argument(
        "--fps", type=int, default=12,
        help="Target FPS (default: 12)"
    )
    args = parser.parse_args()

    # Apply FPS from CLI — must happen BEFORE threads start reading _fps
    _fps = args.fps

    # Start capture thread (daemon so it dies when main exits)
    t = threading.Thread(target=capture_thread, args=(args.source,), daemon=True)
    t.start()

    # Brief wait for first frame to be ready
    time.sleep(0.5)

    # Start HTTP server
    server = HTTPServer(("0.0.0.0", args.port), MJPEGHandler)
    print(f"[INFO] MJPEG server running on http://0.0.0.0:{args.port}")
    print("[INFO] Enter your laptop IP in the ESP32 Video app")
    print("[INFO] Press Ctrl+C to stop")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[INFO] Shutting down...")
        _stop_event.set()
        server.shutdown()


if __name__ == "__main__":
    main()
