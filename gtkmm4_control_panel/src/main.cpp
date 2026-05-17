#include <fmt/format.h>
#include <gtkmm.h>

#include "ControlPanelWindow.hpp"


int
main(int argc, char* argv[]) {
    fmt::println(
        R"(
GTK_MAJOR_VERSION   {: 2d} GTK_MINOR_VERSION   {}
GDK_MAJOR_VERSION   {: 2d} GDK_MINOR_VERSION   {}
GTKMM_MAJOR_VERSION {: 2d} GTKMM_MINOR_VERSION {}
)",
        GTK_MAJOR_VERSION, GTK_MINOR_VERSION,
        GDK_MAJOR_VERSION, GDK_MINOR_VERSION,
        GTKMM_MAJOR_VERSION, GTKMM_MINOR_VERSION);

    // TBD app naming conventions, see:
    //   https://docs.gtk.org/gio/type_func.Application.id_is_valid.html
    Glib::RefPtr<Gtk::Application> app {
        Gtk::Application::create("gtkmm4.control_panel")};
    return app->make_window_and_run<ControlPanelWindow>(argc, argv);
}
