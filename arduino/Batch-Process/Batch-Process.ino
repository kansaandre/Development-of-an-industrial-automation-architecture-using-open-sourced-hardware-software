// LAST UPDATE (roughly): 26.04.2023 00:04
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

#include <ArduinoJson.h> // v6 used in v1.0 // https://arduinojson.org/ // JSON data compatability with Arduino

//Enter "sudo nano /usr/share/arduino/hardware/arduino/avr/cores/arduino/HardwareSerial.h" and change from the default 64 byte buffer size (for UNO anyway) to 300 bytes by changing "#define SERIAL_RX_BUFFER_SIZE 300"
  //This is neccessary to be able to receive the large JSON data string we are receving from the HMI layer (aka Node-RED)...

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
      uint16_t TimeInSequenceJSON; // s // TimeInSequence varible divided by 1000 = seconds since sequence was started
      uint16_t TimeRunningJSON; // s // TimeRunning variable divided by 1000 = seconds since program was uplodaded to microcontroller
    
    // JSON declaration variables
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

    //WriteInUpdatedVariables()
      boolean i = false; // Used to call InitJsonMemory() only first the time WriteInUpdatedVariables() is called
    
    //StateMachine()
      unsigned long current_time; // ms // Used to track time for a condition in one state in our StateMachine function
      unsigned long TimeInSequence; // ms // Track time since we were last in state: Ready (meaning time since sequence begun)
      unsigned long SetTimeInSequence; // ms // Time set since transition from ready to fill_A happens
      const uint16_t TimeOut = 1000; // ms // Maximum time allowed to stay inside StateMachine() loop before condition is met jumping back to void loop() (Too low value cause errors due to variables not being properly set CAREFUL)
      boolean PreviouslyVisited = false; // Have we previously visited a case state in the StateMachine() switch function and completed instructions meant to only be done once?

    //(millis()-SerialWait < SerialTimeOut) Exit condition when serial listening for reading JSON data from outside layer commuication 
    unsigned long SerialWait; // ms // Set when we begin listening on serial port
    uint16_t SerialTimeOut = 5000; // ms // If no data arrive within said timeout, we jump out of while loop.

    //LogicForceFreezeRead() 
      char c; // When we read character by character from large JSON data that is sent to this layer, the Control Layer.
            // Must be done this way due to small receiver serial buffer layer (64 bytes for Arduno UNO microcontroller)
      String jsonstringLastValid_LFFR = ""; // LFFR = LogicForceFreezeRead. Store last valid jsonstring that was read from SerialLine. Used in case we get error when deserializing new incoming jsonstring. 

    //SensorDataRead
      String jsonstringLastValid_SDR = ""; // SDR = SensorDataRead. Store last valid jsonstring that was read from SerialLine. Used in case we get error when deserializing new incoming jsonstring. 

    
  //Setup of JSON // JSON is used as our communication data interchange between layers 
  
    StaticJsonDocument<300> JsonMemory; // Estimated from https://arduinojson.org/v6/assistant/#/step3 (18.04.2023) // This will destroy and recreate the document
    StaticJsonDocument<100> JsonSerialReady; // This will destroy and recreate the document
    
    void InitJsonMemory() { //Error "'JsonMemory' does not name a type" usually occurs when you try to use a variable outside of a function scope therefore it has its own function... run at void setup()...
    //Iniziation of variables to be written to JsonMemory
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
      JsonMemory["TimeInSequence"] = TimeInSequenceJSON;
      JsonMemory["TimeRunning"] = TimeRunningJSON;
    }
        
//-------------------------------------------------------------------------------------------------------------------//

