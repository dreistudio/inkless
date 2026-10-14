#pragma once

// -- Printer UART --
#ifndef PRINTER_TX_PIN
#define PRINTER_TX_PIN   17    // ESP32 GPIO17 (TX2) → QR204 RX   (s3home env: GPIO5 via build flag)
#endif
#ifndef PRINTER_RX_PIN
#define PRINTER_RX_PIN   16    // ESP32 GPIO16 (RX2) ← QR204 TX (optional; s3home env: GPIO4)
#endif
#define PRINTER_BAUD     9600
// The UART the printer hangs on: UART2 on the classic ESP32, UART1 on the S3 (no Serial2 there)
#if CONFIG_IDF_TARGET_ESP32S3
#define PRINTER_UART     Serial1
#else
#define PRINTER_UART     Serial2
#endif

// -- Network --
#ifndef MDNS_HOSTNAME
#define MDNS_HOSTNAME    "printer"   // printer.local   (s3home env: printer-home)
#endif
#define HTTP_PORT        80

// -- Printer specs --
#define DOTS_PER_LINE    384
#define BYTES_PER_LINE   (DOTS_PER_LINE / 8)  // 48
#define CHARS_PER_LINE   32

// -- Firmware --
#define FW_VERSION       "1.1.0"   // 1.1.0: prints go through inkless-server on the studio Pi (gateway), hostname + IP fallback

// -- NTP --
#define NTP_SERVER       "pool.ntp.org"
#define UTC_OFFSET_SEC   0       // Adjust for your timezone
#define DST_OFFSET_SEC   0
