/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-mesh-esp32-esp8266-painlessmesh/
  
  This is a simple example that uses the painlessMesh library: https://github.com/gmag11/painlessMesh/blob/master/examples/basic/basic.ino
*/

#include "painlessMesh.h"

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  String msg = "Hi from node2";
  //msg += mesh.getNodeId();
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

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

void setup() {
  Serial.begin(115200);

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  //mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  //mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  //userScheduler.addTask( taskSendMessage );
  //taskSendMessage.enable();

    // it will run the user scheduler as well
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  String msg3 = "from setup broadcast";
  String msg4 = "from setup single";

  bool setupbroadcastresult =  mesh.sendBroadcast( msg3 );
  bool setupsingleresult = mesh.sendSingle(4189839429, msg4);
  Serial.print("setupbroadcastresult: ");
  Serial.println((int) setupbroadcastresult);

  Serial.print("setupsingleresult: ");
  Serial.println((int) setupsingleresult);

    
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();
  mesh.update();


}

void loop() {
  // it will run the user scheduler as well
  //delay(1000);
  mesh.update();
  //String msg2 = "signle";
  //msg += mesh.getNodeId();
  //mesh.sendBroadcast( msg2 );
  //bool result = mesh.sendSingle(4189839429, msg2);
  //Serial.print("result: "+ (int) result);
  //Serial.println((int) result);
  vTaskDelay(1000 / portTICK_PERIOD_MS);

}
