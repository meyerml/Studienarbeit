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
#include <driver/i2s.h>

#include "M5Atom.h"

#define AUDIO_LENGTH 676648

#include "WiFi.h"
#include "AsyncUDP.h"

//const char *ssid = "WG 2.2";
//const char *password = "opWF6nvt6#v";
const char *ssid = "westpfalz.freifunk.net";
const char *pass = "";
AsyncUDP udp;

extern const unsigned char audio[AUDIO_LENGTH];

#define CONFIG_I2S_BCK_PIN     19
#define CONFIG_I2S_LRCK_PIN    33
#define CONFIG_I2S_DATA_PIN    22
#define CONFIG_I2S_DATA_IN_PIN 23

#define SPEAK_I2S_NUMBER I2S_NUM_0

#define MODE_MIC 0
#define MODE_SPK 1

bool Spakeflag = false;
size_t bytes_written;



// Static IP configuration
IPAddress local_IP(10, 198, 130, 147); // Desired static IP
IPAddress gateway(10, 198, 130, 1);    // Router gateway
IPAddress subnet(255, 255, 255, 0);    // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8);      // Optional: Google DNS

bool InitI2SSpeakOrMic(int mode) {
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(SPEAK_I2S_NUMBER);
    i2s_config_t i2s_config = {
        .mode        = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = 88200,
        .bits_per_sample =
            I2S_BITS_PER_SAMPLE_16BIT,  // is fixed at 12bit, stereo, MSB
        .channel_format       = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 6,
        .dma_buf_len          = 60,
    };
    if (mode == MODE_MIC) {
        i2s_config.mode =
            (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    } else {
        i2s_config.mode     = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }

    Serial.println("Init i2s_driver_install");

    err += i2s_driver_install(SPEAK_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_pin_config_t tx_pin_config;

#if (ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 3, 0))
    tx_pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
#endif

    tx_pin_config.bck_io_num   = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num    = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num  = CONFIG_I2S_DATA_IN_PIN;

    Serial.println("Init i2s_set_pin");
    err += i2s_set_pin(SPEAK_I2S_NUMBER, &tx_pin_config);
    Serial.println("Init i2s_set_clk");
    err += i2s_set_clk(SPEAK_I2S_NUMBER, 88200, I2S_BITS_PER_SAMPLE_16BIT,
                       I2S_CHANNEL_MONO);

    return true;
}

void setup() {
    M5.begin(true, false, true);
    M5.dis.clear();

    Serial.println("Init Spaker");
    InitI2SSpeakOrMic(MODE_SPK);
    delay(100);


    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);


    // Configures static IP address
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
        Serial.println("Failed to configure static IP");
    }


    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
    if (udp.listen(1234)) {
    Serial.print("WiFi connected. UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
        Serial.print("UDP Packet Type: ");
        Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
        Serial.print(", From: ");
        Serial.print(packet.remoteIP());
        Serial.print(":");
        Serial.print(packet.remotePort());
        Serial.print(", To: ");
        Serial.print(packet.localIP());
        Serial.print(":");
        Serial.print(packet.localPort());
        Serial.print(", Length: ");
        Serial.print(packet.length()); //dlzka packetu
        Serial.print(", Data: ");
        Serial.write(packet.data(), packet.length());
        Serial.println();
        String myString = (const char*)packet.data();
        if (myString.startsWith("RING")) {
            Serial.println("Received udp packet!");
            Spakeflag = true;
        } else {
            Serial.println("Received UDP packet but not the right string.");
        }
        packet.printf("Got %u bytes of data", packet.length());
        });
    }
}



void loop() {
    // if( M5.Btn.wasPressed())
    //{
    //    Spakeflag = ( Spakeflag == true )? false : true;
    //}

    if (Spakeflag) {
        M5.dis.drawpix(0, CRGB(128, 0, 0));

        i2s_write(SPEAK_I2S_NUMBER, audio, AUDIO_LENGTH, &bytes_written,
                  portMAX_DELAY);
        Spakeflag = false;
    }

    M5.update();
}
