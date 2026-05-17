import gi
import json
gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, Gdk, GLib

state = {
    "file_name": None,
    "file_path": None,
}


def on_drop(drop_target, value, x, y):
    if isinstance(value, Gdk.FileList):
        files = list(value)
    else:
        files = [value]
    for gfile in files:
        path = gfile.get_path()
        if not path.endswith(".txt"):
            drop_label.set_text("⚠️  Drop a .txt file!")
            return False
        try:
            with open(path, "r", encoding="utf-8") as f:
                contents = f.read()
            print(f"\n{'='*50}\nFile: {path}\n{'='*50}")
            print(contents)
            print(f"{'='*50}\n")
            name = path.split("/")[-1]
            state["file_name"] = name
            state["file_path"] = path
            drop_label.set_text(f"✅ {name}")
        except Exception as e:
            print(f"[Error] {e}")
            drop_label.set_text("❌ Error reading file")
    return True


def clean(val, decimals):
    return int(val) if decimals == 0 else round(val, decimals)

def on_slider_changed(slider, spin, label, decimals):
    val = clean(slider.get_value(), decimals)
    spin.set_value(val)
    state[label] = val

def on_spin_changed(spin, slider, label, decimals):
    val = clean(spin.get_value(), decimals)
    slider.handler_block_by_func(on_slider_changed)
    slider.set_value(val)
    slider.handler_unblock_by_func(on_slider_changed)
    state[label] = val


def make_row(label_text, min_val, max_val, step, default, decimals=1):
    row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=4)
    row.set_margin_top(0)

    lbl = Gtk.Label(label=label_text)
    lbl.add_css_class("section-label")
    lbl.set_xalign(0)
    lbl.set_size_request(70, -1)

    slider = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, min_val, max_val, step)
    slider.set_value(default)
    slider.set_hexpand(True)
    slider.set_draw_value(False)

    spin = Gtk.SpinButton.new_with_range(min_val, max_val, step)
    spin.set_value(default)
    spin.set_digits(decimals)
    spin.set_size_request(65, -1)

    slider.connect("value-changed", on_slider_changed, spin, label_text, decimals)
    spin.connect("value-changed", on_spin_changed, slider, label_text, decimals)

    state[label_text] = default

    row.append(lbl)
    row.append(slider)
    row.append(spin)
    return row, slider, spin


def on_reset(btn, rows_data):
    for label, slider, spin, default, decimals in rows_data:
        slider.handler_block_by_func(on_slider_changed)
        spin.handler_block_by_func(on_spin_changed)
        slider.set_value(default)
        spin.set_value(default)
        slider.handler_unblock_by_func(on_slider_changed)
        spin.handler_unblock_by_func(on_spin_changed)
        state[label] = default
    print_state()

def on_copy(btn, rows_data, win):
    text = json.dumps(state, indent=2)
    win.get_clipboard().set(text)
    print("[Copied]\n" + text)
    btn.set_label("✅ Copied!")
    GLib.timeout_add(1500, lambda: btn.set_label("📋 Copy JSON") or False)

def on_print_all(btn, rows_data):
    print_state()

def print_state():
    print("\n── state ───────────────────────────────")
    print(json.dumps(state, indent=2))
    print("────────────────────────────────────────\n")


