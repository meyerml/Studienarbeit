
//#define DEBUG_FLAG
//#undef DEBUG_FLAG


#include "nvs_flash.h"
#include "esp_netif.h"
#include "zh_network.h"
#include "driver/rtc_io.h"

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO              GPIO_NUM_3     // Only RTC IO are allowed - ESP32 Pin example

void zh_network_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

uint8_t target[6] = {0xF0, 0x24, 0xF9, 0xBB, 0xE0, 0x44};  //MAC address of M

typedef struct
{
    char char_value[5];
} example_message_t;

example_message_t send_message = {0};
bool button_pressed = false;




void handle_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case 7:{
      #ifdef DEBUG_FLAG
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      #endif
      button_pressed = true;
      break;
    }     
    default:  {
      #ifdef DEBUG_FLAG                      
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      #endif 
      vTaskDelay(20000 / portTICK_PERIOD_MS);
      break;
    }
  }
}


void ring_bell(){

    

    //delay(3000);
    //vTaskDelay(30 / portTICK_PERIOD_MS);

//    zh_network_send(NULL, (uint8_t *)&send_message, sizeof(send_message));  //broadcast
    zh_network_send(target, (uint8_t *)&send_message, sizeof(send_message));

    vTaskDelay(30 / portTICK_PERIOD_MS);


    #ifdef DEBUG_FLAG
      Serial.println("sent message via esp-now-mesh.");
    #endif

  }




void setup() {
  #ifdef DEBUG_FLAG
  Serial.begin(115200);
  delay(500);  //Take some time to open up the Serial Monitor
  #endif
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
  #ifdef CONFIG_IDF_TARGET_ESP8266
      esp_event_handler_register(ZH_NETWORK, ESP_EVENT_ANY_ID, &zh_network_event_handler, NULL);
  #else
      //esp_event_handler_instance_register(ZH_NETWORK, ESP_EVENT_ANY_ID, &zh_network_event_handler, NULL, NULL);
  #endif
  strcpy(send_message.char_value, "RING");


  //#endif
  //Increment boot number and print it every reboot


  //Print the wakeup reason for ESP32
  handle_wakeup_reason();



  if(button_pressed){
    ring_bell();
    //send udp packet
  }

  

      gpio_set_pull_mode(WAKEUP_GPIO, GPIO_PULLDOWN_ONLY);

      esp_deep_sleep_enable_gpio_wakeup(BUTTON_PIN_BITMASK(WAKEUP_GPIO), ESP_GPIO_WAKEUP_GPIO_HIGH);
  
  //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(WAKEUP_GPIO), ESP_EXT1_WAKEUP_ALL_LOW);
  //rtc_gpio_pulldown_dis(WAKEUP_GPIO);  // GPIO33 is tie to GND in order to wake up in HIGH
  //rtc_gpio_pullup_dis(WAKEUP_GPIO);   // Disable PULL_UP in order to allow it to wakeup on HIGH
  //Go to sleep now
  #ifdef DEBUG_FLAG
  Serial.println("Going to sleep now");
  #endif
  esp_deep_sleep_start();
}
void zh_network_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case ZH_NETWORK_ON_SEND_EVENT:;
        {
        zh_network_event_on_send_t *send_data = (zh_network_event_on_send_t*) event_data;
        if (send_data->status == ZH_NETWORK_SEND_SUCCESS)
        {
            printf("Message to MAC %02X:%02X:%02X:%02X:%02X:%02X sent success.\n", MAC2STR(send_data->mac_addr));
        }
        else
        {
            printf("Message to MAC %02X:%02X:%02X:%02X:%02X:%02X sent fail.\n", MAC2STR(send_data->mac_addr));
        }
        break;
        }
    default:
        break;
    }
}

void loop() {
  //This is not going to be called
}
