# AlgoLaser DKM1 — Controller Periferiche con Arduino Uno R4 WiFi

## Stato del progetto

| Componente | Stato |
|------------|-------|
| Firmware laser | AlgoLaser GrblHAL 1.1f (originale) |
| WiFi laser (AP mode) | ALDKM1_AP_9F60 @ 192.168.5.1 |
| Rilevamento stato lavoro via Telnet | Operativo |
| Arduino Uno R4 WiFi controller periferiche | Sketch v6 pronto |

---

## Hardware

| Componente | Dettaglio |
|------------|-----------|
| MCU laser | ESP32-S3R8 (240 MHz, 8 MB PSRAM) |
| Firmware | AlgoLaser GrblHAL 1.1f (AL_DKM1_G1_119) |
| WiFi laser | AP mode — SSID: `ALDKM1_AP_9F60` — Password: `12345678` |
| IP laser (AP) | `192.168.5.1` |
| Telnet laser | porta `23` |
| Controller periferiche | Arduino Uno R4 WiFi |

---

## Architettura

```
LightBurn
    │  G-code via USB (COM5)
    ▼
AlgoLaser GrblHAL (ESP32-S3)
    │  WiFi AP: ALDKM1_AP_9F60 @ 192.168.5.1
    │  Telnet port 23 — comando "?"
    ▼
Arduino Uno R4 WiFi
    │  polling ogni 500ms
    ├── PIN 11 → Elettrovalvola aria compressa
    ├── PIN 12 → Semaforo
    ├── PIN 8  → Aspirazione fumi
    └── PIN 7  → LED illuminazione area lavoro
```

**Logica di rilevamento stato:**
- Arduino si connette all'AP della laser (`ALDKM1_AP_9F60`)
- Polling ogni 500ms via Telnet (porta 23) con comando `?`
- Risposta `<Run|...>` → LAVORO (tutte le periferiche ON)
- Risposta `<Idle|...>` → PAUSA (periferiche OFF, aspirazione rimane 3 min)
- Nessuna modifica al G-code LightBurn necessaria

---

## Comportamento

### LAVORO (laser in esecuzione)
- Elettrovalvola aria compressa → **ON**
- Semaforo → **ON**
- Aspirazione fumi → **ON**
- LED area lavoro → **OFF**

### PAUSA (laser ferma)
- Elettrovalvola → **OFF** immediato
- Semaforo → **OFF** immediato
- Aspirazione → **OFF** dopo 3 minuti
- LED → **ON** (dopo 3 minuti se nessun boost attivo)

---

## File nel repository

```
arduino_controller/
└── algolaser_controller.ino  ← Sketch Arduino Uno R4 WiFi (v6)

fluidnc/
└── config.yaml               ← Configurazione FluidNC (non più in uso)
```

---

## Configurazione iniziale laser (dopo ogni power cycle)

Il firmware AlgoLaser resetta il WiFi mode ad ogni riavvio.
Per attivare l'AP, dalla console LightBurn inviare:

```
$73=3
$WRS
```

Questo mette la laser in modalità AP+Station e attiva il Telnet.
L'Arduino si riconnetterà automaticamente.

> **Tip**: aggiungere `$73=3` e `$WRS` allo Start GCode di LightBurn
> (Device Settings → GCode → Start GCode) per automatizzare questo passaggio.

---

## Arduino Uno R4 WiFi — Comandi seriali (115200 baud)

| Comando | Funzione |
|---------|----------|
| `MODE=MANUAL` | Disabilita polling laser, controllo manuale |
| `MODE=AUTO` | Riabilita polling automatico |
| `LED=BOOST` | Forza LED ON per 5 minuti |
| `LED=BOOST_OFF` | Annulla boost LED |
| `LED=ON` / `LED=OFF` | Controllo LED (solo MANUAL) |
| `EV=ON` / `EV=OFF` | Elettrovalvola (solo MANUAL) |
| `ASP=ON` / `ASP=OFF` | Aspirazione (solo MANUAL) |
| `RESET` | Riavvio Arduino |

---

## PIN Arduino

| PIN | Periferica | Logica |
|-----|------------|--------|
| 11 | Elettrovalvola aria compressa | HIGH = ON |
| 12 | Semaforo | HIGH = ON |
| 8 | Aspirazione fumi | HIGH = ON |
| 7 | LED area lavoro | HIGH = ON |

---

## Riferimenti

- [AlgoLaser DKM1](https://algolaser.com)
- [GrblHAL](https://github.com/grblHAL)
- [Arduino Uno R4 WiFi — WiFiS3](https://docs.arduino.cc/hardware/uno-r4-wifi/)
