#!/usr/bin/env python3
import sys, json, socket, threading, queue
from PySide6.QtWidgets import QApplication, QSlider, QLineEdit, QPushButton, QLabel
from PySide6.QtUiTools import QUiLoader
from PySide6.QtCore import QFile, QTimer, QObject, Signal
from PySide6.QtGui import QIntValidator

# ------------ Config ------------
HOST, PORT = "127.0.0.1", 5000
CONNECT_RETRY_SEC = 1.0
SEND_TIMEOUT_SEC  = 1.0
# --------------------------------

PARAMS = [
    "Gain", "Black_Level", "Color_Gain", "Color_Hue",
    "Speed", "Image_Gamma", "H_Shift", "Filter_Type",
]

FRAME_KEY_ALIASES = {"Frame_Number", "Location"}
DURATION_KEY = "Duration"

class RxBridge(QObject):
    kv_received = Signal(str, str)   # key, value (both strings)

class SocketClient:
    """Tiny TCP client with a TX and RX thread, plus a loopback guard."""
    def __init__(self, host: str, port: int):
        self.host, self.port = host, port
        self.sock = None
        self.sock_lock = threading.Lock()
        self.send_q: "queue.Queue[str]" = queue.Queue(2048)
        self.stop_flag = threading.Event()
        self.mute_sends = False
        self.rx_bridge = RxBridge()

        threading.Thread(target=self._run_tx, name="SocketTX", daemon=True).start()
        threading.Thread(target=self._run_rx, name="SocketRX", daemon=True).start()

    # ----- socket helpers -----
    def _connect(self):
        import time
        while not self.stop_flag.is_set():
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(3.0)
                s.connect((self.host, self.port))
                s.settimeout(None)  # blocking mode
                print(f"✅ Connected to {self.host}:{self.port}")
                return s
            except Exception as e:
                print(f"⚠️ Connect failed: {e} — retrying in {CONNECT_RETRY_SEC}s")
                time.sleep(CONNECT_RETRY_SEC)

    def _safe_sendall(self, b: bytes) -> bool:
        with self.sock_lock:
            if not self.sock:
                return False
            try:
                self.sock.sendall(b)
                return True
            except Exception:
                try:
                    self.sock.close()
                except Exception:
                    pass
                self.sock = None
                return False

    # ----- threads -----
    def _run_tx(self):
        while not self.stop_flag.is_set():
            if not self.sock:
                self.sock = self._connect()
                if not self.sock:
                    break
            try:
                line = self.send_q.get(timeout=0.2)
            except queue.Empty:
                continue
            if not self._safe_sendall(line.encode("utf-8")):
                try:
                    self.send_q.put_nowait(line)
                except queue.Full:
                    print("⚠️ Send queue full; dropping")
                continue

    def _run_rx(self):
        import socket as pysock
        buf = b""
        while not self.stop_flag.is_set():
            if not self.sock:
                self.sock = self._connect()
                if not self.sock:
                    break
            try:
                data = self.sock.recv(4096)
                if not data:
                    # server closed
                    with self.sock_lock:
                        if self.sock:
                            try:
                                self.sock.close()
                            except Exception:
                                pass
                        self.sock = None
                    continue
                buf += data
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    if not line:
                        continue
                    try:
                        msg = json.loads(line.decode("utf-8"))
                    except Exception as e:
                        print("⚠️ Bad JSON from server:", e, line[:120])
                        continue
                    for k, v in msg.items():
                        self.rx_bridge.kv_received.emit(str(k), str(v))
            except pysock.timeout:
                continue
            except Exception:
                with self.sock_lock:
                    if self.sock:
                        try:
                            self.sock.close()
                        except Exception:
                            pass
                    self.sock = None

    # ----- public -----
    def send_json(self, obj: dict):
        try:
            self.send_q.put_nowait(json.dumps(obj, separators=(",", ":")) + "\n")
        except queue.Full:
            print("⚠️ Send queue full; dropping")

    def close(self):
        self.stop_flag.set()
        with self.sock_lock:
            if self.sock:
                try:
                    self.sock.close()
                except Exception:
                    pass
            self.sock = None

# ---------- UI wiring ----------
def wire_slider_and_box(window, name: str, sock: SocketClient):
    slider: QSlider   = window.findChild(QSlider, name)
    box:    QLineEdit = window.findChild(QLineEdit, f"{name}_Value")
    if not slider or not box:
        print(f"⚠️ Missing pair for {name}: slider={bool(slider)} box={bool(box)}")
        return

    smin, smax = slider.minimum(), slider.maximum()
    box.setValidator(QIntValidator(smin, smax, box))
    box.setText(str(slider.value()))

    def on_slider_changed(val: int):
        if box.text() != str(val):
            box.setText(str(val))
        print(f"{name} = {val}", flush=True)
        if not sock.mute_sends:
            sock.send_json({name: str(val)})

    def on_box_committed():
        txt = box.text().strip()
        if not txt:
            box.setText(str(slider.value()))
            return
        try:
            v = int(txt)
        except ValueError:
            box.setText(str(slider.value()))
            return
        if v < smin:
            v = smin
        if v > smax:
            v = smax
        if v != slider.value():
            slider.setValue(v)     # triggers on_slider_changed
        else:
            print(f"{name} = {v} (typed)", flush=True)
            if not sock.mute_sends:
                sock.send_json({name: str(v)})
        if box.text() != str(v):
            box.setText(str(v))

    slider.valueChanged.connect(on_slider_changed)
    box.editingFinished.connect(on_box_committed)

