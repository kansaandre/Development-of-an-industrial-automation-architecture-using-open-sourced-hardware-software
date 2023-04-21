// LAST UPDATE (roughly): 21.04.2023 20:00
// Control Layer of "Development of an industrial automation architecture" --> GITHUB https://bit.ly/3TAT78J

// NOTE! In code, a lot of referencing to the thesis document is done to clarify/document code
// this currently is referencing to thesis version ------->  version. 1.0 = v.1.0  <---------- , 
// remember to UPDATE with NEW versions/iterations of the thesis document. Document version seen in front page.

// Short description about the project:
// This open-source, community-driven project aims to develop an industrial automation architecture. 
// Its vast resources, including source code, forums, and documentation, make it easier to integrate 
// AI tools like ChatGPT, aiding coding, architecture design, and hardware selection. 
// This sets it apart from confidential industrial vendors.

//INDUSTRIAL AUTOMATION ARCHITECTURE
  //DATASTORAGE-LAYER = DGRAPH 
  //HMI-LAYER = NODE-RED
  //CONTROL-LAYER = ARDUINO (HERE)
  //PROCESS-LAYER = SENSORS (SIMULATED BY NODE-RED)

#include <ArduinoJson.h> //https://arduinojson.org/ // JSON data compatability with Arduino
//-------------------------------------------------------------------------------------------------------------------//

//Setup of interrupt in our timer1 lib as well as serial commuication

void setup(){ // The setup() function is executed only once, when the Arduino board is powered on or reset
    
//Serial communication setup
  Serial.begin(9600); // //9600 baud per seconds (bits per seconds)
  delay(10000); // Wait until serial commuication is up and running before "starting" program.
}

//-------------------------------------------------------------------------------------------------------------------//
      
// Global variable declaration with their normal default values

  // JSON properties sent between layers
    // Buttons (input)
      boolean start = false; // Start button to start sequence
      boolean stop1 = false; // Stop button (NORMAL) will stop sequence at state "ready"
      boolean stop2 = false; // Stop button (EMERGENCY) will stop sequence immediately and return to "ready"
      
    // Actuators (output)
      boolean heater = false; // Heater for heating up tank
      boolean stirrer = false; // Stirrer for stirring up tank
      boolean valveA = false; // Inlet valve for chemical A
      boolean valveB = false; // Inlet valve for chemical B
      boolean valveC = false; // Outlet valve for chemical C
    
    // Sensors (input)
      boolean s1 = true; // Low level indicator in tank (Normally Closed (NC)) 
      boolean s2 = true; // Medium level indicator in tank (Normally Closed (NC))
      boolean s3 = true; // High level indicator in tank (Normally Closed (NC))
      float temp = 20; //[C] Temperature sensor located inside tank 
    
    // Program Variables (program variables)
      uint8_t counter = 0; // Counter used to count how many times sequence has looped
      String flow = ""; // Variable used to identify string when sent on serial line in JSON format
      boolean overridemode = false; // Set in the UI when operators/engineers override values manually
      String error = ""; // Error message that can be used to detect error in communication when transferring to another layer
      uint16_t TimeInSequenceJSON; // s // TimeInSequence varible divided by 1000 = seconds since sequence was started
      uint16_t TimeRunningJSON; // s // TimeRunning variable divided by 1000 = seconds since program was uplodaded to microcontroller
    
    // JSON declaration direct
      String jsonstring = ""; // Used to send/receive JSON as string between layers
      boolean SerialReady = false; // Used for handshake agreement when HMI Layer is to send data to Control Layer.
                                   // Needed to make sure we are listening on the serial port before the large JSON
                                   // data string is sent. Can be removed with larger serial receiver buffer....
      
    // States (program variables)
    enum states { // Declare our states made directly from Figure 9. in the thesis document.
      pause = 0, 
      ready = 1,
      fill_A = 2,
      fill_B = 3,
      heating = 4,
      wait = 5,
      drain1 = 6,
      drain2 = 7
    };
      states state = ready; // Default/initial state

  //Properties only found in the Control Layer (meaning not sent to any other LAYER)
  
    //Overall
    unsigned long TimeRunning = millis(); // ms // Tracking time since program was uploaded to microcontroller.
    
    //Setup of JSON // JSON is used as our communication data interchange between layers 
    StaticJsonDocument<300> JsonMemory; // Estimated from https://arduinojson.org/v6/assistant/#/step3 (18.04.2023)
    StaticJsonDocument<100> JsonSerialReady;
    
    //StateMachine()
      unsigned long current_time; // ms // Used to track time for a condition in one state in our StateMachine function
      unsigned long TimeInSequence; // ms // Track time since we were last in state: Ready (meaning time since sequence begun)
      //(millis() - EntryTime) < TimeOut) Exit conditions to compensate for blocking code in our StateMachine()
        const uint8_t TimeOut = 50; // ms // Maximum time allowed to stay inside StateMachine() loop before condition is met jumping back to void loop() (Too low value cause errors due to variables not being properly set CAREFUL)
        uint32_t EntryTime; // ms // Start tracking time as soon as we enter a state so we know how long we been there.
        
    //LogicForceFreezeRead()
      char c; // When we read character by character from large JSON data that is sent to this layer, the Control Layer.
            // Must be done this way due to small receiver serial buffer layer (64 bytes for Arduno UNO microcontroller)
    
      //(millis()-SerialWait < SerialTimeOut) Exit condition when serial listening for reading JSON data from outside layer commuication 
        unsigned long SerialWait; // ms // Set when we begin listening on serial port
        uint16_t SerialTimeOut = 25000; // ms // If no data arrive within said timeout, we jump out of while loop.
        
