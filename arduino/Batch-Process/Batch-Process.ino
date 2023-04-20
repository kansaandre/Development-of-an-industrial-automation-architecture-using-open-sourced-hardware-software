//LAST UPDATE (roughly): 20.04.2023 03:12
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
    //such as the ATmega328P found in the Arduino Uno. Here's an overview of the TimerOne library and its key features:w
      //Hardware usage: The library utilizes Timer1, a 16-bit hardware timer present in many AVR microcontrollers. 
        //Timer1 is one of the most versatile timers in these microcontrollers, offering a wide range of functionality 
        //and high-resolution timing. 
      //Precision: Hardware timers are more precise and have better resolution than software timers. They are based on the microcontroller's internal hardware clock, 
        //so their accuracy is not affected by other processes or code execution.
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

//-------------------------------------------------------------------------------------------------------------------//
      
  // Global variable decleration with their normal default value

    //JSON properties sent between layers
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
        String flow = ""; // Variable used to identify string when sent on serial line in JSON format
        boolean overridemode = false; // Set in the ui when operators/engineers override values manually
        String error = ""; //Error message that can be used to detect error in commuication when transfering to another layer
  
        //JSON declaration direct
        String jsonstring = ""; //Used to send/recieve JSON as string between layers
        boolean SerialReady = false; //Used for handshake agreement when HMI Layer are to send data to Control Layer.
                                     //Needed to make sure we are listening on serial port before data the large JSON
                                     //data string is sent. Can be removed with larger serial reciever buffer....
   
      // States (program variables)
        enum states { //Declare our states made direectly from Figure 9. in thesis document.
          pause = 0, 
          ready = 1,
          fill_A = 2,
          fill_B = 3,
          heating = 4,
          wait = 5,
          drain1 = 6,
          drain2 = 7
        };
        states state = ready; // Default/inital state

      //Properties only found in the control layer
         unsigned long current_time;
         char c;
         unsigned long timeout = 1000; // Timeout duration in milliseconds
         unsigned long startTime = millis(); // Used for serial commuication see step 4.
         unsigned long TimeRunning = millis()/1000; // Used to remember since program was uploaded and ran as well as tracking time spent in sequence
         unsigned long TimeInSequence;

//-------------------------------------------------------------------------------------------------------------------//
  
  //Control Process Logic Loop (see Figure 10. in thesis document)
   
    void loop(){ //Main loop for executing process logic sequence

      switch (state) {
        
        case ready:
          TimeInSequence = 0; // reset sequence time tracker
          if (start == true) {
            TimeInSequence = millis()/1000; // Start counting time since we entered sequence
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

//-------------------------------------------------------------------------------------------------------------------//

  //FUNCTION SETUPS AND JSON CREATION 
    //Here all commuication from control layer aka as here in Arduino with the HMI layer as well as the process layer.
    //What we actually have added here is visualized in figure 9 in thesis document v1.0.


    void interrupt(){
      
      TimeInSequence = TimeRunning - TimeInSequence; // Tracking time in sequence meaning (state != ready)
        
      //Setup of JSON // JSON is used as our commuication data interchange between layers // destroyed everytime as recommended by documentation of ArduinoJson.h
      StaticJsonDocument<700> JsonMemory; //Estimated from https://arduinojson.org/v6/assistant/#/step3 (18.04.2023)
      StaticJsonDocument<100> JsonSerialReady;  

      //Step 1 - Read In Updated Variables - (see Figure 9. from thesis document v1.0)
      //Here we will update our variables which may have been given new values from the control logic code above.
      //We write our variables into memory allocated to the StaticJsonDocument where it will be stored ready for transmission.
        Serial.println("step1");
        //Check declaration in top of code for explanation about the variables
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
        JsonMemory["TimeRunning"] = TimeRunning;
        JsonMemory["TimeInSequence"] = TimeInSequence;

      //Step 2 - Sensor & Logic data write - Send data from Control Layer (aka here from Arduino) to the HMI Layer (aka Node-RED)
       Serial.println("step2");
        flow = "SensorLogicDataWrite"; //Identification property in our JSON data // used with figure 9. from thesis document v1.0.
        JsonMemory["flow"] = flow;
        
        jsonstring = ""; // clear jsonstring 
        
        serializeJson(JsonMemory, jsonstring); //Function that converts JSON object to a string.
        Serial.println(jsonstring); //Function that prints text to the Serial Monitor.
        
    
      //Step 3 - HMI Layer which include the user interface with override functionality - Hosted in Node-RED
       Serial.println("step3");
      //Step 4 - Logic force & freeze read - Send data from HMI Layer to Control Layer. Here we sending exact same data as in step 2
            // - but update our variables from the user interface inputs, overwriting data etc.
       Serial.println("step4");
        //Request LogicForceFreezeRead JSON
          flow = "RequestLogicForceFreezeRead";
          SerialReady = true; //Ready to listen to serial line and read out "LogicForceFreezeRead". Due to size, it must be done directly and we can not "store" it in Serial Buffer (at least for UNO)...
          
          JsonSerialReady["flow"] = flow; //Identification property
          JsonSerialReady["SerialReady"] = SerialReady; //Ready to read serial data sent from HMI Layer (aka Node-RED)
          
          jsonstring = ""; // clear jsonstring
  
          serializeJson(JsonSerialReady, jsonstring);
          Serial.println(jsonstring);
          
        // Listen to serial port and read out JSON data from HMI Layer
          jsonstring = ""; // clear jsonstring
        
        // Wait for data or until timeout expires
          Serial.setTimeout(4000000)
        
        // Read data from the serial buffer
          while (Serial.available() > 0) {
            c = (char)Serial.read(); // Read one character from the serial buffer
            jsonstring += c;
          }
        
        // Check if any data was received
          if (jsonstring.length() > 0) {
            // Store serial data string in JSON memory, effectively making it into a JSON object
            deserializeJson(JsonMemory, jsonstring);

            //Test - See what data has been read from HMI Layer and added to our JsonMemory (made to object).        
              flow = "test"; //Identification property
              JsonMemory["flow"] = flow;
      
              jsonstring = ""; // clear jsonstring
      
              serializeJson(JsonMemory, jsonstring);
              Serial.println(jsonstring);            
                
          } else {
            // Handle the case when no data was received or timeout occurred
            // You can add error handling or logging here
          }

        //Close gate stopping serial data from HMI layer to Control Layer 
          flow = "RequestLogicForceFreezeRead";
          SerialReady = false; //Close gate
          JsonSerialReady["flow"] = flow; //Identification property
          JsonSerialReady["SerialReady"] = SerialReady; //Ready to read serial data sent from HMI Layer (aka Node-RED)

          jsonstring = ""; // clear jsonstring
          
          serializeJson(JsonSerialReady, jsonstring); //Send stop signal to HMI Layer
          Serial.println(jsonstring); //Send stop signal to HMI Layer

        
      
    }
  
              
//-------------------------------------------------------------------------------------------------------------------//

  //Setup of interrupt in our timer1 lib as well as serial commuication
    
    void setup(){ // The setup() function is executed only once, when the Arduino board is powered on or reset

      //Timer setup
        Timer1.initialize(20000000); // Set interrupt interval function call to 1 second (1000000 microseconds)
        Timer1.attachInterrupt(interrupt); // Attach the ReadWriteInOutInterrupt() function to the interrupt
        
      //Serial communication setup
        Serial.begin(9600); // //9600 baud per seconds (bits per seconds)

        delay(5000);
    }

    

          
