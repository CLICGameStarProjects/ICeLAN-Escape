#include <ESP32Servo.h>
#include "puzzlelib.h"
#define PUZZLE

#define RLED_PIN    13
#define GLED_PIN    12
#define SIGNAL_PIN  A5
#define THRESHOLD   3000
int bouton = 5;

bool locked = true;
void set_state(bool lock) {
  if (locked == lock) return;

  if (locked) {
    Serial.println("unlock");
  } else {
  }

  locked = lock;
}

void setup() {
  Serial.begin(9600);
  pinMode(bouton,      INPUT);
  pinMode(RLED_PIN,   OUTPUT);
  pinMode(GLED_PIN,   OUTPUT);
  digitalWrite(RLED_PIN,   HIGH);
  digitalWrite(GLED_PIN,   LOW);
  if (puzzle_init(set_state, "WATR") != PUZZLE_OK) {
    Serial.println("Error initializing puzzle");
  } else {
    Serial.println("Init okay");
  }
}

void loop() {
  delay(100);
  if (!locked) {
    digitalWrite(RLED_PIN, LOW);
    digitalWrite(GLED_PIN, HIGH);
    return;
  }
  int value = analogRead(SIGNAL_PIN);
  if (value > THRESHOLD) {
    digitalWrite(GLED_PIN, HIGH);
    digitalWrite(RLED_PIN, LOW);
    for (int i = 0; i < 1000; ++i) {
      value = analogRead(SIGNAL_PIN);
      if (value <= THRESHOLD) {
        digitalWrite(RLED_PIN, HIGH);
        digitalWrite(GLED_PIN, LOW);
        return;
      }
      delay(1);
    }

    while (digitalRead(bouton) == LOW) delay(1);
    puzzle_update_lock_status(false);
  } else {
    digitalWrite(RLED_PIN, HIGH);
    digitalWrite(GLED_PIN, LOW);
  }
  
}
