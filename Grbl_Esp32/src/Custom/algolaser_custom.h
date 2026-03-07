// algolaser_custom.h
// GPIO 5 = Power control: deve stare HIGH per alimentare i driver stepper / laser PSU

void machine_init() {
    // Tieni GPIO5 HIGH per alimentare i driver stepper e il laser PSU
    pinMode(ALGOLASER_BTN_PIN, OUTPUT);
    digitalWrite(ALGOLASER_BTN_PIN, HIGH);
}
