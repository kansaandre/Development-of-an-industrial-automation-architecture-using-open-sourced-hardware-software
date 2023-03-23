//23.03.2023 20:17
//Control Layer of "Development of an industrial automation architecture" --> GITHUB https://bit.ly/3TAT78J
  //NOTE! In code a lot of referencing to thesis document is done to clearify/document code
  //this currently is referencing to thesis version ------->  version. 1.0 = v.1.0  <---------- , 
  //remember to UPDATE with NEW versions/iterations of the thesis document. Document version seen in front page.

  //Short description about the project:
    //This open-source, community-driven project aims to develop an industrial automation architecture. 
    //Its vast resources, including source code, forums, and documentation, make it easier to integrate 
    //AI tools like ChatGPT, aiding coding, architecture design, and hardware selection. 
    //This sets it apart from confidential industrial vendors.

  #include <ArduinoJson.h>
  #include <TimerOne.h> //NOTE! READ SECTION BELOW:
    //The TimerOne library is an Arduino library specifically designed for AVR-based microcontrollers, 
    //such as the ATmega328P found in the Arduino Uno. Here's an overview of the TimerOne library and its key features:
      //Hardware usage: The library utilizes Timer1, a 16-bit hardware timer present in many AVR microcontrollers. 
        //Timer1 is one of the most versatile timers in these microcontrollers, offering a wide range of functionality 
        //and high-resolution timing.
      //PWM generation: TimerOne allows you to generate PWM signals with configurable frequency and duty cycle on specific pins, 
        //typically pin 9 and pin 10 on the Arduino Uno.
      //Interrupt handling: The library enables you to attach and detach interrupt service routines (ISR) to the timer overflow event. 
        //This means you can execute specific functions at defined time intervals, which is useful for tasks like periodic data sampling or time-based control.
      //Timer configuration: TimerOne provides functions to configure the timer's prescaler, allowing you to adjust the timer's resolution and range. 
        //You can also set the desired timer period directly in microseconds, making it easy to achieve precise timing intervals.
      //Easy-to-use interface: The TimerOne library offers a simple and intuitive API, making it easy to work with the Timer1 hardware. 
        //The API includes functions like initialize(), setPeriod(), start(), stop(), resume(), and attachInterrupt().
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      //Keep in mind that the TimerOne library is designed for AVR-based microcontrollers, and it may not work with other architectures like ARM or ESP. 
      //If you're working with a different microcontroller, you may need to find a library that's compatible with your specific hardware or use the built-in timer functionalities of that microcontroller.
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      
  void setup(){ // The setup() function is executed only once, when the Arduino board is powered on or reset.
    //Timer setup
      Timer1.initialize(5000000); // Set interrupt interval function call to 1 second (1000000 microseconds)

      Timer1.attachInterrupt(RWIO); // Attach the IO function to the interrupt
    //Serial communication setup
      Serial.begin(9600);
        }
  
  // Global variable decleration with their normal default value
    
    // Buttons (input)
      boolean start = false; // Start button to start sequence
      boolean stop1 = false; // Stop button (NORMAL) will stop sequence at state "ready"
      boolean stop2 = false; // Stop button (EMERGENCY) will stop sequence immediately and return to "ready'
    
    // Actuators (output)
      boolean heater = false; // Heater for heating up tank
      boolean stirrer = false; // Stirred for stirring up tank
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
      uint16_t current_time = 0; //ms Used to save time from millis() function
      // JSON
      String JSONSTRING; // Declare a JSON string to be able to commuicate JSON data out from the Control Layer to HMI Layer
      uint8_t LFFR_ReadFailCount = 0; // "Logic force & freeze readings" failure to read counter. At =3 JSONOBJ_LastValid is overwritten and freeze/forced values are replaced with raw sensor values.
      boolean Flag_LogicForceFreezeReadings_Error = false; // Flag is set if Control Layer is unable to read in "Logic force & freeze readings" 
    
    // States (program variables)
      enum states { //Declare our states made direectly from Figure 9. in thesis document. 
        ready = 1,
        fill_A = 2,
        fill_B = 3,
        heating = 4,
        wait = 5,
        drain1 = 6,
        drain2 = 7
      };
