#include <painlessMesh.h>

#define statusine LED             2       
#define MESH_SSID       "whateverYouLike"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

#define BLINK_PERIOD    3000   // milliseconds until cycle repeat
#define BLINK_DURATION  100    // milliseconds LED is on for

Scheduler userScheduler;      // to control your personal task
painlessMesh mesh;

String stationNumber;         // id da estação

bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage();           // Prototype
Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage); // start with a five second interval

bool onFlag = false;

Task blinkNoNodes(BLINK_PERIOD, TASK_FOREVER, []() {
  onFlag = !onFlag;
  digitalWrite(LED, onFlag ? HIGH : LOW);
});

// Last values for realistic changes
float lastTemp = 25.0;
float lastHum = 50.0;
float lastPpm = 300.0;
float lastLux = 500.0;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  // Get ESP32 chip ID and format it
  uint64_t chipId = ESP.getEfuseMac();
  stationNumber = "ESP32-" + String((uint16_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
  stationNumber.toUpperCase();            // Optional: convert to uppercase for consistency

  mesh.setDebugMsgTypes(ERROR | DEBUG);   // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));

  Serial.print("Station Number: ");
  Serial.println(stationNumber);
}

void loop() {
  mesh.update();
}

float generateSmallChange(float lastValue, float minChange, float maxChange) {
  float change = random(minChange * 100, maxChange * 100) / 100.0;
  return lastValue + (random(0, 2) == 0 ? -change : change);
}

void sendMessage() {
  String msg = "{\"id\": \"" + stationNumber + "\", ";

  // Generate small realistic changes for temp, hum, ppm, and lux
  lastTemp = generateSmallChange(lastTemp, 0.2, 0.5);
  lastHum = generateSmallChange(lastHum, 0.2, 0.5);
  lastPpm = generateSmallChange(lastPpm, 0.2, 0.5);
  lastLux = generateSmallChange(lastLux, 0.2, 0.5);

  // Append random values to the message
  msg += "\"temp\": " + String(lastTemp, 2) + ", "; 
  msg += "\"hum\": " + String(lastHum, 2) + ", ";   
  msg += "\"ppm\": " + String(lastPpm, 2) + ", ";   
  msg += "\"lux\": " + String(lastLux, 2) + "}";    

  mesh.sendBroadcast(msg);

  if (calc_delay) {
    for (auto node : nodes) {
      mesh.startDelayMeas(node);
    }
    calc_delay = false;
  }

  Serial.printf("Sending message: %s\n", msg.c_str());
  // Serial.println("Message sent.");
}