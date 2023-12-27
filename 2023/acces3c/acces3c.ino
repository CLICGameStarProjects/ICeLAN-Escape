#include <ESP32Servo.h>
#include "puzzlelib.h"
#define PUZZLE

//Il s'agit de l'accès "Speedrun". On observe que les LEDs rouge et verte clignotent 
//en alternance. Si on appuie avec succès sur le bouton blanc lorsque la LED verte 
//est active trois fois d’affilée, la porte s’ouvre.
Servo myServo;


int ledRouge = 14;
int ledVerte = 12;
int button = 34;
bool locked = true;

void set_state(bool lock) {
  if (locked == lock) return;

  if (locked) {
    Serial.println("unlock");

    puzzle_request_sound(2); // UNLOCK SOUND
    digitalWrite(ledVerte, HIGH);

  }

  locked = lock;
}

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  
  pinMode(ledRouge, OUTPUT); //led rouge
  pinMode(ledVerte, OUTPUT); //led verte

  ledcSetup(10, 2000, 10);

  pinMode(button, INPUT); //Bouton

  if (puzzle_init(set_state, "ACC3") != PUZZLE_OK) {
    Serial.println("Error initializing puzzle");
  }
}

int counter = 0; //quand bouton appuyé et que la vert s'allume +1, quand rouge s'allume et quand on appuie alors que le counter est différent de 3, 0
void IRAM_ATTR ledGreen() {
  counter++;
  Serial.println("+1");
  puzzle_request_sound(0);
  detachInterrupt(button);
}

void IRAM_ATTR ledRed() {
  if (counter != 0) {
    puzzle_request_sound(1);
  }
  counter = 0;
  detachInterrupt(button);
}

int timings[2] = {1000, 500};

void loop() {
  // put your main code here, to run repeatedly:
  //boutonAppuye = digitalRead(button); //march que si on a 7??? //si bouton actif ce sera HIGH, sinon LOW

  //utiliser random(2) qui prend un nombre entre 0 et 1, si on a 0 la led rouge s'allume, sinon la verte
  if (locked) {
    int controlLumiere = random(2);
    int timing = timings[random(2)];

    if(controlLumiere == 1) {
      digitalWrite(ledVerte, HIGH);
      attachInterrupt(button, ledGreen, RISING);
      delay(timing);
      detachInterrupt(button);
      digitalWrite(ledVerte, LOW);
      delay(200);
    } else {
      digitalWrite(ledRouge, HIGH);
      attachInterrupt(button, ledRed, RISING);
      delay(timing);
      detachInterrupt(button);
      digitalWrite(ledRouge,LOW);

      delay(200);
    }
    
    if (counter == 5){ //succès
      puzzle_update_lock_status(false);
    }
  }  
}
