#include "Arduino.h"
#include <esp_now.h>
#include <WiFi.h>
#include "puzzlelib.h"

int state = 0;

int controller_init(esp_now_recv_cb_t recv_cb) {
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        state = -1;
        return -INIT_FAIL;
    }

    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info
    esp_now_register_recv_cb(recv_cb);

    state = 1;
    return PUZZLE_OK;
}
uint8_t broadcastAddress[] = {0xE8, 0x68, 0xE7, 0x2B, 0x9C, 0x64};
puzzle_lock_set set_cb;

int id = -1;

void puzzle_recv_message(const uint8_t* mac, const uint8_t *data, int data_len) {
    Serial.println("Received message");
    if (data_len != sizeof(puzzle_message)) return;
    
    const puzzle_message* message = (puzzle_message*) data;
    switch (message->code) {
        case LOGIN:
            id = message->data;
            break;
        case SET_STATE:
            set_cb(message->data);
            break;
    }
}

int puzzle_init(puzzle_lock_set setting_cb, const char* name) {
    set_cb = setting_cb;
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        state = -1;
        return -INIT_FAIL;
    }

    // Register peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        state = -1;
        return -PEER_INIT_FAIL;
    }
    
    esp_now_register_recv_cb(puzzle_recv_message);
    
    int data = 0;
    strncpy((char*)(&data), name, 4);
    puzzle_message message = {.id = id, .code = LOGIN, .data = data};
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &message, sizeof(message));
    state = 1;
    return PUZZLE_OK;
}

void puzzle_request_sound(int sound) {
    if (state == 1) {
        puzzle_message message = {.id = id, .code = PLAY_SOUND, .data = sound};
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &message, sizeof(message));
        if (result == ESP_OK) {
            Serial.println("Requested sound");
        } else {
            Serial.println("Error requesting sound");
        }
    }
}

void puzzle_update_lock_status(bool locked) {
    if (state == 1) {
        puzzle_message message = {.id = id, .code = SET_STATE, .data = locked ? 1 : 0};
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &message, sizeof(message));
        if (result == ESP_OK) {
            Serial.println("Updated lock status");
        } else {
            Serial.println("Error updating lock status");
        }
    }
    set_cb(locked);
}
