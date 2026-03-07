#pragma once
// clang-format off

/*
    algolaser.h
    Macchina laser Algolaser DIY Mini 10W (ESP32-S3)

    Questa configurazione mappa i pin dei motori, finecorsa e laser
    come riportato dal comando $PINS del firmware originale:
      [PIN:5,Power control]
      [PIN:46,Reset]
      [PIN:18,X limit min]
      [PIN:8,Y limit min]
      [PIN:3,Z limit min]
      [PIN:6,X step]
      [PIN:16,Y step]
      [PIN:15,Stepper enable]
      [PIN:48,Spindle on]
      [PIN:7,X dir]
      [PIN:17,Y dir]
      [PIN:47,Spindle PWM]

    Note:
      - Gli assi Z non sono configurati per uscita motori (solo finecorsa).
      - Il pin 5 (Power control) e il pin 46 (Reset) non sono usati da Grbl.
*/

#define MACHINE_NAME "Algolaser DIY mini 10W"

#define N_AXIS 3

// Asse X
#define X_STEP_PIN          GPIO_NUM_6
#define X_DIRECTION_PIN     GPIO_NUM_7

// Asse Y
#define Y_STEP_PIN          GPIO_NUM_16
#define Y_DIRECTION_PIN     GPIO_NUM_17

// Enable (shared per tutti i motori)
#define STEPPERS_DISABLE_PIN GPIO_NUM_15

// Finecorsa
#define X_LIMIT_PIN         GPIO_NUM_18
#define Y_LIMIT_PIN         GPIO_NUM_8
#define Z_LIMIT_PIN         GPIO_NUM_3

// Laser output
#define SPINDLE_TYPE        SpindleType::LASER
#define LASER_OUTPUT_PIN    GPIO_NUM_47  // PWM output (laser power)
#define LASER_ENABLE_PIN    GPIO_NUM_48  // On/Off

// Mantieni il codice custom per pulsante + LED
#ifdef USE_RMT_STEPS
#undef USE_RMT_STEPS
#endif
#define CUSTOM_CODE_FILENAME "Custom/algolaser_custom.h"

// Pin pulsante e LED (usati dal custom code)
#define ALGOLASER_BTN_PIN   GPIO_NUM_5   // pulsante SW, attivo LOW
#define ALGOLASER_LED_PIN   GPIO_NUM_1   // LED verde, attivo LOW (anodo comune)

// Default settings
#define DEFAULT_INVERT_LIMIT_PINS 0
#define DEFAULT_INVERT_PROBE_PIN 0

#define DEFAULT_LASER_MODE 1
