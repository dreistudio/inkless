#include "routes.h"
#include "web_ui.h"
#include "logo_data.h"
#include "config.h"
#include "secrets.h"
#include <ArduinoJson.h>
#include <mbedtls/base64.h>
#include <Update.h>
#include <time.h>

static EscPosWriter *g_printer = nullptr;

static String getFormattedDateTime() {
    time_t now;
    time(&now);
    struct tm *ti = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M", ti);
    return String(buf);
}

// Accumulate body chunks into a String stored on the heap.
// We use a String* stored in _tempObject to avoid conflicts with
// ESPAsyncWebServer's internal use of _tempObject for param parsing.
struct BodyBuffer {
    uint8_t *data;
    size_t total;
};

static void accumulateBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    BodyBuffer *buf;
    if (index == 0) {
        buf = new BodyBuffer();
        buf->data = (uint8_t *)malloc(total + 1);
        buf->total = total;
        if (!buf->data) { delete buf; return; }
        request->_tempObject = buf;
    }
    buf = (BodyBuffer *)request->_tempObject;
    if (!buf || !buf->data) return;
    memcpy(buf->data + index, data, len);
    if (index + len == total) {
        buf->data[total] = '\0';
    }
}

static char *getBody(AsyncWebServerRequest *request) {
    BodyBuffer *buf = (BodyBuffer *)request->_tempObject;
    if (!buf || !buf->data) return nullptr;
    return (char *)buf->data;
}

static void freeBody(AsyncWebServerRequest *request) {
    BodyBuffer *buf = (BodyBuffer *)request->_tempObject;
    if (buf) {
        free(buf->data);
        delete buf;
        request->_tempObject = nullptr;
    }
}

