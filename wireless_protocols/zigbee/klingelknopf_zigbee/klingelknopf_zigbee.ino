
#define DEBUG_FLAG
#undef DEBUG_FLAG

#include "driver/rtc_io.h"


#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO              GPIO_NUM_3     // Only RTC IO are allowed - ESP32 Pin example

#include "OThreadCLI.h"
#include "OThreadCLI_Util.h"

//#define USER_BUTTON           GPIO_NUM_3  // C6/H2 Boot button
#define OT_CHANNEL            "24"
#define OT_NETWORK_KEY        "00112233445566778899aabbccddeeff"
#define OT_MCAST_ADDR         "ff05::abcd"
#define OT_COAP_RESOURCE_NAME "Lamp"


const char *otSetupChild[] = {
  // -- clear/disable all
  // stop CoAP
  "coap", "stop",
  // stop Thread
  "thread", "stop",
  // stop the interface
  "ifconfig", "down",
  // clear the dataset
  "dataset", "clear",
  // -- set dataset
  // set the channel
  "dataset channel", OT_CHANNEL,
  // set the network key
  "dataset networkkey", OT_NETWORK_KEY,
  // commit the dataset
  "dataset", "commit active",
  // -- network start
  // start the interface
  "ifconfig", "up",
  // start the Thread network
  "thread", "start"
};

const char *otCoapSwitch[] = {
  // -- start CoAP as client
  "coap", "start"
};

bool otDeviceSetup(
  const char **otSetupCmds, uint8_t nCmds1, const char **otCoapCmds, uint8_t nCmds2, ot_device_role_t expectedRole1, ot_device_role_t expectedRole2
) {
  Serial.println("Starting OpenThread.");
  Serial.println("Running as Switch - use the BOOT button to toggle the other C6/H2 as a Lamp");
  uint8_t i;
  for (i = 0; i < nCmds1; i++) {
    if (!otExecCommand(otSetupCmds[i * 2], otSetupCmds[i * 2 + 1])) {
      break;
    }
  }
  if (i != nCmds1) {
    log_e("Sorry, OpenThread Network setup failed!");
    //rgbLedWrite(RGB_BUILTIN, 255, 0, 0);  // RED ... failed!
    return false;
  }
  Serial.println("OpenThread started.\r\nWaiting for activating correct Device Role.");
  // wait for the expected Device Role to start
  uint8_t tries = 24;  // 24 x 2.5 sec = 1 min
  
  while (tries && OThread.otGetDeviceRole() != expectedRole1 && OThread.otGetDeviceRole() != expectedRole2) {
    Serial.print(".");
    delay(2500);
    tries--;
  }
  Serial.println();
  if (!tries) {
    log_e("Sorry, Device Role failed by timeout! Current Role: %s.", OThread.otGetStringDeviceRole());
    //rgbLedWrite(RGB_BUILTIN, 255, 0, 0);  // RED ... failed!
    return false;
  }
  Serial.printf("Device is %s.\r\n", OThread.otGetStringDeviceRole());
  for (i = 0; i < nCmds2; i++) {
    if (!otExecCommand(otCoapCmds[i * 2], otCoapCmds[i * 2 + 1])) {
      break;
    }
  }
  if (i != nCmds2) {
    log_e("Sorry, OpenThread CoAP setup failed!");
    //rgbLedWrite(RGB_BUILTIN, 255, 0, 0);  // RED ... failed!
    return false;
  }
  Serial.println("OpenThread setup done. Node is ready.");
  // all fine! LED goes and stays Blue
  ///rgbLedWrite(RGB_BUILTIN, 0, 0, 64);  // BLUE ... Switch is ready!
  return true;
}

void setupNode() {
  // tries to set the Thread Network node and only returns when succeeded
  bool startedCorrectly = false;
  while (!startedCorrectly) {
    startedCorrectly |= otDeviceSetup(
      otSetupChild, sizeof(otSetupChild) / sizeof(char *) / 2, otCoapSwitch, sizeof(otCoapSwitch) / sizeof(char *) / 2, OT_ROLE_CHILD, OT_ROLE_ROUTER
    );
    if (!startedCorrectly) {
      Serial.println("Setup Failed...\r\nTrying again...");
    }
  }
}

// Sends the CoAP frame to the Lamp node
bool otCoapPUT(bool lampState) {
  bool gotDone = false, gotConfirmation = false;
  String coapMsg = "coap put ";
  coapMsg += OT_MCAST_ADDR;
  coapMsg += " ";
  coapMsg += OT_COAP_RESOURCE_NAME;
  coapMsg += " con 0";

  // final command is "coap put ff05::abcd Lamp con 1" or "coap put ff05::abcd Lamp con 0"
  if (lampState) {
    coapMsg[coapMsg.length() - 1] = '1';
  }
  OThreadCLI.println(coapMsg.c_str());
  log_d("Send CLI CMD:[%s]", coapMsg.c_str());

  char cliResp[256];
  // waits for the CoAP confirmation and Done message for about 1.25 seconds
  // timeout is based on Stream::setTimeout()
  // Example of the expected confirmation response: "coap response from fdae:3289:1783:5c3f:fd84:c714:7e83:6122"
  uint8_t tries = 5;
  *cliResp = '\0';
  while (tries && !(gotDone && gotConfirmation)) {
    size_t len = OThreadCLI.readBytesUntil('\n', cliResp, sizeof(cliResp));
    cliResp[len - 1] = '\0';
    log_d("Try[%d]::MSG[%s]", tries, cliResp);
    if (strlen(cliResp)) {
      if (!strncmp(cliResp, "coap response from", 18)) {
        gotConfirmation = true;
      }
      if (!strncmp(cliResp, "Done", 4)) {
        gotDone = true;
      }
    }
    tries--;
  }
  if (gotDone && gotConfirmation) {
    return true;
  }
  return false;
}

bool button_pressed = false;

bool lastLampState = false;


void handle_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  Serial.print("Wakeup reason: ");
  Serial.println(wakeup_reason);
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

    


lastLampState = !lastLampState;
    if (!otCoapPUT(lastLampState)) {  // failed: Lamp Node is not responding due to be off or unreachable
      // timeout from the CoAP PUT message... restart the node.
      //rgbLedWrite(RGB_BUILTIN, 255, 0, 0);  // RED ... something failed!
      Serial.println("Resetting the Node as Switch... wait.");
      // start over...
      setupNode();
    } else {
      Serial.println("did it");
    }

    //delay(3000);
    //vTaskDelay(30 / portTICK_PERIOD_MS);


    vTaskDelay(30 / portTICK_PERIOD_MS);


    #ifdef DEBUG_FLAG
      Serial.println("toggled light via thread");
    #endif

  }




void setup() {
  #ifdef DEBUG_FLAG
  Serial.begin(115200);
  delay(500);  //Take some time to open up the Serial Monitor
  #endif

  OThread.begin(true);  // No AutoStart is necessary
  OThreadCLI.begin();
  OThreadCLI.setTimeout(250);  // waits 250ms for the OpenThread CLI response
  setupNode();

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


void loop() {
  //This is not going to be called
}
