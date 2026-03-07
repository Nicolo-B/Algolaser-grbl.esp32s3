# AlgoLaser DKM1 — FluidNC su ESP32-S3R8

## Stato del progetto

| Componente | Stato |
|------------|-------|
| FluidNC su ESP32-S3 (IP fisso `192.168.1.23`) | ✅ Operativo |
| WebUI FluidNC (ESP3D) | ✅ Operativa |
| Rilevamento M8/M9 via Telnet | ✅ Funzionante |
| Arduino Uno R4 WiFi controller periferiche | ✅ Sketch pronto |
| GPIO5 Power Control (avvio stepper/PSU) | ✅ Configurato via macro |

---

## Hardware

| Componente | Dettaglio |
|------------|-----------|
| MCU laser | ESP32-S3R8 (240 MHz, 8 MB PSRAM) |
| Flash | 8 MB |
| Firmware | FluidNC v4.x (YAML config) |
| IP statico | `192.168.1.23` |
| Controller periferiche | Arduino Uno R4 WiFi — IP `192.168.1.24` |
| Laser | PWM su GPIO47, enable GPIO48 |
| Assi | X (GPIO6/7, limit GPIO18) — Y (GPIO16/17, limit GPIO8) |
| Stepper disable | GPIO15 |
| Power control | GPIO5 (OUTPUT HIGH — via macro `M64P0`) |

---

## Architettura

```
LightBurn
    │  G-code (M8/M9, G0/G1, S, ecc.)
    ▼
FluidNC (ESP32-S3 @ 192.168.1.23)
    │  Telnet port 23 — $G modal state
    ▼
Arduino Uno R4 WiFi (@ 192.168.1.24)
    │  polling ogni 500ms
    ├── PIN 11 → Elettrovalvola aria compressa
    ├── PIN 12 → Semaforo
    ├── PIN 8  → Aspirazione fumi
    └── PIN 7  → LED illuminazione area lavoro
```

**Logica di rilevamento M8/M9:**
- Arduino si connette a FluidNC via Telnet (porta 23), invia `$G`
- Risposta `[GC:... M8 ...]` → LAVORO (tutte le periferiche ON)
- Risposta `[GC:... M9 ...]` → PAUSA (periferiche OFF, aspirazione rimane 3 min)
- Non richiede WebSocket/browser aperto

---

## File nel repository

```
fluidnc/
├── config.yaml               ← Configurazione macchina FluidNC
└── platformio_override.ini   ← Build PlatformIO per ESP32-S3 (python3 fix)

arduino_controller/
└── algolaser_controller.ino  ← Sketch Arduino Uno R4 WiFi
```

---

## FluidNC — Note di configurazione

### Filesystem SPIFFS
Il filesystem (LittleFS) contiene:
- `config.yaml` — configurazione macchina
- `index.html.gz` — WebUI ESP3D
- `preferences.json`, `preferences2.json` — preferenze WebUI (vuoti `{}`)
- `macrocfg.json` — macro WebUI (vuoto `[]`)

### Partizioni
Flash 8 MB: `app3M_spiffs1M_8MB.csv` (firmware 3 MB + SPIFFS 1 MB a `0x610000`)

### GPIO5 — Power Control
GPIO5 deve restare HIGH per alimentare stepper e PSU laser.
Configurato tramite `user_outputs: digital0_pin: gpio.5` e macro:
```yaml
macros:
  after_reset: M64P0
  after_homing: M64P0
```

### Telnet FluidNC (porta 23)
FluidNC espone un'interfaccia Telnet raw sulla porta 23.
Comandi utili:
```bash
echo "?" | nc 192.168.1.23 23      # status report
echo '$G' | nc 192.168.1.23 23     # modal state GCode
echo "M8" | nc 192.168.1.23 23     # attiva coolant flood
echo "M9" | nc 192.168.1.23 23     # disattiva coolant
echo '$X' | nc 192.168.1.23 23     # sblocca allarme
```

---

## Arduino Uno R4 WiFi — Comandi seriali (115200 baud)

| Comando | Funzione |
|---------|----------|
| `MODE=MANUAL` | Disabilita polling FluidNC, controllo manuale |
| `MODE=AUTO` | Riabilita polling automatico |
| `LED=BOOST` | Forza LED ON per 5 minuti |
| `LED=BOOST_OFF` | Annulla boost LED |
| `LED=ON` / `LED=OFF` | (solo MANUAL) |
| `EV=ON` / `EV=OFF` | Elettrovalvola (solo MANUAL) |
| `ASP=ON` / `ASP=OFF` | Aspirazione (solo MANUAL) |
| `RESET` | Riavvio Arduino |

---

## Build e flash FluidNC

```bash
cd FluidNC-main
cp /path/to/platformio_override.ini .

# Flash firmware
python3 -m platformio run -e wifi_s3 --target upload

# Flash filesystem (config.yaml + WebUI)
python3 -m platformio run -e wifi_s3 --target uploadfs
```

---

## Riferimenti

- [FluidNC](https://github.com/bdring/FluidNC)
- [Arduino Uno R4 WiFi — WiFiS3](https://docs.arduino.cc/hardware/uno-r4-wifi/)
