#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace openvault {
// UI-only milestone: this module cannot accept, generate or persist wallet secrets.
enum class Screen { Welcome, Menu, Create, Import, Unlock, Sign, Diagnostics, About };
enum class Action { Next, Select, Back };
struct Metrics {
    std::uint32_t select_presses = 0;
    std::uint32_t next_presses = 0;
    int battery_percent = -1;
};
struct View {
    const char* title;
    std::array<const char*, 5> lines;
    const char* footer;
};

class App {
public:
    static constexpr std::size_t menu_size = 6;
    Screen screen() const { return screen_; }
    std::size_t page() const { return page_; }
    static constexpr std::size_t lines_per_page = 3;
    std::size_t page_count() const {
        std::size_t count = 0;
        for (auto line : view().lines) if (*line) ++count;
        return count == 0 ? 1 : (count + lines_per_page - 1) / lines_per_page;
    }
    std::size_t selected() const { return selected_; }
    const Metrics& metrics() const { return metrics_; }
    void set_battery(int value) { metrics_.battery_percent = value; }
    const char* menu_label(std::size_t index) const {
        static const char* labels[] = {"Criar carteira", "Importar seed", "Desbloquear", "Assinar PSBT", "Diagnostico", "Sobre o projeto"};
        return index < menu_size ? labels[index] : "";
    }
    void dispatch(Action action) {
        if (action == Action::Next) ++metrics_.next_presses;
        if (action == Action::Select) ++metrics_.select_presses;
        if (action == Action::Back) {
            if (screen_ != Screen::Welcome) screen_ = Screen::Menu;
            page_ = 0;
            return;
        }
        if (screen_ != Screen::Menu && action == Action::Next) {
            page_ = (page_ + 1) % page_count();
            return;
        }
        if (screen_ == Screen::Welcome) {
            if (action == Action::Select) { screen_ = Screen::Menu; page_ = 0; }
            return;
        }
        if (screen_ == Screen::Menu) {
            if (action == Action::Next) selected_ = (selected_ + 1) % menu_size;
            else {
                static const Screen screens[] = {Screen::Create, Screen::Import, Screen::Unlock, Screen::Sign, Screen::Diagnostics, Screen::About};
                screen_ = screens[selected_];
                page_ = 0;
            }
        } else if (action == Action::Select) {
            screen_ = Screen::Menu;
            page_ = 0;
        }
    }
    View view() const {
        switch (screen_) {
        case Screen::Welcome:
            return {"OPENVAULT", {"PROTOTIPO 0.1.1", "Nao cria carteira.", "Nao use seed real.", "Tela e botoes", "em validacao."}, "A:inicia B:pagina"};
        case Screen::Menu:
            return {"MENU", {"", "", "", "", ""}, "A:abrir B:proximo"};
        case Screen::Create:
            return {"CRIAR CARTEIRA", {"Nao implementado.", "Nenhuma carteira", "foi criada.", "Proxima etapa:", "BIP39 de teste."}, "A:volta B:pagina"};
        case Screen::Import:
            return {"IMPORTAR SEED", {"Nao implementado.", "Nao digite seed.", "Nada sera salvo.", "Planejado:", "12 ou 24 palavras."}, "A:volta B:pagina"};
        case Screen::Unlock:
            return {"DESBLOQUEAR", {"Nao implementado.", "Nao ha cofre", "ou PIN salvos.", "Persistencia em", "desenvolvimento."}, "A:volta B:pagina"};
        case Screen::Sign:
            return {"ASSINAR PSBT", {"Nao implementado.", "Nao ha assinatura.", "Sem transacoes.", "Planejado:", "Bitcoin testnet."}, "A:volta B:pagina"};
        case Screen::Diagnostics:
            return {"DIAGNOSTICO", {"Contador A", "Contador B", "Bateria", "", ""}, "A:voltar"};
        case Screen::About:
            return {"SOBRE", {"OpenVault 0.1.1", "M5StickC PLUS2", "Prototipo de UI", "Inspirado no", "SeedSigner."}, "A:volta B:pagina"};
        }
        return {"ERRO", {"", "", "", "", ""}, ""};
    }
private:
    Screen screen_ = Screen::Welcome;
    std::size_t selected_ = 0;
    std::size_t page_ = 0;
    Metrics metrics_;
};
} // namespace openvault
