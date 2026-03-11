#!/usr/bin/env python3
# tk_kv_gui.py
import json
import queue
import socket
import threading
import time
import tkinter as tk
import tkinter.font as tkfont
from tkinter import ttk

# ------------ Config ------------
HOST, PORT = "127.0.0.1", 5000
CONNECT_RETRY_SEC = 1.0
SEND_TIMEOUT_SEC  = 1.0
# --------------------------------

# ---- Wire-format toggles (match your Qt/C++) ----
# "flat" -> {"Gain":"123"}   |  "pair" -> {"key":"Gain","value":"123"}
SEND_SHAPE   = "flat"
LINE_ENDING  = "\r\n"    # many C/C++ readers prefer CRLF
DEBUG_PRINT  = False     # True to see [NET]/[TX]/[RX] prints

# ---- UI sizing tweaks (1080p baseline ~2/3 size) ----
SCALE_LENGTH_PX = 155   # was 156 -> ~2/3
ENTRY_CHARS     = 4     # was 5   -> smaller entry

# ---- Names (match your C++/Qt keys exactly) ----
GAIN          = "Gain"
BLACK_LEVEL   = "Black_Level"
COLOR_GAIN    = "Color_Gain"
COLOR_HUE     = "Color_Hue"
IMAGE_GAMMA   = "Image_Gamma"
H_SHIFT       = "H_Shift"
ROTATE        = "Rotate"
SPEED         = "Speed"
FILTER_TYPE   = "Filter_Type"
PAUSE_TOGGLE  = "Pause_Toggle"
FAST_FORWARD  = "Fast_Forward"
REWIND        = "Rewind"

# ---- Ordered controls with defaults & limits (Qt-matching) ----
# Each tuple: (name, default, min, max, kind)
# kind: "slider" or "toggle" or "momentary"
PARAM_LAYOUT = [
    (GAIN,         100,  -100,  200, "slider"),
    (BLACK_LEVEL,    0,  -100,  100, "slider"),
    (COLOR_GAIN,   100,     0,  200, "slider"),
    (COLOR_HUE,      0,   -180, 180, "slider"),
    (IMAGE_GAMMA,   25,     0,  100, "slider"),
    (H_SHIFT,      512,     0, 1023, "slider"),
    (ROTATE,         0,   -100, 100, "slider"),
    (SPEED,        100,     0,  300, "slider"),
    (FILTER_TYPE,    3,     0,   10, "slider"),
    (PAUSE_TOGGLE,   0,     0,    1, "toggle"),
    (FAST_FORWARD,   0,     0,    1, "momentary"),
    (REWIND,         0,     0,    1, "momentary"),
]

# ---- Display label overrides (UI only, does NOT change keys sent to C++) ----
DISPLAY_LABELS = {
    GAIN: "Gain",
    BLACK_LEVEL: "Black",
    COLOR_GAIN: "Color",
    COLOR_HUE: "Hue",
    IMAGE_GAMMA: "Gamma",
    H_SHIFT: "H_Shift",
    ROTATE: "Rotate",
    SPEED: "Speed",
    FILTER_TYPE: "Filter",
}

# Optional read-only telemetry keys
FRAME_KEY_ALIASES = {"Frame_Number", "Location"}
DURATION_KEY = "Duration"


