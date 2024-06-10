#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>

#define BUTTON 38
#define LED_PIN 13
#define VBATPIN 39
#define NEOPIXEL_PIN 0
#define NUMPIXELS 1
#define SERVO_PIN 27
#define SERVO_CHANNEL 0 // Define a channel for the PWM (0-15)
#define CONTRACTED 7    // 0.5ms in 20ms period. 256 / (20/0.5)
#define EXPANDED 32     // 2.5ms in 20ms period. 256 / (20/2.5)

const char* ssid = "338smart";
const char* password = "qwerty123";
const char* reciver_ip = "172.16.0.4";

WiFiUDP udp;
unsigned int localUdpPort = 4210; 
Adafruit_NeoPixel strip(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void vUDPOutput(void *pvParameters) {
  uint32_t send_delay = *((uint32_t *)pvParameters);
  char packetBuffer[255];
  while (1) {
    sprintf(packetBuffer, "SERVO DUTY: %lu", ledcRead(SERVO_CHANNEL));
    udp.beginPacket(reciver_ip, 4210);
    udp.write((uint8_t*)packetBuffer, strlen(packetBuffer));
    udp.endPacket();
    vTaskDelay(send_delay / portTICK_PERIOD_MS); 
  }
}

void vUDPInput(void *pvParameters) {
  uint32_t read_delay = *((uint32_t *)pvParameters);

  ledcAttachPin(SERVO_PIN, SERVO_CHANNEL);
  ledcSetup(SERVO_CHANNEL, 50, 8);  // 50Hz with 8-bit resolution
  ledcWrite(SERVO_CHANNEL, EXPANDED);

  char packetBuffer[255];
  int packetSize;
  int len;
  while (1) {
    packetSize = udp.parsePacket();
    if (packetSize) {
      len = udp.read(packetBuffer, 255);
      if (len > 0) {
        packetBuffer[len] = 0;
      }
      if (String(packetBuffer) == "CONTRACT") {
        ledcWrite(SERVO_CHANNEL, CONTRACTED);
        strip.setPixelColor(0, strip.Color(0, 0, 5)); // Blue color when contracted
        strip.show(); 
      }
      
      if (String(packetBuffer) == "EXPAND") {
        ledcWrite(SERVO_CHANNEL, EXPANDED);
        strip.setPixelColor(0, strip.Color(0, 5, 0)); // Green color when expanded
        strip.show(); 
      }
    }
    vTaskDelay(read_delay / portTICK_PERIOD_MS); 
  }
}

void vBatteryV(void *pvParameters) {
  char packetBuffer[255];
  uint32_t v;
  while (1) {
    v = (uint32_t)analogReadMilliVolts(VBATPIN);
    v *= 2;
    sprintf(packetBuffer, "BATTERY V: %lu", v);
    udp.beginPacket(reciver_ip, 4210);
    udp.write((uint8_t*)packetBuffer, strlen(packetBuffer));
    udp.endPacket();
    vTaskDelay(2000 / portTICK_PERIOD_MS); 
  }
}

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  uint32_t read_delay = 100;
  uint32_t send_delay = 10;

  strip.setPixelColor(0, strip.Color(0, 5, 0)); // Start with green color
  strip.show();

  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  udp.begin(localUdpPort);
  Serial.printf("UDP port %d\n", localUdpPort);

  xTaskCreate(vUDPInput, "Task Blink", 4096, (void *)&read_delay, 2, NULL);
  xTaskCreate(vBatteryV, "Task Blink", 4096, NULL, 2, NULL);
  xTaskCreatePinnedToCore(vUDPOutput, "Task Blink", 4096, (void *)&send_delay, 2, NULL, 1);
}

void loop() {
}
