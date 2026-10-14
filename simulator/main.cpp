#include <iostream>
#include <string>
#include "openvault/app.hpp"

void render(const openvault::App& app) {
    const auto view = app.view();
    std::cout << "\n+----------------------------------------+\n" << view.title << " | DEMO\n";
    if (app.screen() == openvault::Screen::Menu) {
        for (std::size_t i = 0; i < app.menu_size; ++i)
            std::cout << (i == app.selected() ? "> " : "  ") << app.menu_label(i) << '\n';
    } else {
        for (auto line : view.lines) if (*line) std::cout << line << '\n';
        if (app.screen() == openvault::Screen::Diagnostics)
            std::cout << "A:" << app.metrics().select_presses << " B:" << app.metrics().next_presses << " Bateria: simulada\n";
    }
    std::cout << view.footer << "\n[a] A  [b] B  [v] segurar B  [q] sair\n> " << std::flush;
}
int main() {
    openvault::App app;
    std::string line;
    render(app);
    while (std::getline(std::cin, line)) {
        if (line == "q") break;
        if (line == "a") app.dispatch(openvault::Action::Select);
        else if (line == "b") app.dispatch(openvault::Action::Next);
        else if (line == "v") app.dispatch(openvault::Action::Back);
        render(app);
    }
}