def on_activate(app):
    global drop_label

    win = Gtk.ApplicationWindow(application=app)
    win.set_title("GTK4 — Sliders")
    win.set_default_size(400, 500)
    win.set_resizable(True)

    css = Gtk.CssProvider()
    css.load_from_string("""
        window { background-color: #2b2b2b; }
        .drop-zone {
            color: white; font-size: 12pt;
            background-color: #3c3f41;
            border-radius: 8px; padding: 14px;
        }
        .section-label {
            color: #dddddd; font-size: 11pt; font-weight: bold;
        }
        scale trough    { background-color: #444; border-radius: 4px; }
        scale highlight { background-color: #4a9eff; border-radius: 4px; }
        spinbutton {
            background-color: #3c3f41; color: white;
            border-radius: 4px; border: 1px solid #555;
            padding: 0px 2px; min-height: 0; font-size: 8pt;
        }
        spinbutton text   { min-height: 0; padding: 0; }
        spinbutton button { min-height: 0; padding: 0px 2px; color: #A0A0A0; }
        button {
            padding: 0; margin: 0; border: none; outline: none;
            box-shadow: none; background: none;
            min-height: 0; min-width: 0;
        }
        button:hover, button:active, button:focus { box-shadow: none; }
        #btn_print, #btn_print label {
            background-color: #4a9eff; color: white;
            border-radius: 4px; padding: 3px 8px;
            font-size: 8pt; font-weight: bold;
        }
        #btn_print:hover, #btn_print:hover label { background-color: #3a8eef; }
        #btn_print:active, #btn_print:active label { background-color: #2a7edf; }
        #btn_copy, #btn_copy label {
            background-color: #3a7d44; color: white;
            border-radius: 4px; padding: 3px 8px;
            font-size: 8pt; font-weight: bold;
        }
        #btn_copy:hover, #btn_copy:hover label { background-color: #2e6436; }
        #btn_copy:active, #btn_copy:active label { background-color: #256030; }
        #btn_reset, #btn_reset label {
            background-color: #555; color: white;
            border-radius: 4px; padding: 3px 8px; font-size: 8pt;
        }
        #btn_reset:hover, #btn_reset:hover label { background-color: #666; }
        #btn_reset:active, #btn_reset:active label { background-color: #444; }
    """)
    Gtk.StyleContext.add_provider_for_display(
        Gdk.Display.get_default(), css,
        Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION,
    )

    scroll = Gtk.ScrolledWindow()
    scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
    win.set_child(scroll)

    vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
    vbox.set_margin_top(16)
    vbox.set_margin_bottom(16)
    vbox.set_margin_start(20)
    vbox.set_margin_end(20)
    scroll.set_child(vbox)

    drop_label = Gtk.Label(label="🗂️  Drop a .txt file here")
    drop_label.set_wrap(True)
    drop_label.set_justify(Gtk.Justification.CENTER)
    drop_label.add_css_class("drop-zone")
    dt = Gtk.DropTarget.new(Gdk.FileList, Gdk.DragAction.COPY)
    dt.connect("drop", on_drop)
    drop_label.add_controller(dt)
    vbox.append(drop_label)

    vbox.append(Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL))

    configs = [
        ("Gain",       0,    200,   1,    100,  0),
        ("Black",     -100,  100,   1,      0,  0),
        ("Color",      0,    200,   1,    100,  0),
        ("Hue",       -180,  180,   1,      0,  0),
        ("Gamma",      0,    100,   1,      0,  0),
        ("H_Shift",    0,    100,   1,      0,  0),
        ("Rotate",    -100,  100,   1,      0,  0),
        ("Speed",      0,    200,   1,    100,  0),
        ("Filter",     0,     10,   1,      3,  0),
    ]

    rows_data = []
    for label, mn, mx, step, default, decimals in configs:
        row, slider, spin = make_row(label, mn, mx, step, default, decimals)
        vbox.append(row)
        rows_data.append((label, slider, spin, default, decimals))
    print(rows_data)

    vbox.append(Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL))

    btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
    btn_box.set_halign(Gtk.Align.CENTER)

    btn_print = Gtk.Button(label="🖨 Print Dict")
    btn_print.set_name("btn_print")
    btn_print.connect("clicked", on_print_all, rows_data)

    btn_copy = Gtk.Button(label="📋 Copy JSON")
    btn_copy.set_name("btn_copy")
    btn_copy.connect("clicked", on_copy, rows_data, win)

    btn_reset = Gtk.Button(label="↺ Reset")
    btn_reset.set_name("btn_reset")
    btn_reset.connect("clicked", on_reset, rows_data)

    btn_box.append(btn_print)
    btn_box.append(btn_copy)
    btn_box.append(btn_reset)
    vbox.append(btn_box)

    win.present()


app = Gtk.Application(application_id="com.example.gtk4demo")
app.connect("activate", on_activate)
app.run(None)
