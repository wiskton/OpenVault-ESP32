#include <cassert>
#include <cstring>
#include <iostream>
#include "openvault/app.hpp"
using namespace openvault;
int main() {
    App app;
    assert(app.screen() == Screen::Welcome);
    app.dispatch(Action::Next);
    assert(app.screen() == Screen::Welcome); // Explicit acknowledgment is required.
    app.dispatch(Action::Select);
    assert(app.screen() == Screen::Menu);
    assert(app.selected() == 0);
    const Screen expected[] = {Screen::Create, Screen::Import, Screen::Unlock, Screen::Sign, Screen::Diagnostics, Screen::About};
    for (std::size_t i = 0; i < App::menu_size; ++i) {
        assert(app.selected() == i);
        assert(std::strlen(app.menu_label(i)) > 0);
        app.dispatch(Action::Select);
        assert(app.screen() == expected[i]);
        for (auto line : app.view().lines) assert(std::strlen(line) <= 18);
        app.dispatch(Action::Back);
        assert(app.screen() == Screen::Menu);
        assert(app.selected() == i);
        app.dispatch(Action::Next);
    }
    assert(app.selected() == 0); // Navigation wraps safely.
    app.dispatch(Action::Select);
    assert(app.page_count() == 2);
    assert(app.page() == 0);
    app.dispatch(Action::Next);
    assert(app.page() == 1);
    app.dispatch(Action::Next);
    assert(app.page() == 0);
    app.dispatch(Action::Back);
    assert(app.page() == 0);
    assert(app.screen() == Screen::Menu);
    app.dispatch(Action::Select);
    app.dispatch(Action::Select);
    assert(app.screen() == Screen::Menu);
    assert(std::strcmp(app.menu_label(999), "") == 0);
    app.set_battery(76);
    assert(app.metrics().battery_percent == 76);
    assert(app.metrics().next_presses == 9);
    std::cout << "OK: aviso inicial, seis telas, navegacao, retorno, metricas e limites de texto.\n";
}
