#include <WiFiS3.h>

// ==================== PIN ==========================
#define ELETTROVALVOLA 11
#define SEMAFORO       12
#define ASPIRAZIONE    8
#define LED            7

// ==================== CONFIG =======================
#define ASPIRAZIONE_DELAY 180000UL  // 3 min dopo PAUSA → spegni aspirazione/LED

const char* WIFI_SSID    = "Balzoni Vittore Internal";
const char* WIFI_PASS    = "[BV142rd]";
const char* FLUIDNC_IP   = "192.168.1.23";
const int   FLUIDNC_PORT = 80;

// ==================== STATO ========================
enum MachineMode { PAUSA_MODE, LAVORO_MODE };
MachineMode currentMode = PAUSA_MODE;

bool manualMode = false;
unsigned long pausaStartTime = 0;
bool aspirazioneSpenta = false;
bool ledSpento = false;
unsigned long ledBoostEnd = 0;

// Stato coolant precedente (per rilevare cambiamenti)
bool lastCoolantOn = false;

// ==================== PROTOTIPI ====================
void handleUSB();
void handleFluidNC();
void handleTimers();
void setMachineMode(MachineMode mode);
void printStatus();
void sendStatusToPC();
void connectWiFi();

// Buffer per ricezione comandi USB non-bloccante
static char usbCmdBuf[64];
static int  usbCmdLen = 0;

// ==================== SETUP ========================
void setup() {
  Serial.begin(115200);

  pinMode(ELETTROVALVOLA, OUTPUT);
  pinMode(SEMAFORO,       OUTPUT);
  pinMode(ASPIRAZIONE,    OUTPUT);
  pinMode(LED,            OUTPUT);

  digitalWrite(ELETTROVALVOLA, LOW);
  digitalWrite(SEMAFORO,       LOW);
  digitalWrite(ASPIRAZIONE,    LOW);
  digitalWrite(LED,            HIGH);

  Serial.println("=== LASER CONTROLLER v4 (FluidNC) ===");

  connectWiFi();
  printStatus();
}

// ==================== LOOP =========================
void loop() {
  handleUSB();
  if (!manualMode) handleFluidNC();
  if (!manualMode) handleTimers();
  sendStatusToPC();
}

// ==================== WIFI =========================
void connectWiFi() {
  Serial.print("Connessione WiFi a ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" OK → ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" FALLITO — riprovo in loop()");
  }
}

// ==================== POLLING FLUIDNC ==============
// Polling HTTP ogni 100ms.
// FluidNC risponde con: <Idle|MPos:...|A:F> dove A:F = flood coolant ON (M8).
// Senza campo A: = coolant spento (M9/pausa).
void handleFluidNC() {
  static unsigned long lastPoll = 0;
  if (millis() - lastPoll < 100) return;
  lastPoll = millis();

  // Riconnetti WiFi se caduto
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi perso — riconnessione...");
    connectWiFi();
    return;
  }

  WiFiClient client;
  if (!client.connect(FLUIDNC_IP, FLUIDNC_PORT)) return;

  // Richiesta status FluidNC
  client.print(F("GET /command?commandText=? HTTP/1.1\r\n"
                 "Host: 192.168.1.23\r\n"
                 "Connection: close\r\n\r\n"));

  // Leggi risposta con timeout 300ms
  unsigned long t = millis();
  String response = "";
  while (millis() - t < 300) {
    while (client.available()) {
      response += (char)client.read();
    }
    if (response.indexOf('\n') > 0 && !client.available()) break;
  }
  client.stop();

  if (response.length() == 0) return;

  // Rileva coolant:
  // FluidNC status: <Run|MPos:...|A:F>  → flood ON = M8 attivo
  // Nessun campo A: → coolant spento = M9/pausa
  bool coolantOn = (response.indexOf("|A:F") >= 0 || response.indexOf("|A:FM") >= 0);

  if (coolantOn != lastCoolantOn) {
    lastCoolantOn = coolantOn;
    if (coolantOn) {
      Serial.println(">>> M8 RILEVATO (flood ON) <<<");
      setMachineMode(LAVORO_MODE);
    } else {
      Serial.println(">>> M9 RILEVATO (flood OFF) <<<");
      setMachineMode(PAUSA_MODE);
    }
  }
}

