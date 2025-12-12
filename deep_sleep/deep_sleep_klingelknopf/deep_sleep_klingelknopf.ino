/*
  Deep Sleep with External Wake Up
  =====================================
  This code displays how to use deep sleep with
  an external trigger as a wake up source and how
  to store data in RTC memory to use it over reboots

  This code is under Public Domain License.

  Hardware Connections
  ======================
  Push Button to GPIO 33 pulled down with a 10K Ohm
  resistor

  NOTE:
  ======
  Only RTC IO can be used as a source for external wake
  source. They are pins: 0,2,4,12-15,25-27,32-39.

  Author:
  Pranav Cherukupalli <cherukupallip@gmail.com>
*/
#define DEBUG


#include <WiFi.h>
#include <WiFiUdp.h>
#include "driver/rtc_io.h"

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO              GPIO_NUM_39     // Only RTC IO are allowed - ESP32 Pin example
bool button_pressed = false;

/* WiFi network name and password */
const char *ssid = "westpfalz.freifunk.net";
const char *pwd = "";
// IP address to send UDP data to.
// it can be ip address of the server or 
// a network broadcast address
// here is broadcast address
const char * udpAddress = "10.198.130.147";
const char * broadcastAddress = "10.198.130.255";
const int udpPort = 1234;

WiFiUDP udp;

void handle_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT1:{
      #ifdef DEBUG
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      #endif
      button_pressed = true;
      break;
    }     
    default:  {
      #ifdef DEBUG                      
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      #endif 
      break;
    }
  }
}

void ring_bell(){
  
    //Connect to the WiFi network
    WiFi.begin(ssid, pwd);
        // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      #ifdef DEBUG
      Serial.print(".");
      #endif
    }

    #ifdef DEBUG
    Serial.println("sending udp packet");
    #endif

    uint8_t buffer[50] = "RING";
    //This initializes udp and transfer buffer
    udp.beginPacket(udpAddress, udpPort);
    udp.write(buffer, 4);
    udp.endPacket();
    
    memset(buffer, 0, 50);
    //processing incoming packet, must be called before reading the buffer
    udp.parsePacket();
    //receive response from server, it will be HELLO WORLD
    if(udp.read(buffer, 50) > 0){
      Serial.print("Server to client: ");
      Serial.println((char *)buffer);
    }
    delay(1000);
    
  }

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  delay(1000);  //Take some time to open up the Serial Monitor
  #endif
  //Increment boot number and print it every reboot


  //Print the wakeup reason for ESP32
  handle_wakeup_reason();

  if(button_pressed){
    ring_bell();
    //send udp packet
  }


  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(WAKEUP_GPIO), ESP_EXT1_WAKEUP_ALL_LOW);
  rtc_gpio_pulldown_dis(WAKEUP_GPIO);  // GPIO33 is tie to GND in order to wake up in HIGH
  rtc_gpio_pullup_dis(WAKEUP_GPIO);   // Disable PULL_UP in order to allow it to wakeup on HIGH
  //Go to sleep now
  #ifdef DEBUG
  Serial.println("Going to sleep now");
  #endif
  esp_deep_sleep_start();
}

void loop() {
  //This is not going to be called
}