//-------------------------------------------------------------------------------------------------------------------//

void StateMachine(){ // Main function for executing process logic sequence // Control Process Logic Loop (see Figure 8. in thesis document v1.0)

  switch (state) {
    case ready:
      EntryTime = millis();
      while ((millis() - EntryTime) < TimeOut) {
        TimeInSequence = 0; // Reset sequence time tracker
        if (start) {
          TimeInSequence = millis(); // Start counting time since we entered the sequence
          state = fill_A; // Change state to fill_A when start button is pressed
        }
      }
      break;

    case fill_A:
      EntryTime = millis();
      while ((millis() - EntryTime) < TimeOut) {
        counter++; // Increase counter for each sequence loop
        valveA = true; // Open inlet valve A for chemical A
        if (!s2 || stop2) {
          state = fill_B; // Change state to fill_B when medium level indicator in the tank is reached or stop2 is true
        }
      }
      break;

    case fill_B:
      EntryTime = millis();
      while ((millis() - EntryTime) < TimeOut) {
        valveA = false; // Close valve A
        valveB = true; // Open inlet valve B for chemical B
        stirrer = true; // Start stirrer for mixing chemicals A and B
        heater = true; // Start heater for warming up the mixture
        if (!s3 || stop2) {
          state = heating; // Change state to heating when high level indicator in the tank is reached or stop2 is true
        }
      }
      break;

    case heating:
      EntryTime = millis();
      while ((millis() - EntryTime) < TimeOut) {
        if (temp >= 85 || stop2) {
          state = wait; // Change state to wait when the temperature reaches a certain level or stop2 is true
        }
      }
      break;

    case wait:
      EntryTime = millis();
      while ((millis() - EntryTime) < TimeOut) {
        current_time = millis(); // Save time when first entering the wait state
        if ((millis() - current_time) >= 30000 || stop2) {
          state = drain1; // Change state to drain1 after 30 seconds or when stop2 is true
        }
      }
      break;

    case drain1:
      EntryTime = millis();
      while ((millis() - EntryTime) < TimeOut) {
        heater = false; // Stop heater
        valveC = true; // Open outlet valve C for draining the mixture
        if (s2 || stop2) {
          state = drain2; // Change state to drain2 when tank level is below medium level indicator or stop2 is true
        }
      }
      break;

    case drain2:
      EntryTime = millis();
      while ((millis() - EntryTime) < TimeOut) {
        stirrer = false; // Stop stirrer
        if ((s1 || stop2) && (counter == 10 || stop1)) {
          state = ready; // Stop loop and return to ready state given conditions above are met
        } else if (s1 && counter < 10 && !stop1 && !stop2) {
          state = fill_A; // Restart loop/program, moving back to fill_A state given conditions above are met
          }
      }
      break;
  }
  
}

//-------------------------------------------------------------------------------------------------------------------//

// FUNCTION SETUPS AND JSON CREATION
// Here all communication from control layer aka as here in Arduino with the HMI layer as well as the process layer.
// What we actually have added here is visualized in figure 9 in thesis document v1.0.

void loop(){

  JsonMemory.clear(); // Clear JsonMemory // Done to have the document globally declared https://arduinojson.org/v6/how-to/reuse-a-json-document/
  JsonSerialReady.clear(); // Clear JsonSerialReady // Done to have the document globally declared https://arduinojson.org/v6/how-to/reuse-a-json-document/
    
  // Keep track of time variables, see declaration for more info.
    TimeInSequence = TimeRunning - TimeInSequence; // Tracking time in sequence meaning (state != ready)
    TimeInSequenceJSON = TimeInSequence/1000; // s
    TimeRunningJSON = TimeRunning/1000; // s
  
  // Call our functions
    StateMachine(); // Control Logic // Calling main function for executing process logic sequence
    WriteInUpdatedVariables(); // Step 1 // Calling function that writes In Updated Variables updated by StateMachine()
    SensorLogicDataWrite(); // Step 2 // Calling function that sends data from Control Layer (aka here from Arduino) to the HMI Layer (aka Node-RED)
    //Step 3 - HMI Layer which includes the user interface with override functionality - Hosted in Node-RED. //The UI modifies the data sent to Node-RED in SensorLogicDataWrite() and returns the modified data in LogicForceFreezeRead()   
    LogicForceFreezeRead(); // Step 4 // Calling function that sends data from HMI Layer to Control Layer.

}

