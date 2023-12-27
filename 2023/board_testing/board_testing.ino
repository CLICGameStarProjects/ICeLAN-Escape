#include <esp_now.h>
#include <WiFi.h>

#define SIGNAL_ERROR -1
#define SIGNAL_RESET 0
#define SIGNAL_START 1
#define SIGNAL_WIN 2
#define SIGNAL_FAIL 3

// REPLACE WITH THE MAC Address of the master board
uint8_t broadcastAddress[] = {0xE8, 0x68, 0xE7, 0x2E, 0x44, 0x94};

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
  int id;  
  int state;
  long timestamp;
} struct_message;

struct_message inData;
struct_message stateData;

esp_now_peer_info_t peerInfo;

// Testing purpose
int previousState;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\tLast Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(macStr);
  
  memcpy(&inData, incomingData, sizeof(inData));
  Serial.print("\tBytes received: ");
  Serial.print(len);
  Serial.print(", Id: ");
  Serial.print(inData.id);
  Serial.print(", Value: ");
  Serial.println(inData.state);

  inData.timestamp = millis();

  stateData.state = inData.state;
  previousState = stateData.state;
  switch (stateData.state) {
  case SIGNAL_ERROR:
    Serial.println("STATE: ERROR");
    break;
  case SIGNAL_RESET:
    Serial.println("STATE: RESET");
    setup();
    break;
  case SIGNAL_START:
    Serial.println("STATE: START");
  }
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

  // Set ID
  stateData.id = 0;

  // Testing purpose
  previousState = SIGNAL_RESET;
}

void loop() {
  // Game is on
  if(stateData.state == SIGNAL_START || stateData.state == SIGNAL_FAIL) {
    
  }

  // Send result
  if(previousState != stateData.state) {
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &stateData, sizeof(stateData));
     
    if (result == ESP_OK) {
      Serial.println("> Sent with success");
    }
    else {
      Serial.println("> Error sending the data");
    }

    previousState = stateData.state;
  }
  delay(2000);
}