void WriteInUpdatedVariables(){ // Step 1 (figure 9. thesis document v1.0)

  // Step 1 - Write In Updated Variables updated - (see Figure 9. from thesis document v1.0)
  // Here we will update our variables found in Arduino code before StateMachine(), is called. 
  // This is similar to PLC where we update our variables from the process before running the main program.
  // We write our variables into memory allocated to the StaticJsonDocument where it will be stored.
  // Check declaration in top of code for explanation about the variables

  // Initialize JSON 
    if (i == false){
      //JsonMemory.clear(); // Clear data stored in JsonMemory 
      InitJsonMemory(); // Function that initializes the JsonMemory object with the desired values only first time WriteInUpdatedVariables() is called
      i = true;
    }

  // Read updated variables which has been updated by the SensorDataRead() (updating INPUTS before StateMachine())
    start = JsonMemory["start"];
    stop1 = JsonMemory["stop1"];
    stop2 = JsonMemory["stop2"];
    heater = JsonMemory["heater"];
    stirrer = JsonMemory["stirrer"];
    valveA = JsonMemory["valveA"];
    valveB = JsonMemory["valveB"];
    valveC = JsonMemory["valveC"];
    s1 = JsonMemory["s1"];
    s2 = JsonMemory["s2"];
    s3 = JsonMemory["s3"];
    temp = JsonMemory["temp"];
    state = JsonMemory["state"];
    counter = JsonMemory["counter"];
    flow = JsonMemory["flow"].as<String>();
    overridemode = JsonMemory["overridemode"];
    
}

//-------------------------------------------------------------------------------------------------------------------//

void StateMachine(){ // Main function for executing process logic sequence // Control Process Logic Loop (see Figure 8. in thesis document v1.0)

  switch (state) { 
//----------   
    case ready: // The step instructions 
    
      if ((start == true) && (stop2 == false)) { // Condition to change state (the transition) 
        SetTimeInSequence = millis(); // Set equal to time we "started" our sequence
        state = fill_A; // Change state to fill_A when start button is pressed
        PreviouslyVisited = false; //reset the PreviouslyVisited variable when transition is met
      }
      break; // Break out of case and move on in the StateMachine() function
      
//----------
    case fill_A: // The step instructions  

      if (PreviouslyVisited == false){ // Instructions below are only to be done the first time state has been set     
          counter++; // Increase counter for each sequence loop        
          PreviouslyVisited = true; // Set true so instruction above do not happen every main loop() we do.
      }
      
      valveA = true; // Open inlet valve A for chemical A // This instruction should be done every time we loop the main loop() function
                     // as we might need to set variable to "correct" value if override in HMI has been set and then removed.
      
      if ((s2 == false) || (stop2 == true)) { // Condition to change state (the transition) 
        state = fill_B; // Change state to fill_B when medium level indicator in the tank is reached or stop2 is true
        PreviouslyVisited = false; //reset the PreviouslyVisited variable when transition is met
      }
      break; // Break out of case and move on in the StateMachine() function
      
//----------
    case fill_B: // The step instructions 
         
      valveA = false; // Close valve A
      valveB = true; // Open inlet valve B for chemical B
      stirrer = true; // Start stirrer for mixing chemicals A and B
      heater = true; // Start heater for warming up the mixture

      if (!s3 || stop2) { // Condition to change state (the transition) 
        state = heating; // Change state to heating when high level indicator in the tank is reached or stop2 is true
        PreviouslyVisited = false; //reset the PreviouslyVisited variable when transition is met
      }
      break; // Break out of case and move on in the StateMachine() function
      
//----------
    case heating: // The step instructions

      valveB = false; //Stop filling 
     
      if (temp >= 85 || stop2) { // Condition to change state (the transition)
        state = wait; // Change state to wait when the temperature reaches a certain level or stop2 is true
        PreviouslyVisited = false; //reset the PreviouslyVisited variable when transition is met
      }
      break; // Break out of case and move on in the StateMachine() function
      
//----------
    case wait: // The step instructions 

      if (PreviouslyVisited == false){ // Instructions below are only to be done the first time state has been set
        current_time = millis(); // Save time when first entering the wait state
        PreviouslyVisited = true; // Set true so instruction above do not happen every main loop() we do.
      }
      if ((millis() - current_time) >= 30000 || stop2) { // Condition to change state (the transition)
        state = drain1; // Change state to drain1 after 30 seconds or when stop2 is true
        PreviouslyVisited = false; //reset the PreviouslyVisited variable when transition is met
      }
      break; // Break out of case and move on in the StateMachine() function
      
//----------
    case drain1: // The step instructions 

      heater = false; // Stop heater
      valveC = true; // Open outlet valve C for draining the mixture

      if (s2 || stop2) { // Condition to change state (the transition)
        state = drain2; // Change state to drain2 when tank level is below medium level indicator or stop2 is true
      }
      PreviouslyVisited = false; //reset the PreviouslyVisited variable when exiting a case        
      break; // Break out of case and move on in the StateMachine() function
      
//----------
    case drain2: // The step instructions 
    
      stirrer = false; // Stop stirrer
     
      if ((s1 || stop2) && (counter == 10 || stop1)) { // Condition to change state (the transition)
        state = ready; // Stop loop and return to ready state given conditions above are met
        PreviouslyVisited = false; //reset the PreviouslyVisited variable when transition is met
      } else if (s1 && counter < 10 && !stop1 && !stop2) {
        state = fill_A; // Restart loop/program, moving back to fill_A state given conditions above are met
        PreviouslyVisited = false; //reset the PreviouslyVisited variable when transition is met
        }
      
      break; // Break out of case and move on in the StateMachine() function
//----------
  }
    
  if (state == ready){
    TimeInSequence = 0; // Reset sequence time tracker
  } else if ((state != ready) and (state != pause)){
    TimeInSequence = millis()- SetTimeInSequence; // Update time spent in sequence (Time since sketch was uploaded - Time since start button was pressed)
    } 

  // Keep track of / Update - our time variables, see declaration for more info.
    TimeInSequenceJSON = TimeInSequence/1000; // s
    TimeRunningJSON = millis()/1000; // s
}