//-------------------------------------------------------------------------------------------------------------------//

void WriteInUpdatedVariables(){ // Step 1 (figure 9. thesis document v1.0)

  // Step 1 - Write In Updated Variables updated by StateMachine() - (see Figure 9. from thesis document v1.0)
  // Here we will update our variables which may have been given new values from the control logic code above.
  // We write our variables into memory allocated to the StaticJsonDocument where it will be stored ready for transmission.
  // Check declaration in top of code for explanation about the variables
  
  JsonMemory["start"] = start;
  JsonMemory["stop1"] = stop1;
  JsonMemory["stop2"] = stop2;
  JsonMemory["heater"] = heater;
  JsonMemory["stirrer"] = stirrer;
  JsonMemory["valveA"] = valveA;
  JsonMemory["valveB"] = valveB;
  JsonMemory["valveC"] = valveC;
  JsonMemory["s1"] = s1;
  JsonMemory["s2"] = s2;
  JsonMemory["s3"] = s3;
  JsonMemory["temp"] = temp;
  JsonMemory["state"] = state;
  JsonMemory["counter"] = counter;
  JsonMemory["flow"] = flow;
  JsonMemory["overridemode"] = overridemode;
  JsonMemory["error"] = error;
  JsonMemory["TimeInSequence"] = TimeInSequenceJSON;
  JsonMemory["TimeRunning"] = TimeRunningJSON;
  

}

//----------------------------------------------------------

void SensorLogicDataWrite() { // Step 2 (figure 9. thesis document v1.0)

  // Sensor & Logic data write - Send data from Control Layer (aka here from Arduino) to the HMI Layer (aka Node-RED)

    flow = "SensorLogicDataWrite"; // Identification property in our JSON data // used with figure 9. from thesis document v1.0.
    JsonMemory["flow"] = flow;
    
    jsonstring = ""; // clear jsonstring
    
    serializeJson(JsonMemory, jsonstring); // Function that converts JSON object to a string.
    Serial.println(jsonstring); // Function that prints text to the Serial Monitor.
    
}

//----------------------------------------------------------

void LogicForceFreezeRead() { // Step 4 (figure 9. thesis document v1.0)
  // Step 4 - Logic force & freeze read - Send data from HMI Layer to Control Layer. Here we send the exact same data as in step 2
  // - but update our variables from the user interface inputs.
  
  // Request sent to HMI Layer (Node-RED) as LogicForceFreezeRead JSON
    flow = "RequestLogicForceFreezeRead";
    SerialReady = true; // Ready to listen to serial line and read out "LogicForceFreezeRead". Due to size, it must be done directly and we can not "store" it in Serial Buffer (at least for UNO)...
    
    JsonSerialReady["flow"] = flow; // Identification property of flow
    JsonSerialReady["SerialReady"] = SerialReady; // Signalling to Node-RED that we are ready to read serial data from it.
    
    jsonstring = ""; // clear jsonstring
    
    serializeJson(JsonSerialReady, jsonstring); // Function that converts JSON object to a string.
    Serial.println(jsonstring); // Function that prints text to the Serial Monitor.
    
  // Listen to serial port and read out JSON data from HMI Layer
    jsonstring = ""; // clear jsonstring
    SerialWait = millis(); // set it to current time used to track time since we began listening for data from HMI layer (Node-RED) at serial port
    
    while ((Serial.available() == 0) and (millis() - SerialWait < SerialTimeOut)) {
    // Do nothing; just wait for data
    delay(1); // Just to keep it from going bananas
    }
    
    while (Serial.available() > 0) {
    c = (char)Serial.read(); // Read one character from the serial buffer
    jsonstring += c;
    }
  
  // Check if any data was received
    if (jsonstring.length() > 0) {
      deserializeJson(JsonMemory, jsonstring); // Store serial data string in JSON memory, effectively making it into a JSON object
      flow = "test"; // Identification property
      JsonMemory["flow"] = flow;
      
      jsonstring = "";
    
      serializeJson(JsonMemory, jsonstring);
      Serial.println(jsonstring);
      
    } else {
        // Handle the case when no data was received or timeout occurred
        // You can add error handling or logging here
      }
  
  // Sending stop signal to the close function in Node-RED stopping serial data from the HMI layer to Control Layer
    flow = "RequestLogicForceFreezeRead";
    SerialReady = false; // Close gate
    JsonSerialReady["flow"] = flow; // Identification property
    JsonSerialReady["SerialReady"] = SerialReady; // Ready to read serial data sent from HMI Layer (aka Node-RED)
    
    jsonstring = ""; // clear jsonstring
    
    serializeJson(JsonSerialReady, jsonstring); // Send stop signal to HMI Layer
    Serial.println(jsonstring); // Send stop signal to HMI Layer

}

//-------------------------------------------------------------------------------------------------------------------//