# ---------------- Socket client with TX/RX threads ----------------
class SocketClient:
    """Tiny TCP client with TX and RX threads, reconnect, and string-only JSON lines."""
    def __init__(self, host: str, port: int, rx_queue: queue.Queue, client_id="TK"):
        self.host, self.port = host, port
        self.rx_queue = rx_queue
        self.client_id = client_id

        self.sock = None
        self.sock_lock = threading.Lock()
        self.stop_event = threading.Event()

        self.tx_queue = queue.Queue(maxsize=256)

        self._rx_thread = threading.Thread(target=self._rx_loop, daemon=True)
        self._tx_thread = threading.Thread(target=self._tx_loop, daemon=True)
        self._conn_thread = threading.Thread(target=self._connect_loop, daemon=True)

    def start(self):
        self._conn_thread.start()
        self._rx_thread.start()
        self._tx_thread.start()

    def close(self):
        self.stop_event.set()
        with self.sock_lock:
            try:
                if self.sock:
                    self.sock.close()
            except Exception:
                pass
            self.sock = None

    def is_connected(self):
        with self.sock_lock:
            return self.sock is not None

    def send_kv(self, key: str, value: str):
        """Enqueue a KV send (string value). Non-blocking."""
        if not isinstance(key, str) or not isinstance(value, str):
            return
        try:
            self.tx_queue.put_nowait((key, value))
        except queue.Full:
            try:
                self.tx_queue.get_nowait()
                self.tx_queue.put_nowait((key, value))
            except Exception:
                pass

    def _connect_loop(self):
        while not self.stop_event.is_set():
            if self.is_connected():
                time.sleep(0.2)
                continue
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(3.0)
                s.connect((self.host, self.port))
                s.settimeout(None)
                with self.sock_lock:
                    self.sock = s
                self.rx_queue.put(("__status__", "Connected"))
                if DEBUG_PRINT:
                    print("[NET] CONNECTED", self.client_id)

                # Optional HELLO + request-all (safe if ignored)
                try:
                    # hello = json.dumps({"HELLO": f"{self.client_id}"}) + LINE_ENDING
                    # s.sendall(hello.encode("utf-8"))
                    req_all = json.dumps({"Request_All": "1"}) + LINE_ENDING
                    s.sendall(req_all.encode("utf-8"))
                except Exception:
                    pass

            except Exception:
                self.rx_queue.put(("__status__", "Connecting..."))
                if DEBUG_PRINT:
                    print("[NET] Connecting...")
                time.sleep(CONNECT_RETRY_SEC)

    def _rx_loop(self):
        buffer = b""
        while not self.stop_event.is_set():
            if not self.is_connected():
                time.sleep(0.1)
                continue
            try:
                with self.sock_lock:
                    s = self.sock
                data = s.recv(4096)
                if not data:
                    with self.sock_lock:
                        if self.sock:
                            self.sock.close()
                        self.sock = None
                    self.rx_queue.put(("__status__", "DISCONNECTED"))
                    if DEBUG_PRINT:
                        print("[NET] DISCONNECTED")
                    continue
                buffer += data
                while b"\n" in buffer:
                    line, buffer = buffer.split(b"\n", 1)
                    line = line.strip(b"\r \t")
                    if not line:
                        continue
                    self._parse_and_enqueue(line)
            except (socket.timeout, BlockingIOError):
                continue
            except Exception:
                with self.sock_lock:
                    if self.sock:
                        try:
                            self.sock.close()
                        except Exception:
                            pass
                    self.sock = None
                self.rx_queue.put(("__status__", "DISCONNECTED"))
                if DEBUG_PRINT:
                    print("[NET] DISCONNECTED (err)")

    def _parse_and_enqueue(self, raw: bytes):
        try:
            text = raw.decode("utf-8", errors="replace")
            if DEBUG_PRINT:
                print("[RX]", text)
            obj = json.loads(text)
            if isinstance(obj, dict):
                if "key" in obj and "value" in obj:
                    k = str(obj["key"]); v = str(obj["value"])
                    self.rx_queue.put((k, v))
                else:
                    for k, v in obj.items():
                        self.rx_queue.put((str(k), str(v)))
        except Exception as e:
            if DEBUG_PRINT:
                print("[RX-ERR]", e)

    def _tx_loop(self):
        while not self.stop_event.is_set():
            try:
                key, value = self.tx_queue.get(timeout=0.2)
            except queue.Empty:
                continue

            obj = {key: value} if SEND_SHAPE == "flat" else {"key": key, "value": value}
            payload = json.dumps(obj) + LINE_ENDING
            data = payload.encode("utf-8")

            sent = False
            t0 = time.time()
            while (time.time() - t0) < SEND_TIMEOUT_SEC and not sent and not self.stop_event.is_set():
                if not self.is_connected():
                    time.sleep(0.05)
                    continue
                try:
                    with self.sock_lock:
                        s = self.sock
                        if s:
                            if DEBUG_PRINT:
                                print("[TX]", payload.rstrip())
                            s.sendall(data)
                            sent = True
                except Exception as e:
                    if DEBUG_PRINT:
                        print("[TX-ERR] dropping socket:", e)
                    with self.sock_lock:
                        if self.sock:
                            try:
                                self.sock.close()
                            except Exception:
                                pass
                        self.sock = None
                    break



