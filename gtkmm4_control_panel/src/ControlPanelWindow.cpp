#include <vector>

#include <cassert>

#include <fmt/format.h>
#include <gtkmm.h>

#include "ControlPanelWindow.hpp"

// signal handler signature:
// - https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1DropTarget.html#ade3b5d090fa5fe1b462467c7d2455cda
// TBD gtk symbols used: G_VALUE_HOLDS GDK_TYPE_FILE_LIST GdkFileList
bool
ControlPanelWindow::_onDropLabelDrop(
    const Glib::ValueBase& value, double /*x*/, double /*y*/) {
    // TBD no gtkmm-4.0 Gdk::FileList
    assert(G_VALUE_HOLDS(value.gobj(), GDK_TYPE_FILE_LIST));
    Glib::Value<GdkFileList*> file_list_value;
    file_list_value.init(value.gobj());
    const std::vector<Glib::RefPtr<Gio::File>> files {
        Glib::SListHandler<Glib::RefPtr<Gio::File>>::slist_to_vector(
            gdk_file_list_get_files(file_list_value.get()),
            Glib::OWNERSHIP_SHALLOW)};
    fmt::println("{} files dropped:", files.size());
    for (const Glib::RefPtr<Gio::File>& file : files) {
        fmt::println("    {:?}", file->get_path());
    }
    return true;
}

static void
simulatedJSONOutput(const std::string_view key, const int value) {
    fmt::println("-- mock JSON update c>s (GUI15_PY style): '{{{:?}: \"{}\"}}'",
                 key, value);
}

// signal handler signature for all adjustment value changed callbacks:
// - https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Adjustment.html#aff87bea8a93f110b682a6732cb790a9c
void
ControlPanelWindow::_onGainValueChanged() {
    return simulatedJSONOutput(
        _UIParamKeys::GAIN,
        _slider_params.at(_UIParamKeys::GAIN).get_value());
}

void
ControlPanelWindow::_onBlackValueChanged() {
    return simulatedJSONOutput(
        _UIParamKeys::BLACK,
        _slider_params.at(_UIParamKeys::BLACK).get_value());
}

void
ControlPanelWindow::_onColorValueChanged() {
    return simulatedJSONOutput(
        _UIParamKeys::COLOR,
        _slider_params.at(_UIParamKeys::COLOR).get_value());
}

void
ControlPanelWindow::_onHueValueChanged() {
    return simulatedJSONOutput(
        _UIParamKeys::HUE,
        _slider_params.at(_UIParamKeys::HUE).get_value());
}

void
ControlPanelWindow::_onGammaValueChanged() {
    return simulatedJSONOutput(
        _UIParamKeys::GAMMA,
        _slider_params.at(_UIParamKeys::GAMMA).get_value());
}

void
ControlPanelWindow::_onHShiftValueChanged() {
    return simulatedJSONOutput(
        _UIParamKeys::H_SHIFT,
        _slider_params.at(_UIParamKeys::H_SHIFT).get_value());
}

void
ControlPanelWindow::_onRotateValueChanged() {
    return simulatedJSONOutput(
        _UIParamKeys::ROTATE,
        _slider_params.at(_UIParamKeys::ROTATE).get_value());
}

void
ControlPanelWindow::_onSpeedValueChanged() {
    return simulatedJSONOutput(
        _UIParamKeys::SPEED,
        _slider_params.at(_UIParamKeys::SPEED).get_value());
}

void
ControlPanelWindow::_onFilterValueChanged() {
    return simulatedJSONOutput(
        _UIParamKeys::FILTER,
        _slider_params.at(_UIParamKeys::FILTER).get_value());
}

std::string
ControlPanelWindow::_sliderParamsToMockJSON() {
    std::string mock_json;
    mock_json.append("-- mock JSON state (gtk4_demo_0 style):\n{\n");
    // TBD using traits instead of iterating over _slider_params ensures
    //   lookup of values in window layout order
    for (const auto& traits : _SLIDER_TRAITS) {
        mock_json.append(
            fmt::format("  {:?}: {},\n",
                        traits.name, _slider_params.at(traits.name).get_value()));
    }
    mock_json.append("}");
    return mock_json;
}

// Gtk::Button signal_clicked() callback signature:
// https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Button.html#a1f1f7b95ecf966171a3f42a032ff68ad
void
ControlPanelWindow::_onPrintButtonClicked() {
    fmt::println("{}", _sliderParamsToMockJSON());
}

// Glib::SignalTimeout::connect param slot: const sigc::slot< bool()>
// https://gnome.pages.gitlab.gnome.org/glibmm/classGlib_1_1SignalTimeout.html#a26c6d456b606758e819d3db69ae17d56
bool
ControlPanelWindow::_onCopyButtonTimeout() {
    _copy_button.set_label("Copy JSON");
    return true;
}