def wire_transport_buttons(window, sock: SocketClient):
    btn_pause: QPushButton = window.findChild(QPushButton, "Pause_Toggle")
    btn_ff:    QPushButton = window.findChild(QPushButton, "Fast_Forward")
    btn_rw:    QPushButton = window.findChild(QPushButton, "Rewind")

    # Pause toggle: icon swap + yellow when paused
    if btn_pause:
        btn_pause.setCheckable(True)
        btn_pause.setText("⏸")  # start as playing (unchecked)
        btn_pause.setStyleSheet("QPushButton:checked { background-color: yellow; }")

        def on_pause_toggled(checked: bool):
            # Show ▶ when paused (checked), ⏸ when playing (unchecked)
            btn_pause.setText("▶" if checked else "⏸")
            if not sock.mute_sends:
                sock.send_json({"Pause_Toggle": "1" if checked else "0"})
        btn_pause.toggled.connect(on_pause_toggled)

    # Fast Forward: momentary (press/release)
    if btn_ff:
        btn_ff.setText("⏩")
        btn_ff.setAutoRepeat(True)
        btn_ff.setAutoRepeatDelay(300)
        btn_ff.setAutoRepeatInterval(100)
        btn_ff.pressed.connect(lambda: (not sock.mute_sends) and sock.send_json({"Fast_Forward": "1"}))
        btn_ff.released.connect(lambda: (not sock.mute_sends) and sock.send_json({"Fast_Forward": "0"}))

    # Rewind: momentary (press/release)
    if btn_rw:
        btn_rw.setText("⏪")
        btn_rw.setAutoRepeat(True)
        btn_rw.setAutoRepeatDelay(300)
        btn_rw.setAutoRepeatInterval(100)
        btn_rw.pressed.connect(lambda: (not sock.mute_sends) and sock.send_json({"Rewind": "1"}))
        btn_rw.released.connect(lambda: (not sock.mute_sends) and sock.send_json({"Rewind": "0"}))

def send_snapshot(window, sock):
    """Send current values for all sliders and pause state."""
    for name in PARAMS:
        sl: QSlider = window.findChild(QSlider, name)
        if sl:
            sock.send_json({name: str(sl.value())})
    # include pause state
    btn_pause: QPushButton = window.findChild(QPushButton, "Pause_Toggle")
    if btn_pause:
        sock.send_json({"Pause_Toggle": "1" if btn_pause.isChecked() else "0"})

def main():
    app = QApplication(sys.argv)

    # Load UI
    loader = QUiLoader()
    f = QFile("A_Preview_Small.ui")
    if not f.open(QFile.ReadOnly):
        print("❌ Could not open A_Preview_Small.ui")
        sys.exit(1)
    window = loader.load(f)
    f.close()
    if not window:
        print("❌ Failed to load UI")
        sys.exit(1)

    sock = SocketClient(HOST, PORT)

    # Wire sliders/boxes
    for n in PARAMS:
        wire_slider_and_box(window, n, sock)

    # Wire transport buttons
    wire_transport_buttons(window, sock)

    # ---- Status label state + renderer ----
    status_label: QLabel = window.findChild(QLabel, "Status_Label")
    status_state = {"frame": 0, "duration": 0}

    def render_status():
        if status_label:
            status_label.setText(
                f"Frame Number: {status_state['frame']:05d}   Duration: {status_state['duration']:05d}"
            )

    # Apply incoming KVs safely on GUI thread
    def apply_remote_kv(key: str, val: str):
        # Special command from server
        if key == "command" and val == "snapshot":
            print("📩 Server requested snapshot")
            send_snapshot(window, sock)
            return

        # Handle pause toggle from server
        if key == "Pause_Toggle":
            btn: QPushButton = window.findChild(QPushButton, "Pause_Toggle")
            if btn:
                checked = val in ("1", "true", "True")
                sock.mute_sends = True
                try:
                    btn.setChecked(checked)  # triggers icon swap + yellow via toggled()
                finally:
                    sock.mute_sends = False
            return

        # Handle status fields (single label): Frame_Number/Location & Duration
        if key in FRAME_KEY_ALIASES:
            try:
                status_state["frame"] = max(0, int(val))
                render_status()
            except ValueError:
                pass
            return
        if key == DURATION_KEY:
            try:
                status_state["duration"] = max(0, int(val))
                render_status()
            except ValueError:
                pass
            return

        # Otherwise: update a slider + box pair
        slider: QSlider   = window.findChild(QSlider, key)
        box:    QLineEdit = window.findChild(QLineEdit, f"{key}_Value")
        if not slider or not box:
            return
        try:
            iv = int(val)
        except ValueError:
            print(f"⚠️ Non-integer for {key}: {val}")
            return
        if iv < slider.minimum():
            iv = slider.minimum()
        if iv > slider.maximum():
            iv = slider.maximum()
        sock.mute_sends = True
        try:
            if slider.value() != iv:
                slider.setValue(iv)
            if box.text() != str(iv):
                box.setText(str(iv))
        finally:
            sock.mute_sends = False

    sock.rx_bridge.kv_received.connect(apply_remote_kv)

    # Initial render of status (matches your .ui default but ensures format)
    render_status()

    # Initial snapshot once UI is ready
    QTimer.singleShot(150, lambda: send_snapshot(window, sock))

    app.aboutToQuit.connect(sock.close)
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()

