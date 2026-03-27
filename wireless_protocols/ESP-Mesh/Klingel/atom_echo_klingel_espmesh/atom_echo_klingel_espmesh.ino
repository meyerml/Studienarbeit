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
#include "painlessMesh.h"

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

#define AUDIO_LENGTH 676648
#define DEBUG_FLAG
//#undef DEBUG_FLAG



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



// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
    Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());

    

    if (msg.startsWith("RING")) {
        #ifdef DEBUG_FLAG
        Serial.println("Received esp-mesh message with content RING!");
        #endif
        Spakeflag = true;
    } else {
        #ifdef DEBUG_FLAG
        Serial.println("Received esp-mesh message but not the right string.");
        #endif
    }
    
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}


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

    Serial.begin(115200);

    Serial.println("Init Speaker");
    delay(1000);
    InitI2SSpeakOrMic(MODE_SPK);

    Serial.println("done Init Speaker");
    delay(1000);



    //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
    mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

    mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

}



void loop() {
    // if( M5.Btn.wasPressed())
    //{
    //    Spakeflag = ( Spakeflag == true )? false : true;
    //}
    




    
    if (Spakeflag) {
        //M5.dis.drawpix(0, CRGB(128, 0, 0));
        #ifdef DEBUG_FLAG
        Serial.println("starting i2s sound.");
        #endif
        i2s_write(SPEAK_I2S_NUMBER, audio, AUDIO_LENGTH, &bytes_written,
                  portMAX_DELAY);
        Spakeflag = false;
    }
    
    mesh.update();
    //M5.update();
}