//-------------------------------------------------------------------------------------------------------------------//
  //READ WRITE INPUT OUTPUT INTO JSON OBJECT + JSON SETUP // Functions used for RWIO (Interrupt loop)
    StaticJsonDocument<300> JSONBUFFER1; // JSON buffer This is a class provided by the ArduinoJson library to create a JSON buffer. A buffer is a memory area that will store the JSON data. <bytes data>
    StaticJsonDocument<300> JSONBUFFER2;
    JsonObject JSONOBJ = JSONBUFFER1.to<JsonObject>(); // Convert to JsonObject to store key-value pairs because it makes it easy to access and modify the individual values using the corresponding keys.
    JsonObject JSONOBJ_LastValid = JSONBUFFER2.to<JsonObject>(); // Documentatation found in LogicForceFreezeReadings() function.
                         
    void SensorDataReadings(){ //"Sensor data readings" Process Layer to Control Layer, see figure 3 & 10 in thesis document.
      // start = digitalRead()
      // stop1 = digitalRead()
      //Etc. etc. Example how to read data from input ports when real sensors are connected to controller
    }

    void JsonObjPropertyAdd(){ // Add our variables to the JSON document/buffer
      JSONOBJ["start"] = start; 
      JSONOBJ["stop1"] = stop1;
      JSONOBJ["stop2"] = stop2;
      JSONOBJ["heater"] = heater;
      JSONOBJ["stirrer"] = stirrer;
      JSONOBJ["valveA"] = valveA;
      JSONOBJ["valveB"] = valveB;
      JSONOBJ["valveC"] = valveC;
      JSONOBJ["s1"] = s1;
      JSONOBJ["s2"] = s2;
      JSONOBJ["s3"] = s3;
      JSONOBJ["temp"] = temp;
      JSONOBJ["counter"] = counter;
      
    }
    
    void SensorLogicDataWritings(){ //"Sensor & Logic data writing" Control Layer to HMI Layer, see figure 3 & 10 in thesis document (v.1)
      
      serializeJson(JSONBUFFER1, JSONSTRING); // Function to convert data to JSON format string.
      JSONBUFFER1.clear(); // clear buffer
      Serial.println(JSONSTRING); // Print JSON string to serial monitor with Serial.println
    }
    
    void LogicForceFreezeReadings() { //"Logic force & freeze readings" HMI Layer to Control Layer, see figure 3 & 10 in thesis document (v.1)
      DeserializationError error = deserializeJson(JSONBUFFER1, JSONSTRING); // Error msg https://arduinojson.org/v6/api/misc/deserializationerror/
      
      if (error != DeserializationError::Ok){ 
        JSONOBJ["DebugReadLogicForceFreeze"] = error.c_str(); // Convert error to string, will be sent to HMI Layer in next interrupt call
        Flag_LogicForceFreezeReadings_Error = true; // Set a flag true = BAD read of Logic force & freeze reading (fig 10. thesis)
        LFFR_ReadFailCount += 1; // Add one

        start = JSONOBJ_LastValid["start"];
        stop1 = JSONOBJ_LastValid["stop1"];
        stop2 = JSONOBJ_LastValid["stop2"];
        heater = JSONOBJ_LastValid["heater"];
        stirrer = JSONOBJ_LastValid["stirrer"];
        valveA = JSONOBJ_LastValid["valveA"];
        valveB = JSONOBJ_LastValid["valveB"];
        valveC = JSONOBJ_LastValid["valveC"];
        s1 = JSONOBJ_LastValid["s1"];
        s2 = JSONOBJ_LastValid["s2"];
        s3 = JSONOBJ_LastValid["s3"];
        temp = JSONOBJ_LastValid["temp"];
        counter = JSONOBJ_LastValid["counter"]
        ;
      }
      else {
        start = JSONOBJ["start"];
        stop1 = JSONOBJ["stop1"];
        stop2 = JSONOBJ["stop2"];
        heater = JSONOBJ["heater"];
        stirrer = JSONOBJ["stirrer"];
        valveA = JSONOBJ["valveA"];
        valveB = JSONOBJ["valveB"];
        valveC = JSONOBJ["valveC"];
        s1 = JSONOBJ["s1"];
        s2 = JSONOBJ["s2"];
        s3 = JSONOBJ["s3"];
        temp = JSONOBJ["temp"];
        counter = JSONOBJ["counter"];
        
        Flag_LogicForceFreezeReadings_Error = false; // set flag false = GOOD read of Logic force & freeze reading (fig 10. thesis)
        
        LFFR_ReadFailCount = 0; // Reset error counter, we received good data from HMI.
        
        JSONOBJ_LastValid = JSONOBJ;  // Here we store last valid HMI read in case of commuication error reading from HMI Layer "Logic force & freeze readings", see figure 10. 
                                      // We will use this temporary storage of last valid read from the HMI Layer to keep values used in the "Control Process Logic Loop" frozen
                                      // for MAXIMUM 3 interrupt function cycle calls. This will give the architecture time to be able to read "Logic force & freeze readings" correctly 
                                      // and updating the values for the main program instead of insantly use Sensor values directly which effectively overwrite possible forced/freezed 
                                      // values set by the operators which can trip the plant depending on what really is frozen and why. If after 3 interrupt function calls the values
                                      // received is still not able to be read in by function LogicForceFreezeReadings() we will let the sensors overwrite the JSON Object letting values
                                      // be used in the main control program and the Control Layer running the plant "blind" solely on Sensor input and Logic until HMI Layer commuication 
                                      // can be achieved again. 

      }
      JSONOBJ["Flag_LogicForceFreezeReadings_Error"] = Flag_LogicForceFreezeReadings_Error; // Add it to JSON, will be sent to HMI Layer in next interrupt function call.
      JSONOBJ["LFFR_ReadFailCount"] = LFFR_ReadFailCount; // Add it to JSON, will be sent to HMI Layer in next interrupt function call.
      JSONOBJ_LastValid["Flag_LogicForceFreezeReadings_Error"] = Flag_LogicForceFreezeReadings_Error; //Copy
    }
      
  //Read/Write Time-Interrupt Loop
    void RWIO (){ 
      
      SensorDataReadings(); 
       
      JsonObjPropertyAdd();        

      SensorLogicDataWritings(); 

      LogicForceFreezeReadings();
      
    }


  //Control Process Logic Loop (see Figure 10. in thesis document)
  
    states state = ready; // Default/inital state   
  
    void loop(){

      switch (state) {
        
        case ready:
          if (start == true) {
            state = fill_A; // If start button is pressed change state to fill_A
             }
           break; // Break is neccesary to exit the switch statement

        case fill_A:
          counter = counter + 1; // Increase counter with 1 --> Start of sequence loop
          valveA = true; // Open inlet valve A letting in chemical A to tank
          if ((s2 == false) or (stop2 == true)) {
            state = fill_B; // If medium level indicator in tank is reached change state to fill_B
          }
          break; // Jump out of fill_A

        case fill_B:
          valveA = false; // Close valve A
          valveB = true; // Open inlet valve B letting in chemical B to tank
          stirrer = true; // Start stirrer to tank - mixing chemical A and B inside tank
          heater = true; // Start heater to tank, warming up the mixture inside tank
          if ((s3 == false) or (stop2 == true)) {
            state = heating; // If high level indicator in tank is reached change state to heating
          }
          break; // Jump out of fill_B
          
        case heating:
          if ((temp >= 85) or (stop2 == true)) {   
            state = wait; // Change state to wait when temp has reached a certain level inside tank
          }
          break; // Jump out of heating

        case wait:
          current_time = millis(); // Time saved when first entering wait state
          if (((millis()-current_time) >= 30000) or (stop2 == true)) { //millis() = millisecond [ms] since arduino board began running program
            state = drain1; // After 30 s we move over to drain1 state
          }
          break; // Jump out of wait

        case drain1:
          heater = false; // Stop heater
          valveC = true; // Open outlet valve C letting out tank mixture
          if ((s2 == true) or (stop2 == true)) { //If tank level reach below medium level indicator (s2) then move to next state
            state = drain2; // Move to next state when tank has reached below medium level
          }
          break; // Jump out of drain1

        case drain2:
          stirrer = false; // Stop stirrer
          if (((s1 == true) or (stop2 == true)) and ((counter == 10) or stop1 == true)) {
            state = ready; // Stop loop and return to ready state given conditions above are met
          }
          else if (((s1 == true) and (counter < 10)) and ((stop1 == false) and (stop2 == false))){
            state = fill_A; // Restart loop/program moving back to fill_A state given conditions above are met
          }
       }
    }

          