# ---------------- Tkinter App ----------------
class SliderRow:
    """Label | Scale | Entry
       Sends on integer-change while dragging, and again on release.
    """
    def __init__(self, parent, name: str, default: int, vmin: int, vmax: int,
                 send_fn, row_idx: int):
        self.name = name
        self.vmin, self.vmax = vmin, vmax
        self.send_fn = send_fn
        self._internal_update = False
        self.last_sent_value = None

        # compact padding + narrower label
        display_name = DISPLAY_LABELS.get(name, name)

        ttk.Label(parent, text=display_name, width=7).grid(
            row=row_idx, column=0, sticky="w", padx=(4, 0), pady=0
        )

        self.var_scale = tk.DoubleVar(value=default)
        self.scale = ttk.Scale(
            parent, from_=vmin, to=vmax, orient="horizontal",
            variable=self.var_scale, length=SCALE_LENGTH_PX,
            command=self._on_scale_move
        )
        # a bit closer to label
        self.scale.grid(row=row_idx, column=1, sticky="w", padx=(1, 2), pady=0)

        self.var_entry = tk.StringVar(value=str(default))
        vcmd = (parent.register(self._validate_entry), "%P")
        self.entry = ttk.Entry(
            parent,
            textvariable=self.var_entry,
            width=ENTRY_CHARS,
            style="Compact.TEntry",          # <-- shorter / tighter entry style
            validate="key",
            validatecommand=vcmd
        )
        self.entry.grid(row=row_idx, column=2, sticky="w", padx=(2, 3), pady=0)
        self.entry.bind("<Return>", self._on_entry_commit)
        self.entry.bind("<FocusOut>", self._on_entry_commit)

        self.scale.bind("<ButtonRelease-1>", self._on_scale_release)

    def _clamp(self, x) -> int:
        try:
            xi = int(round(float(x)))
        except Exception:
            xi = 0
        return max(self.vmin, min(self.vmax, xi))

    def _validate_entry(self, proposed: str):
        if proposed.strip() in ("", "-", "+"):
            return True
        try:
            _ = int(float(proposed))
            return True
        except Exception:
            return False

    def _on_scale_move(self, _evt=None):
        if self._internal_update:
            return
        v = self._clamp(self.var_scale.get())

        self._internal_update = True
        self.var_entry.set(str(v))
        self._internal_update = False

        sv = str(v)
        if sv != self.last_sent_value:
            self.last_sent_value = sv
            self.send_fn(self.name, sv)

    def _on_scale_release(self, _evt=None):
        v = self._clamp(self.var_scale.get())
        sv = str(v)
        self.last_sent_value = sv
        self.send_fn(self.name, sv)

    def _on_entry_commit(self, _evt=None):
        text = self.var_entry.get().strip()
        if text in ("", "-", "+"):
            return
        v = self._clamp(text)
        sv = str(v)

        self._internal_update = True
        self.var_entry.set(sv)
        self.var_scale.set(v)
        self._internal_update = False

        self.last_sent_value = sv
        self.send_fn(self.name, sv)

    def set_from_remote(self, value_str: str):
        if value_str in ("", "-", "+"):
            return
        v = self._clamp(value_str)

        self._internal_update = True
        self.var_entry.set(str(v))
        self.var_scale.set(v)
        self._internal_update = False

