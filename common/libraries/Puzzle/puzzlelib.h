#pragma once
#include "Arduino.h"
#include <esp_now.h>
#include <WiFi.h>

typedef enum {
    SET_STATE,
    LOGIN,
    PLAY_SOUND,
} puzzle_code;

typedef enum {
    PUZZLE_OK,
    INIT_FAIL,
    PEER_INIT_FAIL,
} error_code;

typedef struct {
    uint8_t id;
    uint32_t code;
    uint32_t data;
} puzzle_message;

typedef void (*puzzle_lock_set)(bool state);

int controller_init(esp_now_recv_cb_t recv_cb);
int puzzle_init(puzzle_lock_set setting_cb, const char* name);
void puzzle_request_sound(int sound);
void puzzle_update_lock_status(bool locked);
