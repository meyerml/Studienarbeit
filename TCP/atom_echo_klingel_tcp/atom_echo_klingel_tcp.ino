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
#include <WiFi.h>
//#include <M5Atom.h>
#include <driver/i2s.h>


#define AUDIO_LENGTH 676648
#define DEBUG
#undef DEBUG

//const char *ssid = "WG 2.2";
//const char *password = "opWF6nvt6#v";
const char *ssid = "westpfalz.freifunk.net";
const char *pass = "";

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

const uint16_t serverPort = 8080;
WiFiServer server(serverPort);


// Static IP configuration
IPAddress local_IP(10, 198, 130, 147); // Desired static IP
IPAddress gateway(10, 198, 130, 1);    // Router gateway
IPAddress subnet(255, 255, 255, 0);    // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8);      // Optional: Google DNS

bool InitI2SSpeakOrMic(int mode) {
    esp_err_t err = ESP_OK;

    //i2s_driver_uninstall(SPEAK_I2S_NUMBER);
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
    //M5.begin(true, false, false);
    //M5.dis.clear();
    Serial.begin(115200);

    Serial.println("Init Speaker");
    delay(1000);
    InitI2SSpeakOrMic(MODE_SPK);

    Serial.println("done Init Speaker");
    delay(1000);


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
    
    Serial.println("Wifi connected!");

    server.begin();
    Serial.println("server begun.");
}



void loop() {
    // if( M5.Btn.wasPressed())
    //{
    //    Spakeflag = ( Spakeflag == true )? false : true;
    //}
    WiFiClient client = server.available();
    if(client){
        #ifdef DEBUG
        Serial.println("A client has connected to the server.");
        #endif
        while (client.connected()){
            if(client.available()){
                String line = client.readStringUntil('\n');
                #ifdef DEBUG
                Serial.println("Received: " + line);
                #endif
                if (line.startsWith("RING")) {
                    #ifdef DEBUG
                    Serial.println("Received tcp packet with content RING!");
                    #endif
                    Spakeflag = true;
                } else {
                    #ifdef DEBUG
                    Serial.println("Received tcp packet but not the right string.");
                    #endif
                }
                client.stop();
                #ifdef DEBUG
                Serial.println("client disconnected");
                #endif

            }
        }
        //client.stop();
        #ifdef DEBUG
        Serial.println("exited while-loop.");
        #endif

    } //else{
        //Serial.println("client not found.");
        //delay(1000);
    //}
    if (Spakeflag) {
        //M5.dis.drawpix(0, CRGB(128, 0, 0));
        #ifdef DEBUG
        Serial.println("starting i2s sound.");
        #endif
        i2s_write(SPEAK_I2S_NUMBER, audio, AUDIO_LENGTH, &bytes_written,
                  portMAX_DELAY);
        Spakeflag = false;
    }

    //M5.update();
}