void
ControlPanelWindow::_onCopyButtonClicked() {
    get_clipboard()->set_text(_sliderParamsToMockJSON());
   _copy_button.set_label("Copied JSON!");
   Glib::signal_timeout().connect(
       sigc::mem_fun(*this, &ControlPanelWindow::_onCopyButtonTimeout),
       /*interval (ms)*/1500);
}

void
ControlPanelWindow::_onResetButtonClicked() {
    // TBD intentionally not block()/unblock() signals as a reset should count
    //   as param update
    for (auto& [key, value] : _slider_params) {
        value.restore_default();
    }
}

ControlPanelWindow::ControlPanelWindow() {
    set_title(_TITLE.data());

    set_default_size(400, 500);
    set_resizable(true);

    _scroll.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    set_child(_scroll);

    _scroll_vbox = Gtk::Box(Gtk::Orientation::VERTICAL, /*spacing*/4);
    _scroll_vbox.set_margin_top(16);
    _scroll_vbox.set_margin_bottom(16);
    _scroll_vbox.set_margin_start(20);
    _scroll_vbox.set_margin_end(20);
    _scroll.set_child(_scroll_vbox);

    Gtk::Label drop_label("Drop file(s) here");
    //_drop_label.set_wrap(true);
    drop_label.set_justify(Gtk::Justification::CENTER);
    drop_label.add_css_class("drop-zone");
    Glib::RefPtr< Gtk::DropTarget > dt =
        Gtk::DropTarget::create(GDK_TYPE_FILE_LIST, Gdk::DragAction::COPY);
    dt->signal_drop().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::_onDropLabelDrop),
        /*after*/false);
    drop_label.add_controller(dt);
    _scroll_vbox.append(drop_label);

    Gtk::Separator drop_sliders_separator (Gtk::Orientation::HORIZONTAL);
    _scroll_vbox.append(drop_sliders_separator);

    for (const _SliderTraits& traits : _SLIDER_TRAITS) {
        Gtk::Box row(Gtk::Orientation::HORIZONTAL, /*spacing*/4);
        row.set_margin_top(0);

        Gtk::Label label(traits.name.data());
        label.add_css_class("section-label");
        label.set_xalign(0);
        label.set_size_request(70, -1);

        Glib::RefPtr<Gtk::Adjustment> adjustment {
            Gtk::Adjustment::create(
                traits.default_value, traits.min_value,
                traits.max_value, traits.increment)};
        // TBD could assign return value sigc::connection to _slider_params
        //   values to allow for signal blocking
        adjustment->signal_value_changed().connect(
            sigc::mem_fun(*this, traits.value_changed_callback),
            /*after*/false);
        _slider_params.emplace(
            traits.name, _UIParamValue(adjustment, traits.default_value));
        Gtk::Scale slider {adjustment, Gtk::Orientation::HORIZONTAL};
        slider.set_hexpand(true);
        slider.set_draw_value(false);
        Gtk::SpinButton spin_button {adjustment};
        spin_button.set_size_request(65, -1);

        row.append(label);
        row.append(slider);
        row.append(spin_button);

        _scroll_vbox.append(row);
    }

    Gtk::Separator sliders_buttons_separator (Gtk::Orientation::HORIZONTAL);
    _scroll_vbox.append(sliders_buttons_separator);

    Gtk::Box button_box(Gtk::Orientation::HORIZONTAL, /*spacing*/10);
    button_box.set_halign(Gtk::Align::CENTER);
    _print_button.set_label("Print JSON");
    _print_button.set_name("btn_print");
    _print_button.signal_clicked().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::_onPrintButtonClicked),
        /*after*/false);
    button_box.append(_print_button);
    _copy_button.set_label("Copy JSON");
    _copy_button.set_name("btn_copy");
    _copy_button.signal_clicked().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::_onCopyButtonClicked),
        /*after*/false);
    button_box.append(_copy_button);
    _reset_button.set_label("Reset");
    _reset_button.set_name("btn_reset");
    _reset_button.signal_clicked().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::_onResetButtonClicked),
        /*after*/false);
    button_box.append(_reset_button);
    _scroll_vbox.append(button_box);

    //// layout

    //// styling
    Glib::RefPtr<Gtk::CssProvider> css {Gtk::CssProvider::create()};
    // TBD using gtkmm 4.10, load_from_string not available until 4.12
    css->load_from_data(R"(
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
    )");
    Gtk::StyleContext::add_provider_for_display(
        Gdk::Display::get_default(), css,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
