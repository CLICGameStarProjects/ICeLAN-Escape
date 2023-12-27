int SIGNAL_PIN = 15; // ESP32 pin GIOP36 (ADC0) connected to sensor's signal pin

int value = 0; // variable to store the sensor value

void setup() {
  Serial.print("hi");
  pinMode (SIGNAL_PIN, INPUT);
  Serial.print("Hello");
  Serial.begin(115200);
}

void loop() {
  delay(10);                      // wait 10 milliseconds
  value = analogRead(SIGNAL_PIN); // read the analog value from sensor

  Serial.print("The water sensor value: ");
  Serial.println(value);

  delay(1000);
}