class TransportClusterRow:
    """Single row: PAUSE (toggle) + REW + FF (momentary) + Quit (right-justified)."""
    def __init__(self, parent, send_fn, row_idx: int,
                 quit_fn,
                 pause_key="Pause_Toggle",
                 rewind_key="Rewind",
                 ff_key="Fast_Forward",
                 auto_off_ms=50):

        self.send_fn = send_fn
        self.quit_fn = quit_fn
        self.auto_off_ms = auto_off_ms
        self.pause_key = pause_key
        self.rewind_key = rewind_key
        self.ff_key = ff_key
        self.pause_state = False

        # style = ttk.Style()
        # style.configure("Transport.TButton",
        #                 padding=(1, 0),
        #                 font=("TkDefaultFont", 7))

        # Holder spans entire row width so Quit can go far right
        holder = ttk.Frame(parent)
        holder.grid(row=row_idx, column=0, columnspan=3, sticky="ew",
                    padx=(4, 0), pady=(3, 1))

        holder.columnconfigure(0, weight=1)  # pushes Quit to the right

        # Left cluster
        left = ttk.Frame(holder)
        left.grid(row=0, column=0, sticky="w")

        self.btn_pause = ttk.Button(left, text="PAUSE", style="Transport.TButton",
                                    width=7, command=self._toggle_pause)
        self.btn_pause.pack(side="left", padx=(0, 5))

        self.btn_rew = ttk.Button(left, text="REW", style="Transport.TButton",
                                  width=4, command=self._press_rew)
        self.btn_rew.pack(side="left", padx=(0, 5))

        self.btn_ff = ttk.Button(left, text="FF", style="Transport.TButton",
                                 width=2, command=self._press_ff)
        self.btn_ff.pack(side="left", padx=(0, 0))

        # Quit button on the far right
        self.btn_quit = ttk.Button(holder, text="QUIT", style="Transport.TButton",
                                   width=4, command=self.quit_fn)
        self.btn_quit.grid(row=0, column=1, sticky="e", padx=(0, 6))

        self._update_pause_visual()

    def _toggle_pause(self):
        self.pause_state = not self.pause_state
        self.send_fn(self.pause_key, "1" if self.pause_state else "0")
        self._update_pause_visual()

    def _update_pause_visual(self):
        if self.pause_state:
            self.btn_pause.state(["pressed"])
        else:
            self.btn_pause.state(["!pressed"])

    def _press_rew(self):
        self.send_fn(self.rewind_key, "1")
        self.btn_rew.after(self.auto_off_ms,
                           lambda: self.send_fn(self.rewind_key, "0"))

    def _press_ff(self):
        self.send_fn(self.ff_key, "1")
        self.btn_ff.after(self.auto_off_ms,
                          lambda: self.send_fn(self.ff_key, "0"))

    def set_from_remote(self, value_str: str):
        # Optional: reflect Pause_Toggle coming back from C++
        try:
            self.pause_state = bool(int(value_str))
            self._update_pause_visual()
        except Exception:
            pass
        

