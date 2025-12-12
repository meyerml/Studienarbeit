/****************************************************************
 *
 * This Example only for M5AtomEcho!
 *
 * Arduino tools Setting
 * -board : M5StickC
 * -Upload Speed: 115200 / 750000 / 1500000
 * -partition Scheme: No OTA
 *
 ****************************************************************/

#include "M5Atom.h"

#include <WiFi.h>
#include <WiFiUdp.h>


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


void setup() {
    M5.begin(true, false, true);
    M5.dis.clear();


    //Connect to the WiFi network
    WiFi.begin(ssid, pwd);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

bool Spakeflag = false;
size_t bytes_written;

void loop() {
    if( M5.Btn.wasPressed())
    {
        Spakeflag = ( Spakeflag == true )? false : true;
    }

    if (Spakeflag) {
        Spakeflag = false;
        M5.dis.drawpix(0, CRGB(128, 0, 0));


        Serial.println("sending udp packet");
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

    M5.update();
}
