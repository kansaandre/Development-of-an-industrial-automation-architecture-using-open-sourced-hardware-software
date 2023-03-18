

void setup() {
  Serial.begin(9600);
  }
 
void loop() {

  enum state {
    ready = 0,
    fill_a = 1,
    fill_b = 2,
    heating = 3,
    wait = 4, 
    drain1 = 5,
    drain2 = 6
  };
  
  
  bool heater = false;
  bool s1 = true; // Normally Closed (NC)
  bool s2 = true; // NC
  bool s3 = true; // NC
  bool start = false;
  bool stirrer = false;
  bool stop1 = false;
  bool stop2 = false;
  bool valve_a = false;
  bool valve_b = false;
  bool valve_c = false;

  unsigned int counter = 0;
  float temp = 50.0; //[C] Set to a value within the process area 
  unsigned long int elapsed_millis = 0; //[ms] time logged at case wait  
  state current_state = ready;
  //-------------------------------------

  switch (current_state) {

    case ready: 
      Serial.print("Current state: ");
      Serial.println(current_state);
      counter = 0; // Reset counter
      if(start == true) {
        current_state = fill_a; // Change state name
      }
      break;  // Jump out of ready case

    case fill_a:
      Serial.print("Current state: ");
      Serial.println(current_state);
      counter = counter + 1; // Counting loop cycles
      valve_a = true; // Open valve A letting in substance A 
      if(s2 == false or stop2 == true) { // When tank reach s2 then do...
        current_state = fill_b; // Change state name
       }
      break; // Jump out of fill_a case

    case fill_b:
      Serial.print("Current state: ");
      Serial.println(current_state);
      valve_a = false; // Close valve A from letting in substance A
      valve_b = true; // Open valve B letting in substance B
      heater = true; // Start heating the substances in the tank
      stirrer = true; // Start stirring the tank substances
      if(s3 == false or stop2 == true) { // When tank reach s3 then do...
        current_state = heating; // Change state name
      }
      break; // Jump out of fill_b case
      
    case heating:
      Serial.print("Current state: ");
      Serial.println(current_state);
      valve_b = false; // Close valve B from letting in substance B
      if(temp > 85 or stop2 == true) {
        current_state = wait; // Change state name
      }
      break; // Jump out of heating case

    case wait:
      Serial.print("Current state: ");
      Serial.println(current_state);
      elapsed_millis = millis(); // Log time elapsed as soon as case is activated
      if((millis()-elapsed_millis) > 30000 or stop2 == true) { // Wait until 30 s has elapsed then do...
        current_state = drain1; // Change state name
      }
      break; // Jump out of wait case

    case drain1:
      Serial.print("Current state: ");
      Serial.println(current_state);
      heater = false; // Stop heating of tank
      valve_c = true; // Open valve C letting out mixture of A and B from tank
      if(s2 == true or stop2 == true) { // When tank level fall below s2 then do...
        current_state = drain2; // Change state name
      }
      break; // Jump out of drain1 case

    case drain2:
      Serial.print("Current state: ");
      Serial.println(current_state);
      stirrer = false; // Stop stirring the tank substances
      if((s1 == true) and (counter < 10) and (stop1 == false and stop2 == false)) {
        current_state = fill_a; // must complete 10 cycles or be stopped to go back to ready state
      }
      else {
        current_state = ready; // if stop or 10 cycles have been complete jump to readystate
      }
      break; // Jump out of drain2 case
      
  
      

    
   
       
  }
}