void setupRoutes(AsyncWebServer &server, EscPosWriter *printer) {
    g_printer = printer;

    // --- GET / : Web UI ---
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        response->printf("<script>window.INKLESS_SERVER='%s';window.INKLESS_SERVER_FALLBACK='%s';</script>", INKLESS_SERVER_URL, INKLESS_SERVER_URL_FALLBACK);
        response->print(FPSTR(WEB_UI_HTML));
        request->send(response);
    });

    // --- GET /status ---
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["status"] = "online";
        doc["firmware"] = FW_VERSION;
        doc["hostname"] = MDNS_HOSTNAME;
        doc["uptime_s"] = millis() / 1000;
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // --- POST /print/test : diagnostic solid black bar ---
    server.on("/print/test", HTTP_POST, [printer](AsyncWebServerRequest *request) {
        printer->printTestPattern();
        printer->feed(1);
        request->send(200, "application/json", "{\"status\":\"printed\",\"method\":\"test\"}");
    });

    // --- POST /print/logo/raster : row-by-row raster from PROGMEM ---
    server.on("/print/logo/raster", HTTP_POST, [](AsyncWebServerRequest *request) {
        xTaskCreate([](void *param) {
            if (g_printer) {
                g_printer->lock();
                g_printer->printBitImageProgmem(LOGO_BITMAP, LOGO_W, LOGO_H);
                g_printer->unlock();
            }
            vTaskDelete(NULL);
        }, "logo_r", 4096, NULL, 1, NULL);
        request->send(200, "application/json", "{\"status\":\"printed\",\"method\":\"raster\"}");
    });

    // --- POST /print/logo/inverted : inverted bits ---
    server.on("/print/logo/inverted", HTTP_POST, [](AsyncWebServerRequest *request) {
        xTaskCreate([](void *param) {
            if (g_printer) {
                g_printer->lock();
                g_printer->printBitImageProgmemInverted(LOGO_BITMAP, LOGO_W, LOGO_H);
                g_printer->feed(1);
                g_printer->unlock();
            }
            vTaskDelete(NULL);
        }, "logo_inv", 4096, NULL, 1, NULL);
        request->send(200, "application/json", "{\"status\":\"printed\",\"method\":\"inverted\"}");
    });

    // --- POST /submit : Scribe-compatible endpoint ---
    server.on("/submit", HTTP_POST, [printer](AsyncWebServerRequest *request) {
        String message;
        String date;

        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
        }
        if (request->hasParam("date", true)) {
            date = request->getParam("date", true)->value();
        }

        if (message.isEmpty()) {
            request->send(400, "text/plain", "Missing 'message' parameter");
            return;
        }

        if (date.isEmpty()) {
            date = getFormattedDateTime();
        }

        // Copy strings for the task (request object won't survive)
        struct SubmitData { String message; String date; };
        SubmitData *sd = new SubmitData{message, date};

        xTaskCreate([](void *param) {
            SubmitData *sd = (SubmitData *)param;
            if (g_printer) {
                g_printer->lock();
                g_printer->setAlign(1);
                g_printer->setInverse(true);
                g_printer->printLine((" " + sd->date + " ").c_str());
                g_printer->setInverse(false);
                g_printer->setAlign(0);
                g_printer->printLine("");
                g_printer->printWrappedReversed(sd->message.c_str());
                g_printer->feed(4);
                g_printer->unlock();
            }
            Serial.println("[submit] " + sd->message);
            delete sd;
            vTaskDelete(NULL);
        }, "submit", 8192, sd, 1, NULL);

        request->send(200, "text/plain", "Printed!");
    });

    // --- POST /print/text ---
    server.on("/print/text", HTTP_POST,
        [printer](AsyncWebServerRequest *request) {
            char *body = getBody(request);
            if (!body) {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
                return;
            }

            JsonDocument doc;
            if (deserializeJson(doc, body, strlen(body))) {
                freeBody(request);
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            const char *text = doc["text"] | "";
            if (strlen(text) == 0) {
                freeBody(request);
                request->send(400, "application/json", "{\"error\":\"Missing 'text'\"}");
                return;
            }

            struct TextData {
                String text;
                bool bold, underline;
                uint8_t align, fontW, fontH;
            };
            TextData *td = new TextData{
                String(text),
                doc["bold"] | false,
                doc["underline"] | false,
                doc["align"] | 0,
                doc["font_width"] | 1,
                doc["font_height"] | 1
            };

            freeBody(request);

            xTaskCreate([](void *param) {
                TextData *td = (TextData *)param;
                if (g_printer) {
                    g_printer->lock();
                    g_printer->setAlign(td->align);
                    g_printer->setBold(td->bold);
                    g_printer->setUnderline(td->underline);
                    g_printer->setFontSize(td->fontW, td->fontH);
                    g_printer->printLine(td->text.c_str());
                    g_printer->setBold(false);
                    g_printer->setUnderline(false);
                    g_printer->setAlign(0);
                    g_printer->setFontSize(1, 1);
                    g_printer->feed(1);
                    g_printer->unlock();
                }
                delete td;
                vTaskDelete(NULL);
            }, "text", 8192, td, 1, NULL);

            request->send(200, "application/json", "{\"status\":\"printed\"}");
        },
        NULL,
        accumulateBody
    );

    // --- POST /print/receipt ---
    server.on("/print/receipt", HTTP_POST,
        [printer](AsyncWebServerRequest *request) {
            char *body = getBody(request);
            if (!body) {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
                return;
            }

            JsonDocument doc;
            if (deserializeJson(doc, body, strlen(body))) {
                freeBody(request);
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            const char *title = doc["title"] | "RECEIPT";

            printer->setAlign(1);
            printer->setBold(true);
            printer->setFontSize(2, 2);
            printer->printLine(title);
            printer->setFontSize(1, 1);
            printer->setBold(false);

            printer->printLine(getFormattedDateTime().c_str());
            printer->setAlign(0);
            printer->printLine("--------------------------------");

            JsonArray items = doc["items"].as<JsonArray>();
            for (JsonObject item : items) {
                const char *name = item["name"] | "";
                const char *price = item["price"] | "";
                int nameLen = strlen(name);
                int priceLen = strlen(price);
                int padding = max(1, CHARS_PER_LINE - nameLen - priceLen);
                String line = String(name);
                for (int i = 0; i < padding; i++) line += ' ';
                line += price;
                printer->printLine(line.c_str());
            }

            printer->printLine("--------------------------------");

            const char *total_str = doc["total"] | "";
            if (strlen(total_str) > 0) {
                printer->setBold(true);
                String totalLine = "TOTAL";
                int padding = max(1, CHARS_PER_LINE - 5 - (int)strlen(total_str));
                for (int i = 0; i < padding; i++) totalLine += ' ';
                totalLine += total_str;
                printer->printLine(totalLine.c_str());
                printer->setBold(false);
            }

            const char *footer = doc["footer"] | "";
            if (strlen(footer) > 0) {
                printer->printLine("");
                printer->setAlign(1);
                printer->printLine(footer);
                printer->setAlign(0);
            }

            printer->feed(3);
            freeBody(request);
            request->send(200, "application/json", "{\"status\":\"printed\"}");
        },
        NULL,
        accumulateBody
    );

    // --- POST /print/image ---
    server.on("/print/image", HTTP_POST,
        [printer](AsyncWebServerRequest *request) {
            char *body = getBody(request);
            if (!body) {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
                return;
            }

            JsonDocument doc;
            if (deserializeJson(doc, body, strlen(body))) {
                freeBody(request);
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            const char *b64 = doc["data"] | "";
            uint16_t width = doc["width"] | 384;
            uint16_t height = doc["height"] | 0;

            if (strlen(b64) == 0 || height == 0) {
                freeBody(request);
                request->send(400, "application/json", "{\"error\":\"Missing 'data' or 'height'\"}");
                return;
            }

            size_t b64Len = strlen(b64);
            size_t outLen = 0;
            mbedtls_base64_decode(NULL, 0, &outLen, (const uint8_t *)b64, b64Len);

            uint8_t *bitmap = (uint8_t *)malloc(outLen);
            if (!bitmap) {
                freeBody(request);
                request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
                return;
            }

            size_t decoded = 0;
            mbedtls_base64_decode(bitmap, outLen, &decoded, (const uint8_t *)b64, b64Len);

            freeBody(request);

            struct ImgData { uint8_t *bitmap; uint16_t width; uint16_t height; };
            ImgData *id = new ImgData{bitmap, width, height};

            xTaskCreate([](void *param) {
                ImgData *id = (ImgData *)param;
                if (g_printer) {
                    g_printer->lock();
                    g_printer->printRasterBitmap(id->bitmap, id->width, id->height);
                    g_printer->feed(2);
                    g_printer->unlock();
                }
                free(id->bitmap);
                delete id;
                vTaskDelete(NULL);
            }, "img", 8192, id, 1, NULL);

            request->send(200, "application/json", "{\"status\":\"printed\"}");
        },
        NULL,
        accumulateBody
    );

    // --- POST /print/qrcode ---
    server.on("/print/qrcode", HTTP_POST,
        [printer](AsyncWebServerRequest *request) {
            char *body = getBody(request);
            if (!body) {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
                return;
            }

            JsonDocument doc;
            if (deserializeJson(doc, body, strlen(body))) {
                freeBody(request);
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            const char *qrData = doc["data"] | "";
            const char *label = doc["label"] | "";
            uint8_t size = doc["size"] | 4;

            if (strlen(qrData) == 0) {
                freeBody(request);
                request->send(400, "application/json", "{\"error\":\"Missing 'data'\"}");
                return;
            }

            struct QRData { String data; String label; uint8_t size; };
            QRData *qd = new QRData{String(qrData), String(label), size};
            freeBody(request);

            xTaskCreate([](void *param) {
                QRData *qd = (QRData *)param;
                if (g_printer) {
                    g_printer->lock();
                    g_printer->setAlign(1);
                    g_printer->printQRCode(qd->data.c_str(), qd->size);
                    if (qd->label.length() > 0) {
                        g_printer->printLine(qd->label.c_str());
                    }
                    g_printer->setAlign(0);
                    g_printer->feed(2);
                    g_printer->unlock();
                }
                delete qd;
                vTaskDelete(NULL);
            }, "qr", 4096, qd, 1, NULL);

            request->send(200, "application/json", "{\"status\":\"printed\"}");
        },
        NULL,
        accumulateBody
    );

    // --- POST /print/raw --- send raw ESC/POS bytes (base64-encoded)
    server.on("/print/raw", HTTP_POST,
        [printer](AsyncWebServerRequest *request) {
            char *body = getBody(request);
            if (!body) {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
                return;
            }

            JsonDocument doc;
            if (deserializeJson(doc, body, strlen(body))) {
                freeBody(request);
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            const char *b64 = doc["data"] | "";
            if (strlen(b64) == 0) {
                freeBody(request);
                request->send(400, "application/json", "{\"error\":\"Missing 'data'\"}");
                return;
            }

            // Decode base64
            size_t b64Len = strlen(b64);
            size_t outLen = 0;
            mbedtls_base64_decode(NULL, 0, &outLen, (const uint8_t *)b64, b64Len);

            uint8_t *raw = (uint8_t *)malloc(outLen);
            if (!raw) {
                freeBody(request);
                request->send(500, "application/json", "{\"error\":\"Out of memory\"}");
                return;
            }

            size_t decoded = 0;
            mbedtls_base64_decode(raw, outLen, &decoded, (const uint8_t *)b64, b64Len);

            printer->sendRaw(raw, decoded);
            free(raw);
            freeBody(request);
            request->send(200, "application/json", "{\"status\":\"sent\"}");
        },
        NULL,
        accumulateBody
    );

    // --- POST /feed ---
    server.on("/feed", HTTP_POST, [printer](AsyncWebServerRequest *request) {
        uint8_t lines = 3;
        if (request->hasParam("lines", true)) {
            lines = request->getParam("lines", true)->value().toInt();
        }
        printer->feed(lines);
        request->send(200, "application/json", "{\"status\":\"fed\"}");
    });

    // --- POST /cut ---
    server.on("/cut", HTTP_POST, [printer](AsyncWebServerRequest *request) {
        printer->cut();
        request->send(200, "application/json", "{\"status\":\"cut\"}");
    });

    // --- GET /update : OTA firmware update form ---
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html",
            "<!DOCTYPE html><html><body>"
            "<h2>Firmware Update</h2>"
            "<form method='POST' action='/update' enctype='multipart/form-data'>"
            "<input type='file' name='firmware' accept='.bin'><br><br>"
            "<input type='submit' value='Upload'>"
            "</form></body></html>");
    });

    // --- POST /update : OTA firmware upload ---
    server.on("/update", HTTP_POST,
        // Response handler (called after upload completes)
        [](AsyncWebServerRequest *request) {
            bool success = !Update.hasError();
            request->send(200, "text/plain", success ? "OK. Rebooting..." : "FAIL");
            if (success) {
                delay(500);
                ESP.restart();
            }
        },
        // Upload handler (called per chunk)
        [](AsyncWebServerRequest *request, const String &filename, size_t index,
           uint8_t *data, size_t len, bool final) {
            if (index == 0) {
                Serial.printf("[ota-http] Begin: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }
            if (!Update.hasError()) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
            }
            if (final) {
                if (Update.end(true)) {
                    Serial.printf("[ota-http] Success: %u bytes\n", index + len);
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );
}
