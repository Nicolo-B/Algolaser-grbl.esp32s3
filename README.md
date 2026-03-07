# AlgoLaser DKM1 — Grbl_ESP32 / FluidNC su ESP32-S3R8

## Contesto e obiettivo

Questo repository nasce dall'esigenza di sostituire il firmware proprietario di una **AlgoLaser DKM1** (scheda ESP32-S3R8, 8 MB PSRAM, MAC `24:EC:4A:0E:D0:6C`) con un firmware open-source basato su **Grbl**, estendibile e integrabile in un flusso di lavoro aziendale.

### Percorso intrapreso

| Fase | Firmware | Stato |
|------|----------|-------|
| 1 | **Grbl_ESP32** (fork bdring) adattato a ESP32-S3 | ✅ Funzionante — base di partenza |
| 2 | Migrazione a **FluidNC** (successore ufficiale di Grbl_ESP32) | 🔜 Obiettivo primario |

Il firmware attuale (Grbl_ESP32 patchato) è il punto di partenza documentato in questo repo. La direzione finale è **FluidNC**, che supporta nativamente ESP32-S3, configurazione YAML e API HTTP/WebSocket più ricche.

---

## Obiettivi del progetto

### 1. Interfaccia aziendale personalizzata
Sostituire la WebUI generica (ESP3D) con un'interfaccia su misura che esponga solo le funzioni rilevanti per il workflow aziendale: avvio job, monitoraggio stato, gestione periferiche.

### 2. Controllo periferiche via M8 / M9 (Arduino slave)
Il punto critico del progetto è **inviare i codici M8 e M9 via API** al firmware, affinché questo li propaghi a un **Arduino secondario** collegato alla scheda, responsabile della gestione delle periferiche:

| Codice G | Azione | Periferica |
|----------|--------|------------|
| `M8`     | ON     | Elettrovalvola aria compressa |
| `M8`     | ON     | Aspirazione fumi |
| `M8`     | ON     | LED illuminazione area lavoro |
| `M9`     | OFF    | Tutte le periferiche sopra |

Il flusso previsto:
```
App / API HTTP
     │
     ▼
FluidNC (ESP32-S3)  ──── WebSocket / UART ────▶  Arduino Slave
     │                                                  │
     │  M8 / M9 via G-code stream                      │
     │                                                  ▼
     └──────────────────────────────────▶  Elettrovalvola / Aspirazione / LED
```

### 3. API HTTP per integrazione esterna
Inviare comandi G-code (inclusi M8/M9) tramite chiamate HTTP semplici, integrabili da qualsiasi sistema gestionale o script aziendale:

```bash
# Esempio — attiva periferiche (M8)
curl "http://192.168.1.23/command?commandText=M8"

# Esempio — disattiva periferiche (M9)
curl "http://192.168.1.23/command?commandText=M9"
```

---

## Hardware

| Componente | Dettaglio |
|------------|-----------|
| MCU | ESP32-S3R8 (Xtensa LX7 dual-core, 240 MHz) |
| PSRAM | 8 MB OPI integrata (AP_3v3) |
| Flash | 16 MB |
| WiFi | 802.11 b/g/n — IP statico `192.168.1.23` |
| WebUI | ESP3D v2.1.3b0 — porta 80 |
| WebSocket | porta 81 (protocollo `arduino`) |
| Laser | PWM spindle (AlgoLaser DKM1) |
| GPIO5 | Power Control — deve restare OUTPUT HIGH |

---

## Fix critici apportati

### GPIO5 — Power Control (bug hardware)
La configurazione originale impostava GPIO5 come `INPUT_PULLUP` (pulsante). In realtà GPIO5 è il pin di **Power Control** che alimenta i driver stepper e il laser PSU: deve stare `OUTPUT HIGH`. Con la configurazione errata il device entrava in deep sleep 3 secondi dopo il boot in un loop infinito.

```cpp
// algolaser_custom.h
void machine_init() {
    pinMode(ALGOLASER_BTN_PIN, OUTPUT);   // GPIO5
    digitalWrite(ALGOLASER_BTN_PIN, HIGH);
}
```

### Partizioni NVS
Il tipo NVS per `Radio/Mode` deve essere `i8` (non `i32`) — Grbl_ESP32 usa `nvs_get_i8` per `EnumSetting`.

---

## Struttura del repository

```
Grbl_Esp32/
├── src/
│   ├── Custom/
│   │   └── algolaser_custom.h      ← machine_init() GPIO5 fix
│   ├── Machines/
│   │   └── algolaser.h             ← definizione assi, pin, spindle
│   ├── Config.h                    ← target ESP32-S3
│   ├── Machine.h
│   ├── I2SOut.cpp / I2SOut_stub.cpp
│   ├── Serial.cpp
│   ├── Stepper.cpp
│   └── Spindles/
│       └── ...                     ← patch compatibilità S3
└── data/
    ├── index.html.gz               ← ESP3D WebUI v2.1.3b0 (produzione)
    ├── images/                     ← jog dial SVG
    └── tools/                      ← template strumenti
platformio.ini                      ← board esp32-s3, partizioni, upload
```

---

## Roadmap

- [ ] Migrazione a **FluidNC** (YAML config, supporto nativo S3)
- [ ] Implementazione handler M8/M9 con segnale verso Arduino slave
- [ ] Protocollo UART/I2C Arduino slave per controllo periferiche
- [ ] WebUI aziendale personalizzata (React o Vue, build statica su SPIFFS)
- [ ] API REST documentata per integrazione gestionale
- [ ] Test elettrovalvola aria compressa
- [ ] Test aspirazione fumi
- [ ] Test LED area lavoro

---

## Riferimenti

- [FluidNC](https://github.com/bdring/FluidNC) — successore ufficiale, obiettivo migrazione
- [Grbl_ESP32](https://github.com/bdring/Grbl_Esp32) — base di partenza
- [ESP3D WebUI](https://github.com/luc-github/ESP3D-WEBUI) — interfaccia web v2.1
