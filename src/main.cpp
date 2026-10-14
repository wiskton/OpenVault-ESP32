#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <esp32-hal-bt.h>
#include "openvault/app.hpp"

#ifndef OPENVAULT_DEMO_ONLY
#error "This milestone must remain a UI-only prototype"
#endif

namespace {
openvault::App app;
M5Canvas canvas(&M5.Display);
constexpr uint16_t bg = 0x0842;
constexpr uint16_t muted = 0x9CF3;
constexpr uint16_t green = 0x47F1;
bool canvas_ready = false;
bool held_b = false;
uint32_t last_refresh = 0;

void render() {
    // A small sprite avoids flashing the entire LCD on every button press.
    auto& d = canvas;
    const auto view = app.view();
    d.fillScreen(bg);
    d.setTextSize(1);
    d.fillRect(0, 0, 240, 22, 0x126A);
    d.setTextColor(TFT_WHITE, 0x126A);
    d.setCursor(8, 7);
    d.print(view.title);
    d.setTextColor(TFT_YELLOW, 0x126A);
    d.setCursor(199, 7);
    d.print("DEMO");

    if (app.screen() == openvault::Screen::Menu) {
        for (std::size_t i = 0; i < app.menu_size; ++i) {
            const int y = 26 + i * 14;
            const bool active = i == app.selected();
            if (active) d.fillRoundRect(4, y - 1, 232, 13, 3, green);
            d.setTextColor(active ? bg : TFT_WHITE, active ? green : bg);
            d.setCursor(9, y + 1);
            d.print(active ? "> " : "  ");
            d.print(app.menu_label(i));
        }
    } else {
        d.setTextColor(TFT_WHITE, bg);
        for (std::size_t i = 0; i < view.lines.size(); ++i) {
            d.setCursor(8, 28 + i * 14);
            d.print(view.lines[i]);
        }
        if (app.screen() == openvault::Screen::Diagnostics) {
            d.fillRect(0, 97, 240, 18, bg);
            d.setTextColor(green, bg);
            d.setCursor(8, 100);
            d.printf("A:%lu B:%lu Bat:%d%%", static_cast<unsigned long>(app.metrics().select_presses),
                     static_cast<unsigned long>(app.metrics().next_presses), app.metrics().battery_percent);
        }
    }
    d.drawFastHLine(0, 117, 240, 0x3186);
    d.setTextColor(muted, bg);
    d.setCursor(5, 124);
    d.print(view.footer);
    d.pushSprite(0, 0);
}
} // namespace

void setup() {
    // HOLD is essential on PLUS2 to stay on when the USB cable is disconnected.
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    auto cfg = M5.config();
    cfg.fallback_board = m5::board_t::board_M5StickCPlus2;
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);
    WiFi.mode(WIFI_OFF);
    btStop();
    M5.Display.setRotation(1);
    M5.Display.setBrightness(90);
    canvas.setColorDepth(16);
    canvas_ready = canvas.createSprite(240, 135) != nullptr;
    if (!canvas_ready) {
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setTextColor(TFT_RED);
        M5.Display.print("Erro: sem memoria para UI");
        return;
    }
    app.set_battery(M5.Power.getBatteryLevel());
    render();
    Serial.println("OpenVault 0.1.0: UI-only demo; no wallet or secrets.");
}

void loop() {
    M5.update();
    if (!canvas_ready) { delay(20); return; }
    bool redraw = false;
    if (M5.BtnA.wasPressed()) { app.dispatch(openvault::Action::Select); redraw = true; }
    if (M5.BtnB.pressedFor(700) && !held_b) {
        held_b = true;
        app.dispatch(openvault::Action::Back);
        redraw = true;
    }
    if (M5.BtnB.wasReleased()) {
        if (!held_b) { app.dispatch(openvault::Action::Next); redraw = true; }
        held_b = false;
    }
    if (millis() - last_refresh >= 1000) {
        last_refresh = millis();
        app.set_battery(M5.Power.getBatteryLevel());
        if (app.screen() == openvault::Screen::Diagnostics) redraw = true;
    }
    if (redraw) render();
    delay(10);
}
