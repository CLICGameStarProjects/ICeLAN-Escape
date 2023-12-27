#include "puzzlelib.h"
#define PUZZLE

const int MAX_LEVEL = 10;
int sequence[MAX_LEVEL];
int your_sequence[MAX_LEVEL];
int level = 0;

int velocity = 700;
int boutonStart = 10;
int boutons[4] = {4,21, 33, 27};
int leds[4] = {15, 19, 32, 14};
note_t notes[4] = {NOTE_A, NOTE_C, NOTE_E, NOTE_G};
int buzzer = 32;

bool locked = true;

void set_all(int state) {
  for (int i = 0; i < 4; ++i) {
    digitalWrite(leds[i], state);
  }
}

void set_state(bool lock) {
  if (locked) {
    Serial.println("unlock");
    delay(1000);
  } else {
  }

  locked = lock;
}

void setup() {
  Serial.begin(9600);
  Serial.println();
  ledcSetup(0, 2048, 10);
  ledcAttachPin(buzzer, 0);
  //set up boutons & leds
  for (int i = 0; i < 4; ++i) {
    pinMode(boutons[i], INPUT);
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], HIGH);
  }
  set_all(LOW);


  if (puzzle_init(set_state, "SIMN") != PUZZLE_OK) {
    Serial.println("Error initializing puzzle");
  }
}

void loop() {
  if (!locked) {
    set_all(HIGH);
    return;
  }

  if (level == 0) {
    int buttonPressed = -1;
    while (buttonPressed == -1) {
      for (int j = 0; j < 4; j++) {
      	if (digitalRead(boutons[j]) == HIGH)
      	  buttonPressed = j;
      }
    }
    level = 1;
    generate_sequence();
    delay(500);
  } else {
    Serial.print("Level ");
    Serial.println(level);
    //montre la séquence  
    show_sequence();
    //enregistre ta séquence
    get_sequence();
  }
}

void show_sequence(){
  //éteint toutes les leds
  set_all(LOW);

  //montre la séquence
  for (int i = 0; i < level; i++){
    digitalWrite(leds[sequence[i]], HIGH);
    ledcWriteNote(0, NOTE_B, 5);
    delay(velocity);
    digitalWrite(leds[sequence[i]], LOW);
    delay(velocity/5);
  }
}

void get_sequence() {
  for (int i = 0; i < level; i++){
    int buttonPressed = -1;
    Serial.println("Waiting for enter");
    while (buttonPressed == -1) {
      for (int j = 0; j < 4; j++) {
      	if (digitalRead(boutons[j]) == HIGH) {
      	  buttonPressed = j;
        }
      }
    }
    Serial.print("Entered ");
    Serial.println(buttonPressed);
    Serial.print("Expected ");
    Serial.println(sequence[i]);
    
    digitalWrite(leds[buttonPressed], HIGH); // TODO beeps
    if (buttonPressed != sequence[i]) {
      wrong_sequence();
      return;
    }
    delay(10);
    while (digitalRead(boutons[buttonPressed]) == HIGH) {
      while (digitalRead(boutons[buttonPressed]) == HIGH) delay(1);
      delay(5);
    }
    delay(10);
    digitalWrite(leds[buttonPressed], LOW);
  }
  right_sequence();
}

void generate_sequence(){
  //générer une sequence aléatoire
  randomSeed(millis()); 
  for (int i = 0; i < MAX_LEVEL; i++){
    sequence[i] = random(0,4);
    while (i > 2 && sequence[i-2] == sequence[i-1] && sequence[i-1] == sequence[i]) {
      sequence[i] = random(0,4);
    }
    Serial.print(sequence[i]);
    Serial.print(", ");
  }
  Serial.println();
}

void wrong_sequence(){
  /*for (int i = 0; i < 3; i++){
    digitalWrite(2, HIGH);
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
    delay(250);
    digitalWrite(2, LOW);
    digitalWrite(3, LOW);
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    delay(250);
  }*/

  //Son défaite  
  Serial.println("dommage"); // TODO beeps lol
  puzzle_request_sound(1);
  //Retour départ  
  for (int i = 0; i < 5; i++){
      set_all(HIGH);
      delay(100);

      set_all(LOW);
      delay(100);
    }
   
  level = 0;
  velocity = 1000;
  
}

void right_sequence(){
  if (level < MAX_LEVEL){
    //led clignotes
    set_all(LOW);
    delay(500);

    puzzle_request_sound(0);
    set_all(HIGH);
    delay(500);

    set_all(LOW);
    delay(500);
    level++;
  }   
  
  //Condition de voctoire (j'ai ajouté)
  if (level == MAX_LEVEL){
    //led clignote 3 fois;
    for (int i = 0; i < 3; i++){
      set_all(HIGH);
      delay(250);

      set_all(LOW);
      delay(250);
    }
    
    //Son de victoire
    //Déclanche le moteur
    puzzle_update_lock_status(false);
    puzzle_request_sound(2);
  }
  //Augmente la difficulté
  if (velocity >= 100) {
    velocity -= 100;   
  }
  if (velocity < 100) {
    velocity = 100;
  }
}
