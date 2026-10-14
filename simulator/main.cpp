#include <iostream>
#include <string>
#include "openvault/app.hpp"

void render(const openvault::App& app) {
    const auto view = app.view();
    std::cout << "\n+----------------------------------------+\n" << view.title << " | DEMO\n";
    if (app.screen() == openvault::Screen::Menu) {
        for (std::size_t i = (app.selected() / 3) * 3; i < (app.selected() / 3) * 3 + 3; ++i)
            std::cout << (i == app.selected() ? "> " : "  ") << app.menu_label(i) << '\n';
    } else {
        for (std::size_t row = 0; row < openvault::App::lines_per_page; ++row) {
            const auto i = app.page() * openvault::App::lines_per_page + row;
            if (i < view.lines.size() && *view.lines[i]) std::cout << view.lines[i] << '\n';
        }
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
