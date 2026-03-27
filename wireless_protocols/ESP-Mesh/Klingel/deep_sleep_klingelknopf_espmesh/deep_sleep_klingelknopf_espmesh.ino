/*
for codecell
*/
#define DEBUG_FLAG
//#undef DEBUG_FLAG


#include "painlessMesh.h"
#include "driver/rtc_io.h"

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO              GPIO_NUM_5     // Only RTC IO are allowed - ESP32 Pin example
#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task

painlessMesh  mesh;
void sendMessage() ; // Prototype so PlatformIO doesn't complain
void goToSleep();
Task taskSendMessage( 1000 , TASK_FOREVER, &sendMessage );
Task taskGoToSleep(10000, TASK_FOREVER, &goToSleep);

int counter = 0;
void goToSleep(){
  counter += 1;
  if (counter > 2){
  gpio_set_pull_mode(WAKEUP_GPIO, GPIO_PULLDOWN_ONLY);

  esp_deep_sleep_enable_gpio_wakeup(BUTTON_PIN_BITMASK(WAKEUP_GPIO), ESP_GPIO_WAKEUP_GPIO_HIGH);
  //rtc_gpio_pulldown_dis(WAKEUP_GPIO);  // GPIO33 is tie to GND in order to wake up in HIGH
  //rtc_gpio_pullup_dis(WAKEUP_GPIO);   // Disable PULL_UP in order to allow it to wakeup on HIGH
  //Go to sleep now
  #ifdef DEBUG_FLAG
  Serial.println("Going to sleep now");
  #endif
  esp_deep_sleep_start();
  }
}

void sendMessage() {
  String msg = "RING";
  //msg += mesh.getNodeId();
  mesh.sendBroadcast( msg );
  //taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
  taskSendMessage.setInterval(2000);
  //delay(3000);
  taskGoToSleep.enable();

  /*
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(WAKEUP_GPIO), ESP_EXT1_WAKEUP_ALL_LOW);
  rtc_gpio_pulldown_dis(WAKEUP_GPIO);  // GPIO33 is tie to GND in order to wake up in HIGH
  rtc_gpio_pullup_dis(WAKEUP_GPIO);   // Disable PULL_UP in order to allow it to wakeup on HIGH
  //Go to sleep now
  #ifdef DEBUG_FLAG
  Serial.println("Going to sleep now");
  #endif
  esp_deep_sleep_start();
  
*/
}

bool button_pressed = false;

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
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


void handle_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case 7:{
      #ifdef DEBUG_FLAG
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      #endif
      button_pressed = true;
        mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  //mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  userScheduler.addTask( taskSendMessage );
  userScheduler.addTask( taskGoToSleep );

    taskSendMessage.enable();

      break;
    }     
    default:  {
      #ifdef DEBUG_FLAG                      
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      #endif 
      gpio_set_pull_mode(WAKEUP_GPIO, GPIO_PULLDOWN_ONLY);

      esp_deep_sleep_enable_gpio_wakeup(BUTTON_PIN_BITMASK(WAKEUP_GPIO), ESP_GPIO_WAKEUP_GPIO_HIGH);
  
      //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(WAKEUP_GPIO), ESP_EXT1_WAKEUP_ALL_LOW);
      //rtc_gpio_pulldown_dis(WAKEUP_GPIO);  // GPIO33 is tie to GND in order to wake up in HIGH
      //rtc_gpio_pullup_dis(WAKEUP_GPIO);   // Disable PULL_UP in order to allow it to wakeup on HIGH
      //Go to sleep now
      delay(60000);

      #ifdef DEBUG_FLAG
      Serial.println("Going to sleep now");
      #endif
      esp_deep_sleep_start();

      break;
    }
  }
}


void ring_bell(){
  //vTaskDelay(1000 / portTICK_PERIOD_MS);
  //String msg2 = "RING";
  //bool result = mesh.sendBroadcast( msg2 );

  //Serial.print("result: "+ (int) result);
  //Serial.println((int) result);
  //vTaskDelay(1000 / portTICK_PERIOD_MS);


    #ifdef DEBUG_FLAG
      Serial.println("sent broadcast message via esp-mesh.");
    #endif

  }




void setup() {
  #ifdef DEBUG_FLAG
  Serial.begin(115200);
  //delay(500);  //Take some time to open up the Serial Monitor
  #endif
  //Increment boot number and print it every reboot




  //Print the wakeup reason for ESP32
  handle_wakeup_reason();







}

void loop() {


    mesh.update();



}
