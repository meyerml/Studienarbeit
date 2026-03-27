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
#define DEBUG_FLAG
//#undef DEBUG_FLAG

#include <driver/i2s.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "zh_network.h"

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

void zh_network_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

//uint8_t target[6] = {0x58, 0xBF, 0x25, 0x18, 0xC8, 0x04};

typedef struct
{
    char char_value[5];

} example_message_t;

example_message_t send_message = {0};

#define AUDIO_LENGTH 676648




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

    esp_log_level_set("zh_vector", ESP_LOG_NONE);
    esp_log_level_set("zh_network", ESP_LOG_NONE);
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_max_tx_power(8); // Power reduction is for example and testing purposes only. Do not use in your own programs!
    zh_network_init_config_t network_init_config = ZH_NETWORK_INIT_CONFIG_DEFAULT();
    zh_network_init(&network_init_config);
    esp_event_handler_instance_register(ZH_NETWORK, ESP_EVENT_ANY_ID, &zh_network_event_handler, NULL, NULL);
}

void zh_network_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case ZH_NETWORK_ON_RECV_EVENT:;
        {
        zh_network_event_on_recv_t *recv_data = (zh_network_event_on_recv_t*) event_data;
        example_message_t *recv_message = (example_message_t *)recv_data->data;
        #ifdef DEBUG_FLAG
          printf("Message from MAC %02X:%02X:%02X:%02X:%02X:%02X is received. Data lenght %d bytes.\n", MAC2STR(recv_data->mac_addr), recv_data->data_len);
          printf("Char %s\n", recv_message->char_value);
        #endif
        String string = String((const char*)recv_data->data);     
        if (string.startsWith("RING")){
          Spakeflag = true;
        }
        heap_caps_free(recv_data->data); // Do not delete to avoid memory leaks!
        break;
        }
    default:
        break;
    }
}


void loop() {
    if (Spakeflag) {
        //M5.dis.drawpix(0, CRGB(128, 0, 0));
        #ifdef DEBUG_FLAG
        Serial.println("starting i2s sound.");
        #endif
        i2s_write(SPEAK_I2S_NUMBER, audio, AUDIO_LENGTH, &bytes_written,
                  portMAX_DELAY);
        Spakeflag = false;
    }
    
    //M5.update();
}
