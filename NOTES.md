# Note Tecniche — AlgoLaser DKM1 + Arduino Controller

## Configurazione Hardware

| Componente | Dettaglio |
|------------|-----------|
| Laser MCU | ESP32-S3R8 — AlgoLaser DKM1 |
| Firmware laser | GrblHAL 1.1f (AL_DKM1_G1_119.bin) |
| Controller periferiche | Arduino Uno R4 WiFi |
| Connessione LightBurn → Laser | USB (COM5 su PC Windows 192.168.1.22) |
| Connessione Arduino → Laser | WiFi AP laser → Telnet porta 23 |

---

## WiFi Laser

- **SSID AP**: `ALDKM1_AP_9F60` (ultimi 4 hex del MAC: `24:EC:4A:0D:9F:60`)
- **Password AP**: `12345678`
- **IP laser (AP)**: `192.168.5.1`
- **Telnet port**: `23`

> Il SSID AP cambia con la scheda (dipende dal MAC).
> Se si cambia scheda: leggere `$76` via seriale per conoscere il nuovo SSID.

---

## Problema noto: WiFi mode si resetta ad ogni power cycle

Il firmware AlgoLaser resetta `$73=1` (Station mode) ad ogni riavvio.
L'AP non è attivo in modalità Station → Arduino non si connette.

**Soluzione**: dopo ogni accensione della laser, dalla console LightBurn inviare:
```
$73=3
$WRS
```
Questo attiva la modalità AP+Station. L'AP rimane attivo per tutta la sessione.

> NON mettere questi comandi nel Start GCode per-job: `$WRS` riavvia il WiFi
> ad ogni job, disconnettendo l'Arduino continuamente.
> Usare solo nella console LightBurn una volta per sessione.

---

## Impostazioni GrblHAL utili

| Setting | Valore | Descrizione |
|---------|--------|-------------|
| `$73` | `1`=Station / `2`=AP / `3`=AP+Station | WiFi mode |
| `$74` | SSID rete home | WiFi STA SSID |
| `$75` | Password rete home | WiFi STA Password |
| `$76` | SSID AP laser | WiFi AP SSID (readonly, dipende dal MAC) |
| `$77` | `12345678` | WiFi AP Password |
| `$WRS` | — | Riavvia WiFi con nuove impostazioni |

---

## Procedura di avvio sessione laser

1. Accendi la laser
2. Apri LightBurn e connetti via USB (COM5)
3. Dalla console LightBurn invia:
   ```
   $73=3
   $WRS
   ```
4. Aspetta ~10 secondi che il WiFi si riavvii
5. L'Arduino si riconnette automaticamente all'AP
6. Sistema pronto — avvia i job normalmente

---

## Logica Arduino (sketch v6)

- Polling ogni 500ms con `?` via Telnet
- `<Run|...>` → LAVORO: elettrovalvola ON, aspirazione ON, semaforo ON, LED OFF
- `<Idle|...>` → PAUSA: elettrovalvola OFF, semaforo OFF, aspirazione OFF dopo 3 min, LED ON
- Se WiFi perso → reconnect automatico

## PIN Arduino

| PIN | Periferica |
|-----|------------|
| 11 | Elettrovalvola aria compressa |
| 12 | Semaforo |
| 8 | Aspirazione fumi |
| 7 | LED area lavoro |

---

## Comandi seriali Arduino (115200 baud)

| Comando | Funzione |
|---------|----------|
| `MODE=MANUAL` | Controllo manuale |
| `MODE=AUTO` | Torna al polling automatico |
| `LED=BOOST` | LED ON forzato 5 min |
| `LED=BOOST_OFF` | Annulla boost |
| `EV=ON/OFF` | Elettrovalvola (solo MANUAL) |
| `ASP=ON/OFF` | Aspirazione (solo MANUAL) |
| `RESET` | Riavvio Arduino |

---

## Accesso remoto PC Windows (192.168.1.22)

- **SSH**: `balzoni@192.168.1.22` — password: `525`
- **Porta seriale laser**: COM5
- **esptool**: `C:\Users\balzoni\AppData\Local\Programs\Python\Python313\Scripts\esptool.exe`

## Flash firmware originale AlgoLaser (ripristino emergenza)

```bash
# File: C:\Users\balzoni\Desktop\AL_DKM1_G1_119.bin
# Mettere board in bootloader: tieni BOOT, premi RESET, rilascia RESET, rilascia BOOT

esptool --chip esp32s3 --port COM4 --baud 921600 write_flash 0x0 AL_DKM1_G1_119.bin
```

> Il file AL_DKM1_G1_119.bin è solo l'applicazione (non include bootloader).
> Flashare a 0x0 sovrascrive il bootloader → board non si avvia.
> Usare il bootloader FluidNC disponibile nel repo come recovery.

---

## Storia del progetto

- Tentato FluidNC su ESP32-S3: motori funzionanti, homing mai risolto (limit switch sempre attivi)
- Tornati al firmware originale AlgoLaser GrblHAL
- Sviluppato sistema Arduino per controllo periferiche via WiFi AP laser
- Rilevamento stato tramite polling `?` (Run/Idle) invece di M8/M9
