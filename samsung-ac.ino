#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

// WiFi and MQTT parameters
#define WIFI_SSID         "WhiFi"
#define WIFI_PASS         "WiFiPassword"
#define MQTT_SERVER       "192.168.XX.XX"
#define MQTT_PORT         1883
#define MQTT_USER         "mqttuser"
#define MQTT_PASSWORD     "mqttpassword"
#define MQTT_TOPIC        "home/ac/control"

// MQTT Client object
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Variables 
SoftwareSerial sUART(3, 2);  // RX, TX
char receivedChars[200]; 
char charsToSend[200]; 
bool newData = false;
int sendWIFIMGSG = 0;
int DecCounter = 0;
bool AcAck = false;
byte xorTemp;
char rc;

// Sinric Variables
float globalTemperature;
bool globalPowerState;
int globalFanSpeed;

// AC Status
int AcMode = 0;
float RoomTemp = 0;

void setup() {
  // WiFi Connect
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  // Set up MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  // Initialize the UART communication
  sUART.begin(9600);
  FirstFrame();
}
void loop() {
  // Handle MQTT messages
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Read UART data
  UARTReader();

  // Timely send Wifi/Network Status
  if (sendWIFIMGSG == 200){
    byte WithWifi[] = {0xFE ,0x14 ,0x04 ,0x03 ,0x37 ,0x01 ,0x0F};
    SendFrame(7, WithWifi); 
    byte WithIter[] = {0xFE ,0x14 ,0x04 ,0x03 ,0x38 ,0x01 ,0x0F};
    SendFrame(7, WithIter); 
    sendWIFIMGSG = 0;
  }
  sendWIFIMGSG++;
  delay(250);
}
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP_AC_Samsung", MQTT_USER, MQTT_PASSWORD)) {
      mqttClient.subscribe(MQTT_TOPIC);  // Subscribe to the MQTT topic
    } else {
      delay(5000);
    }
  }
}
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string for easier handling
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  // Handling different MQTT commands
  if (String(topic) == MQTT_TOPIC) {
    if (message == "ON") {
      onPowerState(true);  // Turn on the AC
    } else if (message == "OFF") {
      onPowerState(false);  // Turn off the AC
    } else if (message.startsWith("SET_TEMP:")) {
      // Parse temperature setting from the MQTT message
      float temp = message.substring(9).toFloat();
      onTargetTemperature(temp);
    } else if (message.startsWith("SET_MODE:")) {
      // Parse AC mode from MQTT message
      String mode = message.substring(9);
      onThermostatMode(mode);
    } else if (message.startsWith("SET_FAN_SPEED:")) {
      // Parse fan speed from the MQTT message
      int fanSpeed = message.substring(14).toInt();
      onRangeValue(fanSpeed);
    } else if (message.startsWith("CMD:")) {
      // Parse fan speed from the MQTT message
      String Command = message.substring(4);
      sendAnyCMD(Command);
    }
  }
}
void sendMQTTMessage(const char* message) {
  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_TOPIC, message);
  } else {
    reconnectMQTT(); // Ensure connection before sending
  }
}
void UARTReader(){
  
  boolean recvInProgress = false;
  newData = false;
  byte lenght = 0;
  byte ndx = 0;

  while (sUART.available() > 0 && newData == false) { 
    rc = sUART.read();

    if (recvInProgress == true) {
      if (rc != 0xe0) {
        receivedChars[ndx] = rc;
        if (ndx == 3){
            lenght = receivedChars[ndx] + 4;
        }
        ndx++;
      }
      else {
          if (receivedChars[ndx - 1] == CheckUSM(receivedChars, lenght)) {
            receivedChars[ndx] = 0xe0;//preserve endmarker 
            ReciveFrame(receivedChars, lenght);

            recvInProgress = false;
            newData = true;
            ndx = 0;
          } else {
            receivedChars[ndx] = rc;
            ndx++;
          }
      }
    }
    else if (rc == 0xd0) {
      receivedChars[0] = 0xd0;//preserve startMarker
      ndx++;
      recvInProgress = true;
    }

    
    if (lenght >= 52 && ndx == 51) {
      receivedChars[lenght - 1] = 0xe0;
      ReciveFrame(receivedChars, lenght);
      recvInProgress = false;
      newData = true;
      ndx = 0;
    }
  } 
}
void ReciveFrame(char uartmessage[], int length){

  if (uartmessage[11] == 0x12 && uartmessage[12] == 0x06) {
      
    // uartmessage[16] - Power globalPowerState;
    // uartmessage[22] - Mode AcMode = 0;
    // uartmessage[28] - Wind globalFanSpeed;
    // uartmessage[37] - AcTemp globalTemperature;
    // uartmessage[40] - RoomTemp RoomTemp = 0;

    if (uartmessage[16] == 0x0f) {
      globalPowerState = true;
    } else if (uartmessage[16] == 0xf0) {
      globalPowerState = false;
    }

    if (uartmessage[28] == 0x12) {
      globalFanSpeed = 1;
    } else if (uartmessage[28] == 0x14) {
      globalFanSpeed = 2;
    } else if (uartmessage[28] == 0x16) {
      globalFanSpeed = 3;
    }
      
    globalTemperature = uartmessage[37];
  
  }

  if (uartmessage[12] == 0x06) {
      uartmessage[12] = 0x07;

      uartmessage[uartmessage[3] + 2] = CheckUSM(uartmessage,uartmessage[3] + 4);

      uint32_t msgcount = ((uint32_t) uartmessage[6]) << 24; // put the MSB in place
      msgcount |= ((uint32_t) uartmessage[7]) << 16; // add next byte
      msgcount |= ((uint32_t) uartmessage[8]) << 8; // add next byte
      msgcount |= ((uint32_t) uartmessage[9]); // add LSB

      DecCounter = msgcount + 1;
      sUART.write(uartmessage,length);
  }

}
void SendFrame(int lght, byte payload[]){
  delay(1100);
  byte Header[] = {0xD0, 0xC0, 0x02};
  
  uint8_t arrayOfBytes[sizeof(int32_t)];
  arrayOfBytes[0] = (uint8_t)(DecCounter >> 24);
  arrayOfBytes[1] = (uint8_t)(DecCounter >> 16);
  arrayOfBytes[2] = (uint8_t)(DecCounter >> 8);
  arrayOfBytes[3] = (uint8_t)DecCounter;

  for(int i = 0; i <= 2; i++){
  charsToSend[i] = Header[i];
  }

  charsToSend[3] = lght + 8;
  charsToSend[4] = 0x00;
  charsToSend[5] = 0x00;

  for(int i = 6; i <= 9; i++){
  charsToSend[i] = arrayOfBytes[i-6];
  }

  for(int i = 10; i <= 10 + lght; i++){
  charsToSend[i] = payload[i-10];
  }

  charsToSend[lght + 10] = CheckUSM(charsToSend, lght + 12);

  charsToSend[lght + 11] = 0xE0;

  sUART.write(charsToSend,lght + 12);
  DecCounter += 1;
}
byte CheckUSM(char rcvd[] ,int lght){
  // Faz o calculo do checksum
  xorTemp = 0;
  for(int i = 0; i <= lght - 3; i++){
  xorTemp ^= byte(rcvd[i]);
  }
  return xorTemp;
}
void FirstFrame(){
  // First Module MSG
  byte start[] = {0xfe, 0x00};
  sUART.write(start, sizeof(start));

  // Static Start MSGs
  byte msg00[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x01 ,0x01 ,0x0F ,0x74 ,0x01 ,0xF0};
  SendFrame(10, msg00); 
  byte msg01[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x01 ,0x01 ,0x0F ,0x74 ,0x01 ,0xF0};
  SendFrame(10, msg01); 
  byte msg02[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x01 ,0x01 ,0x0F ,0x74 ,0x01 ,0xF0};
  SendFrame(10, msg02); 
  byte msg03[] = {0xFE ,0x14 ,0x04 ,0x0C ,0x17 ,0x01 ,0x14 ,0x18 ,0x01 ,0x04 ,0x19 ,0x01 ,0x03 ,0xFD ,0x01 ,0x02};
  SendFrame(16, msg03); 
  byte msg04[] = {0xFE ,0x14 ,0x04 ,0x0C ,0x17 ,0x01 ,0x14 ,0x18 ,0x01 ,0x04 ,0x19 ,0x01 ,0x03 ,0xFD ,0x01 ,0x02};
  SendFrame(16, msg04); 
  byte msg05[] = {0xFE ,0x14 ,0x04 ,0x12 ,0xFA ,0x01 ,0xBC ,0xFB ,0x01 ,0x8C ,0xFC ,0x01 ,0xCD ,0xF7 ,0x01 ,0xD7 ,0xF8 ,0x01 ,0x20 ,0xF9 ,0x01 ,0x2F};
  SendFrame(22, msg05); 
  byte msg06[] = {0xFE ,0x12 ,0x02 ,0x14 ,0x01 ,0x00 ,0x02 ,0x00 ,0x43 ,0x00 ,0x5A ,0x00 ,0x44 ,0x00 ,0xF7 ,0x00 ,0x5C ,0x00 ,0x73 ,0x00 ,0x62 ,0x00 ,0x63 ,0x00};
  SendFrame(24, msg06); 
  byte msg07[] = {0xFE ,0x13 ,0x02 ,0x10 ,0x32 ,0x00 ,0x40 ,0x00 ,0x44 ,0x00 ,0x43 ,0x00 ,0x75 ,0x00 ,0x76 ,0x00 ,0x77 ,0x00 ,0x78 ,0x00};
  SendFrame(20, msg07); 
  byte msg08[] = {0xFE ,0x14 ,0x04 ,0x03 ,0x37 ,0x01 ,0x0F};
  SendFrame(7, msg08); 
  byte msg09[] = {0xFE ,0x14 ,0x04 ,0x03 ,0x38 ,0x01 ,0xF0};
  SendFrame(7, msg09); 
  byte msg10[] = {0xFE ,0x14 ,0x02 ,0x16 ,0x32 ,0x00 ,0xF6 ,0x00 ,0xF4 ,0x00 ,0xF3 ,0x00 ,0xF5 ,0x00 ,0x39 ,0x00 ,0xE0 ,0x00 ,0xE4 ,0x00 ,0xE8 ,0x00 ,0xE9 ,0x00 ,0xE6 ,0x00};
  SendFrame(26, msg10); 
  byte msg11[] = {0xFE ,0x14 ,0x04 ,0x03 ,0x37 ,0x01 ,0x0F};
  SendFrame(7, msg11); 
  byte msg12[] = {0xFE ,0x14 ,0x04 ,0x03 ,0x38 ,0x01 ,0x0F};
  SendFrame(7, msg12); 
}
// Handle power state change (ON/OFF)
void onPowerState(bool state) {
  globalPowerState = state;
  if (state){
    byte ACON[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x02 ,0x01 ,0x0F ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACON);
  }else{
    byte ACOFF[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x02 ,0x01 ,0xF0 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACOFF);
  }
}
// Handle setting the target temperature
void onTargetTemperature(float temperature) {
  globalTemperature = temperature;
  if (temperature <= 16) {
  byte ACTEMPMIN[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x5a ,0x01 ,0x10 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACTEMPMIN);
  } else if (temperature >= 28) {
  byte ACTEMPMAX[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x5a ,0x01 ,0x1c ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACTEMPMAX);
  } else {
  byte ACTEMP[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x5a ,0x01 ,temperature , 0x74,0x01 ,0x0F}; SendFrame(10, ACTEMP);
  }
}
// Handle mode change (Cool/Heat/Auto/Fan)
void onThermostatMode(String mode) {
  mode.toUpperCase();    
    if (mode == "HEAT") {
      byte ACHEAT[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x43 ,0x01 ,0x42 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACHEAT);
    } 
    if (mode == "COOL") {
      byte ACCOOL[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x43 ,0x01 ,0x12 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACCOOL);
    } 
    if (mode == "AUTO") {
      byte ACAUTO[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x43 ,0x01 ,0xe2 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACAUTO);
    } 
    if (mode == "DRY") {
      byte ACAUTO[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x43 ,0x01 ,0x22 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACAUTO);
    } 
    if (mode == "WIND") {
      byte ACAUTO[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x43 ,0x01 ,0x32 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACAUTO);
    } 
}
// Handle fan speed change (Low/Medium/High)
void onRangeValue(int speed) {
    if (speed == 0) {
      byte ACS1[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x62 ,0x00 ,0x00 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACS1);
    }
    if (speed == 1) {
      byte ACS1[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x62 ,0x01 ,0x12 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACS1);
    }
    if (speed == 2) {
      byte ACS2[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x62 ,0x01 ,0x14 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACS2);
    }
    if (speed == 3) {
      byte ACS3[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x62 ,0x01 ,0x16 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACS3);
    }
    if (speed == 4) {
      byte ACS1[] = {0xFE ,0x12 ,0x04 ,0x06 ,0x62 ,0x01 ,0x18 ,0x74 ,0x01 ,0x0F}; SendFrame(10, ACS1);
    }
}

void sendAnyCMD(String Command) {
    byte CommandHEX[10];

    // Converter a string hexadecimal para array de bytes
    for (int i = 0; i < 10; i++) {
        // Usando substring() que é compatível com a classe String do Arduino
        CommandHEX[i] = strtol(Command.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
    }

    // Enviar os dados através da função SendFrame
    SendFrame(10, CommandHEX);
}