//----------------------------------------------------------

void WriteOutUpdatedVariables(){ // Step 1 (figure 9. thesis document v1.0)

  // Step 1 - Write Out Updated Variables updated - (see Figure 9. from thesis document v1.0)
  // Here we will update our variables found in Arduino code after control code, StateMachine(), is called. 
  // This is similar to PLC where we update variables after running main program.
  // We write our variables into memory allocated to the StaticJsonDocument where it will be stored.
  // Check declaration in top of code for explanation about the variables


  // Read updated variables which has been updated by the SensorDataRead() (updating INPUTS before StateMachine())
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
    JsonMemory["TimeInSequence"] = TimeInSequenceJSON; //Kept track on only in Arduino // Meaning is only declared/updated/changed in the  Arduino (Control Layer)
    JsonMemory["TimeRunning"] = TimeRunningJSON; //Kept track on only in Arduino // Meaning is only declared/updated/changed in the Arduino (Control Layer)
}

//----------------------------------------------------------

void SensorLogicDataWrite() { // Step 2 (figure 9. thesis document v1.0)

  // Sensor & Logic data write - Send data from Control Layer (aka here from Arduino) to the HMI Layer (aka Node-RED)
  
    JsonMemory["flow"] = "SensorLogicDataWrite"; // Identification property in our JSON data // used with figure 9. from thesis document v1.0.    
    jsonstring = ""; // clear jsonstring
    
    serializeJson(JsonMemory, jsonstring); // Function that converts JSON object to a string.
    Serial.println(jsonstring); // Function that prints text to the Serial Monitor.
    Serial.flush(); // Ensures all data in buffer is sent before continuing program execution.
    
}

//----------------------------------------------------------

void RequestLogicForceFreezeRead(boolean RequestOrderLogic) { // Allow step 4 to begin by sending request of data to the HMI Layer (aka Node-RED)
  // Request sent to HMI Layer (Node-RED) as LogicForceFreezeRead JSON
    SerialReady = RequestOrderLogic; // Ready to listen to serial line and read out "LogicForceFreezeRead". Due to size, it must be done directly and we can not "store" it in Serial Buffer (at least for UNO)...

    JsonSerialReady.clear(); // Clear data stored in JsonSerialReady
    JsonSerialReady["flow"] = "RequestLogicForceFreezeRead"; // Identification property of flow
    JsonSerialReady["SerialReady"] = SerialReady; // Signalling to Node-RED that we are ready to read serial data from it.
    
    jsonstring = ""; // clear jsonstring
    
    serializeJson(JsonSerialReady, jsonstring); // Function that converts JSON object to a string.
    Serial.println(jsonstring); // Function that prints text to the Serial Monitor.
    Serial.flush(); // Ensures all data in buffer is sent before continuing program execution.
}

//----------------------------------------------------------