// ==================== USB MANUALE ===================
void handleUSB() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      if (usbCmdLen == 0) continue;
      usbCmdBuf[usbCmdLen] = '\0';
      String cmd = String(usbCmdBuf);
      cmd.trim();
      cmd.toUpperCase();
      usbCmdLen = 0;

      if      (cmd == "MODE=MANUAL") { manualMode = true;  Serial.println(">>> MANUALE <<<"); }
      else if (cmd == "MODE=AUTO")   { manualMode = false; Serial.println(">>> AUTO <<<"); }
      else if (cmd == "RESET")       { NVIC_SystemReset(); }

      if (cmd == "LED=BOOST") {
        ledBoostEnd = millis() + 300000UL;
        digitalWrite(LED, HIGH);
        Serial.println(">>> LED BOOST 5 MIN <<<");
      }
      if (cmd == "LED=BOOST_OFF") {
        ledBoostEnd = 0;
        ledSpento   = true;
        digitalWrite(LED, LOW);
        Serial.println(">>> LED BOOST ANNULLATO <<<");
      }

      if (manualMode) {
        if      (cmd == "LED=ON")  { digitalWrite(LED, HIGH); }
        else if (cmd == "LED=OFF") { ledBoostEnd = 0; digitalWrite(LED, LOW); }
        else if (cmd == "EV=ON")   digitalWrite(ELETTROVALVOLA, HIGH);
        else if (cmd == "EV=OFF")  digitalWrite(ELETTROVALVOLA, LOW);
        else if (cmd == "ASP=ON")  digitalWrite(ASPIRAZIONE,    HIGH);
        else if (cmd == "ASP=OFF") digitalWrite(ASPIRAZIONE,    LOW);
      }
    } else {
      if (usbCmdLen < 63) usbCmdBuf[usbCmdLen++] = c;
    }
  }
}

// ==================== TIMER PAUSA ==================
void handleTimers() {
  unsigned long now = millis();

  if (ledBoostEnd > 0 && now >= ledBoostEnd) {
    ledBoostEnd = 0;
    ledSpento   = true;
    digitalWrite(LED, LOW);
    Serial.println(">>> LED BOOST SCADUTO <<<");
  }

  if (currentMode == PAUSA_MODE) {
    if (!aspirazioneSpenta && now - pausaStartTime >= ASPIRAZIONE_DELAY) {
      digitalWrite(ASPIRAZIONE, LOW);
      aspirazioneSpenta = true;
      Serial.println(">>> ASPIRAZIONE SPENTA (3 min) <<<");
    }
    if (!ledSpento && now - pausaStartTime >= ASPIRAZIONE_DELAY) {
      ledSpento = true;
      if (ledBoostEnd == 0) {
        digitalWrite(LED, LOW);
        Serial.println(">>> LED SPENTO (3 min) <<<");
      }
    }
  }
}

// ==================== CAMBIO STATO =================
void setMachineMode(MachineMode mode) {
  if (currentMode == mode) return;
  currentMode = mode;

  if (mode == LAVORO_MODE) {
    digitalWrite(ELETTROVALVOLA, HIGH);
    digitalWrite(SEMAFORO,       HIGH);
    digitalWrite(ASPIRAZIONE,    HIGH);
    if (ledBoostEnd == 0 || millis() >= ledBoostEnd) digitalWrite(LED, LOW);
    aspirazioneSpenta = false;
    ledSpento         = false;
    Serial.println(">>> STATO MACCHINA: LAVORO (M8) <<<");
  } else {
    digitalWrite(ELETTROVALVOLA, LOW);
    digitalWrite(SEMAFORO,       LOW);
    if (ledBoostEnd == 0 || millis() >= ledBoostEnd) digitalWrite(LED, HIGH);
    // ASPIRAZIONE rimane ON → spenta da handleTimers dopo 3 min
    pausaStartTime    = millis();
    aspirazioneSpenta = false;
    ledSpento         = false;
    Serial.println(">>> STATO MACCHINA: PAUSA (M9) <<<");
  }

  printStatus();
}

// ==================== DEBUG ========================
void printStatus() {
  Serial.print("STATO: ");
  Serial.print(currentMode == LAVORO_MODE ? "LAVORO" : "PAUSA");
  Serial.print(" | MODE="); Serial.print(manualMode ? "MANUAL" : "AUTO");
  Serial.print(" | EV=");   Serial.print(digitalRead(ELETTROVALVOLA));
  Serial.print(" | ASP=");  Serial.print(digitalRead(ASPIRAZIONE));
  Serial.print(" | LED=");  Serial.println(digitalRead(LED));
}

void sendStatusToPC() {
  static unsigned long last = 0;
  if (millis() - last < 5000) return;
  last = millis();
  printStatus();
}