class App:
    def __init__(self, root, client_id="TK"):
        self.root = root
        root.title("Processor")

        self.rx_queue = queue.Queue()
        self.client = SocketClient(HOST, PORT, self.rx_queue, client_id=client_id)
        self.client.start()

        # Main container
        main = ttk.Frame(root, padding=4)
        main.grid(row=0, column=0, sticky="nsew")
        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)

        # =========================
        # HEADER (top row)
        # =========================
        hdr = ttk.Frame(main)
        hdr.grid(row=0, column=0, columnspan=3, sticky="ew")

        # Left expands, right stays tight
        hdr.columnconfigure(0, weight=1)
        hdr.columnconfigure(1, weight=0)

        # --- Left: Location + Duration (no labels) ---
        self.hdr_location_var = tk.StringVar(value="—:—")
        self.hdr_duration_var = tk.StringVar(value="—:—")

        left = ttk.Frame(hdr)
        left.grid(row=0, column=0, sticky="w")

        ttk.Label(left, textvariable=self.hdr_location_var).pack(side="left")
        ttk.Label(left, text="   ").pack(side="left")  # spacer
        ttk.Label(left, textvariable=self.hdr_duration_var).pack(side="left")

        # --- Right: Connection status ---
        self.status_var = tk.StringVar(value="Connecting...")
        ttk.Label(hdr, textvariable=self.status_var).grid(row=0, column=1, sticky="e")

        # =========================
        # SLIDERS + TRANSPORT
        # =========================
        self.rows = {}
        rows_frame = ttk.Frame(main)
        rows_frame.grid(row=1, column=0, columnspan=3, sticky="nsew", pady=(4, 1))
        rows_frame.columnconfigure(1, weight=0)
        rows_frame.columnconfigure(2, weight=0)

        r = 0
        transport_added = False

        for name, default, vmin, vmax, kind in PARAM_LAYOUT:

            if kind == "slider":
                row = SliderRow(rows_frame, name, default, vmin, vmax,
                                send_fn=self.client.send_kv, row_idx=r)
                self.rows[name] = row
                r += 1
                continue

            if kind in ("toggle", "momentary"):
                if not transport_added:
                    row = TransportClusterRow(
                        rows_frame,
                        send_fn=self.client.send_kv,
                        row_idx=r,
                        quit_fn=self._quit,
                        pause_key=PAUSE_TOGGLE,
                        rewind_key=REWIND,
                        ff_key=FAST_FORWARD
                    )
                    self.rows[PAUSE_TOGGLE] = row
                    self.rows[REWIND] = row
                    self.rows[FAST_FORWARD] = row
                    r += 1
                    transport_added = True
                continue

        self._poll_rx()
        root.protocol("WM_DELETE_WINDOW", self._quit)

    # =========================
    # QUIT
    # =========================
    def _quit(self):
        try:
            self.client.send_kv("QUIT", "1")
            self.root.after(150, lambda: (self.client.close(), self.root.destroy()))
        except Exception:
            try:
                self.client.close()
            except Exception:
                pass
            self.root.destroy()

    # =========================
    # RX Poll
    # =========================
    def _poll_rx(self):
        try:
            while True:
                k, v = self.rx_queue.get_nowait()

                if k == "__status__":
                    self.status_var.set(v)
                    continue

                # Telemetry
                if k == "Location":
                    self.hdr_location_var.set(v)
                    continue

                if k == "Duration":
                    self.hdr_duration_var.set(v)
                    continue

                # Slider updates
                row = self.rows.get(k)
                if row:
                    row.set_from_remote(v)

        except queue.Empty:
            pass

        self.root.after(50, self._poll_rx)

def main():
    root = tk.Tk()

    # shrink overall UI text for 1080p baseline
    try:
        for name in ("TkDefaultFont", "TkTextFont", "TkHeadingFont", "TkMenuFont"):
            f = tkfont.nametofont(name)
            f.configure(size=10)
    except Exception:
        pass

    # theme + light blue
    try:
        style = ttk.Style(root)
        # prefer clam on Linux for consistent styling
        if "clam" in style.theme_names():
            style.theme_use("clam")

        # light blue theme
        LIGHT_BLUE = "#e6f2ff"
        WIDGET_BLUE = "#cfe8ff"
        root.configure(bg=LIGHT_BLUE)

        style.configure(".", font=("TkDefaultFont", 11))
        style.configure("TFrame", background=LIGHT_BLUE)
        style.configure("TLabel", background=LIGHT_BLUE)
        style.configure("TSeparator", background=LIGHT_BLUE)

        style.configure("TButton",
                        background=WIDGET_BLUE,
                        padding=2,
                        font=("TkDefaultFont", 8))
        style.map("TButton", background=[("active", "#b8dcff")])

        # ensure transport style also uses the smaller font
        style.configure("Transport.TButton",
                        padding=(1, 0),
                        font=("TkDefaultFont", 9))


        style.configure("Compact.TEntry",
                padding=(0, -1),
                font=("TkDefaultFont", 9))

    except Exception:
        pass

    
    
    App(root, client_id="TK_A")
    # root.title("KV Control – Tk (HD baseline smaller)")
    root.title("Processor")
    root.geometry("280x300+120+120")
    root.mainloop()


if __name__ == "__main__":
    main()