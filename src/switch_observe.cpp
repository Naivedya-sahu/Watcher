/*
 * Raw Mechanical Switch Observer for ESP32-S3
 * Hardware: 3 switches with external 10kΩ pull-ups + 100nF capacitors to GND
 * Purpose: Observe RC filter behavior and threshold noise
 */

#include <Arduino.h>

// GPIO pins with external pull-ups + RC filters
#define SW1_PIN  35
#define SW2_PIN  36
#define SW3_PIN  37

// Built-in LED for visual feedback (optional - may not exist on all boards)
#define LED_PIN  38

// Track last known states
uint8_t lastState1 = HIGH;
uint8_t lastState2 = HIGH;
uint8_t lastState3 = HIGH;

// Heartbeat counter
uint32_t loopCount = 0;
uint32_t lastHeartbeat = 0;

void setup() {
    // Initialize LED for visual feedback
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED on during setup

    // Initialize hardware UART Serial (not USB CDC)
    Serial.begin(115200);
    delay(100);  // Short delay for serial to initialize

    // Configure as plain inputs (no internal pull-ups)
    pinMode(SW1_PIN, INPUT);
    pinMode(SW2_PIN, INPUT);
    pinMode(SW3_PIN, INPUT);

    // Read initial states
    lastState1 = digitalRead(SW1_PIN);
    lastState2 = digitalRead(SW2_PIN);
    lastState3 = digitalRead(SW3_PIN);

    // Banner output
    Serial.println("\n\n===========================================");
    Serial.println("   ESP32-S3 Raw Switch Observer (UART)");
    Serial.println("===========================================");
    Serial.println("Hardware: External 10kΩ pull-ups + 100nF to GND");
    Serial.println("GPIOs: 35, 36, 37");
    Serial.println("Format: [timestamp_us] SW# old->new");
    Serial.println("-------------------------------------------");
    Serial.printf("Initial States: SW1=%d SW2=%d SW3=%d\n", lastState1, lastState2, lastState3);
    Serial.println("===========================================\n");

    digitalWrite(LED_PIN, LOW);  // LED off - ready
}

void loop() {
    loopCount++;

    // Heartbeat every 5 seconds to confirm code is running
    uint32_t now = millis();
    if (now - lastHeartbeat >= 5000) {
        Serial.printf("[HEARTBEAT] %lu loops, uptime: %lu ms\n", loopCount, now);
        loopCount = 0;
        lastHeartbeat = now;
    }

    // Read current states
    uint8_t state1 = digitalRead(SW1_PIN);
    uint8_t state2 = digitalRead(SW2_PIN);
    uint8_t state3 = digitalRead(SW3_PIN);

    // Log transitions only (edge detection)
    if (state1 != lastState1) {
        Serial.printf("[%lu] SW1 %d->%d\n", micros(), lastState1, state1);
        lastState1 = state1;
    }

    if (state2 != lastState2) {
        Serial.printf("[%lu] SW2 %d->%d\n", micros(), lastState2, state2);
        lastState2 = state2;
    }

    if (state3 != lastState3) {
        Serial.printf("[%lu] SW3 %d->%d\n", micros(), lastState3, state3);
        lastState3 = state3;
    }

    // No delays - maximize sampling rate to catch noise
}
