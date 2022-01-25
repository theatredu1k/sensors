/* https://dronebotworkshop.com/esp32-servo/

  FIIEC continuous servo FS90R */

//library for servo
#include <Servo.h>


// library for Heltec
#include "DHTesp.h"
#include "heltec.h"
DHTesp dht;

//library for ESP NOW
#include <esp_now.h>
#include <WiFi.h>
//library to use I2C devices
#include <Wire.h>

// ************** VARIABLES **************
//  MAC Address of the receiver from Heltec
//uint8_t broadcastAddress[] = {0x94, 0xB9, 0x7E, 0xDA, 0xA2, 0x28};//ESP 32-1
//uint8_t broadcastAddress[] = {0x7C, 0x9E, 0xBD, 0x5B, 0x35, 0xE4};// sending to HELTEC 2 GREEN- //7C:9E:BD:5B:35:E4
uint8_t broadcastAddress[] = {0x08, 0x3A, 0xF2, 0x71, 0x2A, 0x28};//Heltec 1 BLACK

// Define variables to store DHT22 readings to be sent
float temperature;
float humidity;
// Define variables to store incoming readings
float incomingTemp;
float incomingHum;
#define DHTPIN 27    // Digital pin connected to the DHT sensor GPIO D5
#define DHTTYPE    DHT22     // DHT 22 (AM2302)


//DECLARATIONS SERVO
Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

// GPIO the servo is attached to
static const int servoPin = 13;

int servoTemp;//value for rotating the servo


// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
  float temp;
  float hum;
  float pres;
} struct_message;

// Create a struct_message called DHT22Readings to hold sensor readings
struct_message DHT22Readings;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;


// ************** CALL BACK FUNCTIONS **************

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success :)";
  }
  else {
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingTemp = incomingReadings.temp;
  incomingHum = incomingReadings.hum;
}



//************** SET UP AND LOOP **************
void setup()
{
  // Init Serial Monitor
  Serial.begin(115200);
  dht.setup(DHTPIN, DHTesp::DHTTYPE);
  myservo.attach(servoPin);  // attaches the servo on the servoPin to the servo object


  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, false /*Serial Enable*/);


  //// Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer Register the other ESP to broadcast to
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

}

void loop() {

  getReadings();
  // Set values to send
  DHT22Readings.temp = temperature;
  DHT22Readings.hum = humidity;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &DHT22Readings, sizeof(DHT22Readings));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  updateDisplay();

  if (incomingReadings.hum < 50) servoTemp = 90;//treshold 50% humidity
  else servoTemp = 95;

  Serial.print("Rotate : " );
  Serial.println(servoTemp);
  myservo.write(servoTemp);




  delay(10000);

}


// ************** READ AND DISPLAY FUNCTIONS **************
void getReadings() {
  temperature = dht.getTemperature();
  humidity = dht.getHumidity();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  }
}


void updateDisplay() {

  String temperatureDisplay = "Temperature: " + (String)incomingTemp +  "°C";
  String humidityDisplay = "Humidity: " + (String)incomingHum + "%";

  String temperaturehereDisplay = "Temp. here: " + (String)incomingTemp +  "°C";
  String humidityhereDisplay = "Hum. here: " + (String)humidity + "%";

  // Clear the OLED screen
  Heltec.display->clear();
  //Heltec.display->drawString(0, 0, "INCOMING READINGS");

  // Prepare to display temperature
  Heltec.display->drawString(0, 0, temperatureDisplay);
  // Prepare to display humidity
  Heltec.display->drawString(0, 10, humidityDisplay);
  // Prepare to display humidity here
  Heltec.display->drawString(0, 25, temperaturehereDisplay);
 // Prepare to display humidity here
  Heltec.display->drawString(0, 35, humidityhereDisplay);
  
  // Prepare to display state of sending success
  Heltec.display->drawString(0, 50, success);
  // Display the readings
  Heltec.display->display();


  // Display Readings in Serial Monitor
  Serial.println("INCOMING READINGS");
  Serial.print("Temperature: ");
  Serial.print(incomingReadings.temp);
  Serial.println(" ºC");
  Serial.print("Humidity: ");
  Serial.print(incomingReadings.hum);
  Serial.println(" %");
  Serial.println();


  Serial.print("Humidity here: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.println();


}
