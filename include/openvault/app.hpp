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
            return;
        }
        if (screen_ == Screen::Welcome) {
            if (action == Action::Select) screen_ = Screen::Menu;
            return;
        }
        if (screen_ == Screen::Menu) {
            if (action == Action::Next) selected_ = (selected_ + 1) % menu_size;
            else {
                static const Screen screens[] = {Screen::Create, Screen::Import, Screen::Unlock, Screen::Sign, Screen::Diagnostics, Screen::About};
                screen_ = screens[selected_];
            }
        } else if (action == Action::Select) {
            screen_ = Screen::Menu;
        }
    }
    View view() const {
        switch (screen_) {
        case Screen::Welcome:
            return {"OPENVAULT", {"PROTOTIPO 0.1 - SEM FUNDOS", "M5StickC PLUS2", "Tela e botoes em teste", "Nao insira uma seed real.", "Nenhuma carteira e criada."}, "A: iniciar"};
        case Screen::Menu:
            return {"OPENVAULT / MENU", {"", "", "", "", ""}, "A: abrir  B: proximo"};
        case Screen::Create:
            return {"CRIAR CARTEIRA", {"Recurso ainda indisponivel", "Proxima etapa: BIP39", "Entropia precisa validacao", "Nenhuma seed foi gerada.", "Use apenas dados de teste."}, "A: voltar | segure B: menu"};
        case Screen::Import:
            return {"IMPORTAR SEED", {"Entrada de seed desativada", "Planejado: 12/24 palavras", "Checksum BIP39 obrigatorio", "Nao digite segredos reais.", "Nada e salvo em flash."}, "A: voltar | segure B: menu"};
        case Screen::Unlock:
            return {"DESBLOQUEAR", {"Cofre ainda nao implementado", "Nao ha PIN ou seed salvos.", "Criptografia e anti-brute", "force exigem validacao.", "Nenhum segredo neste build."}, "A: voltar | segure B: menu"};
        case Screen::Sign:
            return {"ASSINAR PSBT", {"Assinatura indisponivel", "Futuro: Bitcoin testnet", "Conferir destino e taxa", "Entrada QR requer camera.", "USB usado para gravacao."}, "A: voltar | segure B: menu"};
        case Screen::Diagnostics:
            return {"DIAGNOSTICO", {"Tela: 240 x 135 (paisagem)", "A: selecionar/voltar", "B: avancar; segurar: menu", "Radio: desligado no boot", "C: botao de energia"}, "A: voltar | segure B: menu"};
        case Screen::About:
            return {"SOBRE", {"OpenVault ESP32 / 0.1.0", "Inspirado no SeedSigner", "Firmware proprio ESP32", "Sem garantia de seguranca", "Nao e uma wallet pronta."}, "A: voltar | segure B: menu"};
        }
        return {"ERRO", {"", "", "", "", ""}, ""};
    }
private:
    Screen screen_ = Screen::Welcome;
    std::size_t selected_ = 0;
    Metrics metrics_;
};
} // namespace openvault
