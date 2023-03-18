#include <ArduinoJson.h>
#include <arduino-timer.h>

// Variables
bool heater = false;
bool stirrer = false;
bool valve_a = false;
bool valve_b = false;
bool valve_c = false;
bool s1 = true;
bool s2 = true;
bool s3 = true;
float temp = 50.0;
bool start = false;
bool stop1 = false;
bool stop2 = false;

// Timer
auto timer = timer_create_default();

void setup() {
  Serial.begin(9600);
  timer.every(100, processControlLogic, nullptr); // Pass nullptr as a second argument
}

void loop() {
  receiveData();
  sendData();
  timer.tick();
}

void receiveData() {
  if (Serial.available()) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, Serial);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    bool simulate = doc["simulate"];

    if (simulate) {
      s1 = doc["sim_s1"];
      s2 = doc["sim_s2"];
      s3 = doc["sim_s3"];
      temp = doc["sim_temp"];
    } else {
      s1 = doc["s1"];
      s2 = doc["s2"];
      s3 = doc["s3"];
      temp = doc["temp"];
    }

    start = doc["start"];
    stop1 = doc["stop1"];
    stop2 = doc["stop2"];
  }
}

void sendData() {
  StaticJsonDocument<256> doc;

  doc["heater"] = heater;
  doc["stirrer"] = stirrer;
  doc["valve_a"] = valve_a;
  doc["valve_b"] = valve_b;
  doc["valve_c"] = valve_c;

  serializeJson(doc, Serial);
  Serial.println();
}

bool processControlLogic(void*) { // Update the function signature
  enum state {
    ready = 0,
    fill_a = 1,
    fill_b = 2,
    heating = 3,
    wait = 4,
    drain1 = 5,
    drain2 = 6
  };

  static state current_state = ready;

  switch (current_state) {
    case ready:
      if (start) {
        current_state = fill_a;
      }
      break;

    case fill_a:
      valve_a = true;
      if (!s2 || stop2) {
        current_state = fill_b;
      }
      break;

    case fill_b:
      valve_a = false;
      valve_b = true;
      heater = true;
      stirrer = true;
      if (!s3 || stop2) {
        current_state = heating;
      }
      break;

    case heating:
      valve_b = false;
      if (temp > 85 || stop2) {
        current_state = wait;
      }
      break;

    case wait:
      heater = false;
      stirrer = false;
      if (stop1) {
        current_state = drain1;
      }
      break;

    case drain1:
      valve_c = true;
      if (!s1 || stop1) {
        current_state = drain2;
      }
      break;

    case drain2:
      valve_c = false;
      current_state = ready;
      break;
         default:
      current_state = ready;
      break;
  }
  return true;
}
