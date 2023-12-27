#include <ESP32Servo.h>
#include "puzzlelib.h"
#define PUZZLE

Servo myServo;
int seq[8] = {0, 1, 1, 1, 0, 1, 1, 0};
int timeDelay = 250;
int button = 4;

int switches[8] = {23, 22, 21, 19, 18, 5, 17, 16};

bool locked = true;

void set_state(bool lock) {
  if (locked) {
    puzzle_request_sound(2);
    Serial.println("unlock");
  }

  locked = lock;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Séquence : ");
  for (int i=0; i<8; i++){
    Serial.print(seq[i]);
  }
  //set up switches and LEDs
  for (int i = 0; i < 8; i++) {
    pinMode(switches[i], INPUT);
  }

  if (puzzle_init(set_state, "SEQS") != PUZZLE_OK) {
    Serial.println("Error initializing puzzle");
  }
}


void loop() {
  bool okay = true;
  for (int i = 0; i < 8; i++) {
    Serial.print(digitalRead(switches[i]));
    if ((digitalRead(switches[i]) == HIGH ? 1 : 0) != seq[i]) {
      okay = false;
    }
  }

  Serial.println();

  if (locked && okay) {
    puzzle_update_lock_status(false);
  }
  delay(100);
}
