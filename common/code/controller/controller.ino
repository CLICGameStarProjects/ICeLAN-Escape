#include <ESP32Servo.h>
#include "puzzlelib.h"

uint8_t macs[6*16] = {};
char names[5*16] = {};
int next_id = 0;

int com_state = 0;
typedef enum {
  COM_SET_STATE,
  COM_LOGIN,
  COM_PLAY_SOUND,
  COM_RESET,
} com_message_type;

typedef struct {
  uint8_t id;
  uint32_t type;
  uint32_t data;
} com_message;

void init_peripheral(const uint8_t* mac, const puzzle_message* message) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    //Serial.println("Failed to add peer");
  }

  com_message computer = { .id = next_id, .type = COM_LOGIN, .data = message->data};
  Serial.write((char*) &computer, sizeof(computer));

  digitalWrite(2, !digitalRead(2));

  strncpy(names+5*next_id, (char*) &message->data, 5);
  memcpy(macs+6*next_id, mac, 6);
  puzzle_message sendable = {.id = -1, .code = LOGIN, .data = next_id++};
  esp_now_send(mac, (uint8_t *) &sendable, sizeof(sendable));
}

void send_com_state_message(int id, uint32_t data) {
  com_message computer = { .id = id, .type = COM_LOGIN, .data = data};
  Serial.write((char*) &computer, sizeof(computer));
}

void send_com_sound_message(int id, uint32_t data) {
  com_message computer = { .id = id, .type = COM_PLAY_SOUND, .data = data};
  Serial.write((char*) &computer, sizeof(computer));
}

void send_state_message(int id, const puzzle_message* message) {
  esp_now_send(macs+6*id, (uint8_t*) message, sizeof(puzzle_message));
}

void recv_message(const uint8_t* mac, const uint8_t *data, int data_len) {
  if (data_len != sizeof(puzzle_message)) return;
  
  const puzzle_message* message = (puzzle_message*) data;
  switch (message->code) {
      case LOGIN:
          init_peripheral(mac, message);
          break;
      case SET_STATE:
          send_com_state_message(message->id, message->data);
          break;
      case PLAY_SOUND:
          send_com_sound_message(message->id, message->data);
          break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  controller_init(recv_message);
}

void loop() {
  delay(10);
  com_message message = { 0, COM_LOGIN, 0};

  puzzle_message sendable = {.id = 0, .code = SET_STATE, .data = 0};
  delay(1000);
  if (com_state) {
    if (Serial.available()) {
      Serial.readBytes((char*) &message, sizeof(com_message));
      puzzle_message sendable = {.id = message.id, .code = LOGIN, .data = message.data};
      switch (message.type) {
        case COM_SET_STATE:
          sendable.code = SET_STATE;
          send_state_message(message.id, &sendable);
          break;
        case COM_RESET:
          next_id = 0;
          break;
      }
    }
  } else {
    if (Serial.available()) {
      Serial.readBytes((char*) &message, sizeof(com_message));
      com_state = 1;
    }
  }
}