void LogicForceFreezeRead() { // Step 4 (figure 9. thesis document v1.0)
  // Step 4 - Logic force & freeze read - Send data from HMI Layer to Control Layer. Here we send the exact same data as in step 2
  // - but update our variables from the user interface inputs.
   
  // Listen to serial port and read out JSON data from HMI Layer
    jsonstring = ""; // clear jsonstring
    SerialWait = millis(); // set it to current time used to track time since we began listening for data from HMI layer (Node-RED) at serial port
    
    while ((Serial.available() == 0) and (millis() - SerialWait < SerialTimeOut)) {
    // Do nothing; just wait for data
    delay(1); // Just to keep it from going bananas
    }

    delay(175); // This can be adjusted as wished but for a baud rate of 9600 I found this delay was long enough to fill up the whole serial receive buffer before reading it.
    
    while (Serial.available() > 0) {
      c = (char)Serial.read(); // Read one character from the serial buffer
      jsonstring += c;
  }

    //delay(500); // I am very paranoid regarding reading serial data in Arduino so don't be mad at me for having excessive amount of delays...
    
  if (jsonstring.length() > 0) {

    DeserializationError error = deserializeJson(JsonMemory, jsonstring); 
    
    if (error == DeserializationError::Ok){ // Used to check for errors when deserializing the jsonstring which just has been read from serial line
      jsonstringLastValid_LFFR = jsonstring; // Store the jsonstring we read out, it contain good data.
    } else {
      
      String TempStr = ("DeserializationError in LogicForceFreezeRead: "); // Local variable, declared directly...
      TempStr.concat(error.c_str()); // Appending error message to TempStr
      Serial.println(TempStr);  
      
      deserializeJson(JsonMemory, jsonstringLastValid_LFFR); // This function clear JsonMemory and fill it with last valid data we received from the HMI Layer.
      delay(10); // Waiting 10 ms before continuing program, maybe serial commuication resolve...
      }
  } else {
      Serial.println("No data recieved from HMI Layer in LogicForceFreezeRead"); // Just for easy debugging in HMI Layer (node-red)
    }
}
   
//----------------------------------------------------------

void ActuatorWrite() {
    
      JsonMemory["flow"] = "ActuatorWrite";  // Identification property // set by input to function.
      jsonstring = ""; // clear jsonstring in case something already is on it.
      
      serializeJson(JsonMemory, jsonstring); // JsonMemory set in LogicForceFreezeRead()
      Serial.println(jsonstring);
      Serial.flush(); // Ensures all data in buffer is sent before continuing program execution.
}

//----------------------------------------------------------
void RequestSensorDataRead(boolean RequestOrderSensor) { // Allow step 4 to begin by sending request of data to the HMI Layer (aka Node-RED)
  // Request sent to HMI Layer (Node-RED) as LogicForceFreezeRead JSON
    SerialReady = RequestOrderSensor; // Ready to listen to serial line and read out "LogicForceFreezeRead". Due to size, it must be done directly and we can not "store" it in Serial Buffer (at least for UNO)...
    
    JsonSerialReady.clear(); // Clear data stored in JsonSerialReady
    JsonSerialReady["flow"] = "RequestSensorDataRead"; // Identification property of flow
    JsonSerialReady["SerialReady"] = SerialReady; // Signalling to Node-RED that we are ready to read serial data from it.
    
    jsonstring = ""; // clear jsonstring
    
    serializeJson(JsonSerialReady, jsonstring); // Function that converts JSON object to a string.
    Serial.println(jsonstring); // Function that prints text to the Serial Monitor.
    Serial.flush(); // Ensures all data in buffer is sent before continuing program execution.

}
//----------------------------------------------------------

