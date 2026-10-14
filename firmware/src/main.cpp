#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <time.h>

#include "config.h"
#include "escpos.h"
#include "routes.h"

// WiFi credentials from gitignored secrets.h
#include "secrets.h"

EscPosWriter printer;
AsyncWebServer server(HTTP_PORT);

void setup() {
    Serial.begin(115200);
    Serial.println("\n[receipt-printer] Starting...");

    // Init printer on Serial2
    PRINTER_UART.begin(PRINTER_BAUD, SERIAL_8N1, PRINTER_RX_PIN, PRINTER_TX_PIN);
    printer.begin(PRINTER_UART);

    // Connect WiFi (try primary, then fallback)
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(MDNS_HOSTNAME);

    const char* ssids[] = { WIFI_SSID, WIFI_SSID2 };
    const char* passwords[] = { WIFI_PASSWORD, WIFI_PASSWORD2 };
    const int numNetworks = (strlen(WIFI_SSID2) > 0) ? 2 : 1;

    for (int i = 0; i < numNetworks; i++) {
        Serial.printf("[wifi] Trying %s", ssids[i]);
        WiFi.begin(ssids[i], passwords[i]);

        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) break;
        Serial.printf("[wifi] Failed to connect to %s\n", ssids[i]);
        WiFi.disconnect();
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[wifi] All networks failed - restarting in 5s");
        delay(5000);
        ESP.restart();
    }

    Serial.printf("[wifi] Connected to %s: ", WiFi.SSID().c_str());
    Serial.println(WiFi.localIP());

    // Start mDNS
    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        Serial.printf("[mdns] http://%s.local\n", MDNS_HOSTNAME);
    }

    // NTP time sync
    configTime(UTC_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
    Serial.println("[ntp] Syncing...");

    // ArduinoOTA (enables `pio run -t upload -e ota` over WiFi)
    ArduinoOTA.setHostname(MDNS_HOSTNAME);
    ArduinoOTA.onStart([]() {
        Serial.println("[ota] Update starting...");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[ota] %u%%\r", progress * 100 / total);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[ota] Done, rebooting...");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[ota] Error[%u]\n", error);
    });
    ArduinoOTA.begin();
    Serial.println("[ota] ArduinoOTA ready");

    // HTTP routes
    setupRoutes(server, &printer);
    server.begin();
    Serial.println("[http] Server started on port 80");

    // Let power rail stabilize before printing to avoid brownout reset loop.
    // The 680µF capacitor helps with print bursts but boot-up still needs time.
    delay(2000);

    // Print startup receipt
    printer.setAlign(1);
    printer.setBold(true);
    printer.setFontSize(2, 2);
    printer.printLine("PRINTER ONLINE");
    printer.setFontSize(1, 1);
    printer.setBold(false);
    printer.printLine("");
    printer.printLine(WiFi.localIP().toString().c_str());
    printer.printLine((String(MDNS_HOSTNAME) + ".local").c_str());
    printer.printLine("");
    printer.printLine(("FW " + String(FW_VERSION)).c_str());
    printer.setAlign(0);
    printer.feed(3);
}

void loop() {
    ArduinoOTA.handle();
    delay(10);
}