void SensorDataRead(){ // Step 8 (figure 9. thesis document v1.0)
  // Step 8 - Sensor data read - Send data from Process Layer to Control Layer (aka here). 

  // Listen to serial port and read out JSON data from Process Layer
    jsonstring = ""; // clear jsonstring
    SerialWait = millis(); // set it to current time used to track time since we began listening for data from HMI layer (Node-RED) at serial port
    
    while ((Serial.available() == 0) and (millis() - SerialWait < SerialTimeOut)) {
    // Do nothing; just wait for data
      delay(1); // Just to keep it from going bananas
    }

    delay(175); // This can be adjusted as wished but for a baud rate of 9600 I found this delay was long enough to fill up the whole serial receive buffer before reading it.
    
    while (Serial.available() > 0) {
      c = (char)Serial.read(); // Read one character from the serial buffer
      jsonstring += c;
  }
    //delay(250); // I am very paranoid regarding reading serial data in Arduino so don't be mad at me for having excessive amount of delays...

    // Check if any data was received
    if (jsonstring.length() > 0) {

    DeserializationError error = deserializeJson(JsonMemory, jsonstring); 
    
    if (error == DeserializationError::Ok){ // Used to check for errors when deserializing the jsonstring which just has been read from serial line
      jsonstringLastValid_SDR = jsonstring; // Store the jsonstring we read out, it contain good data.
    } else {
      String TempStr = ("DeserializationError in SensorDataRead: "); // Local variable, declared directly... 
      TempStr.concat(error.c_str()); // Appending error message to TempStr
      Serial.println(TempStr);
      
      deserializeJson(JsonMemory, jsonstringLastValid_SDR); // This function clear JsonMemory and fill it with last valid data we received from the Process Layer.
      delay(10); // Waiting 10 ms before continuing program, maybe serial commuication resolve...
      }
  } else {
      Serial.println("No data recieved from HMI Layer in SensorDataRead"); // Just for easy debugging in HMI Layer (node-red)
    }
}

//-------------------------------------------------------------------------------------------------------------------//

// FUNCTION SETUPS AND JSON CREATION
// Here all communication from control layer aka as here in Arduino with the HMI layer as well as the process layer.
// What we actually have added here is visualized in figure 9 in thesis document v1.0.

void loop(){
  // Call our functions
delay(25000);
    WriteInUpdatedVariables(); // Step 1 // Calling function that writes In Updated Variables updated by SensorDataRead() // READ INPUT
delay(50);    
      StateMachine(); // Control Logic // Calling main function for executing process logic sequence
delay(50);          
    WriteOutUpdatedVariables(); // Step 1 // Calling function that writes In Updated Variables updated by StateMachine() // UPDATE OUTPUT
delay(50);   
    SensorLogicDataWrite(); // Step 2 // Calling function that sends data from Control Layer (aka here from Arduino) to the HMI Layer (aka Node-RED)
delay(50);
    //Step 3 - HMI Layer which includes the user interface with override functionality - Hosted in Node-RED. //The UI modifies the data sent to Node-RED in SensorLogicDataWrite() and returns the modified data in LogicForceFreezeRead()   
delay(50);
    RequestLogicForceFreezeRead(true); // Allow step 4 (LogicForceFreezeRead();) to begin by sending request of data to the HMI Layer (aka Node-RED) // set "gate" function node to true for allowed passage
delay(50);    
      LogicForceFreezeRead(); // Step 4 // Calling function that sends data from HMI Layer to Control Layer.
delay(50);
    RequestLogicForceFreezeRead(false); // Stop step 4 from sending data without request from the HMI Layer to the Control Layer (aka Arduino) // set "gate" function node to false for blockage
delay(50);    
    ActuatorWrite(); // Step 5 + Step 6 where we read in the Json string "LogicForceFreezeRead" from HMI Layer, change flow property (ID of json data) and send it out of the Control Layer again.
delay(50);
    //Step 7 - Process Layer -> Simulation of the process which as of v1.0 is hosted in node-RED in a function node. Send in ActuatorWrite() json data and process react to it as a real-process and return sensor data in json format in the SensorDataRead() function.   
delay(50);    
    RequestSensorDataRead(true); // Allow step 8 (SensorDataRead();) to begin by sending request of data to the Process Layer (aka Node-RED for v1.0) // set "gate" function node to true for allowed passage
delay(50);    
      SensorDataRead(); // Step 8 // Return sensor value which are updated by the simulated process hosted in a function node in node-RED (as of v1.0). 
delay(50);    
    RequestSensorDataRead(false); // Stop step 8 from sending data without request from the Process Layer to the Control Layer (aka Arduino) // set "gate" function node to false for blockage
}

//-------------------------------------------------------------------------------------------------------------------//

//Setup of interrupt in our timer1 lib as well as serial commuication

void setup(){ // The setup() function is executed only once, when the Arduino board is powered on or reset
    
//Serial communication setup
  Serial.begin(38400); // //9600 baud per seconds (bits per seconds)
  delay(5000); // Wait until serial commuication is up and running before "starting" program.
}
