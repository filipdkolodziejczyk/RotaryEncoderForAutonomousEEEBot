//================================================================================================//
// HOW THE ENCODER & DEBOUNCE SYSTEM WORKS STEP-BY-STEP
//
// 1. PHYSICAL ENCODER SIGNAL MAPPING:
// As the motor drives forward, the channels cycle through sequential states. By shifting Channel A 
// left by 1 bit and combining it with Channel B, i get a clean decimal representation:
//
//   States:               S0          S1          S2          S3
//   Raw Channels:        a=0 b=0 --> a=1 b=0 --> a=1 b=1 --> a=0 b=1 
//   Bitwise Decimal:       0           2           3           1
// 
// Note: Due to physical hardware alignment (seen on the oscilloscope), state S3 (decimal 1) 
// is natively skipped during forward motion. The sequence travels S0 -> S1 -> S2 -> S0. 
// S3 is retained in the matrix table solely for mathematical completeness.
//
// 2. THE STARTUP PHASE (setup()):
// At boot, the code takes a single snapshot of the pins to see where the wheel is sitting. This 
// sets 'initialState' and seeds our tracking history ('previousStateNum'). This initialization
// enum is a dead variable after setup() finishes and is not used in the running loop logic.
//
// 3. THE DIGITAL DEBOUNCE FILTER (loop()):
// The ESP32 processes the main loop millions of times per second. Because mechanical DC motor 
// brushes create massive high-frequency electrical arcing, the raw pins flicker rapidly. The ESP32 
// catches this noise, creating catastrophic distance calculation tracking errors.
//
// To destroy this noise, i enforced a consecutive confirmation rule:
// - The code stores our last officially accepted position in 'stableStateNum'.
// - Every loop, it calculates the live 'currentStateNum' via bitwise math.
// - If the wheel moves or noise hits, 'currentStateNum != stableStateNum'. This triggers the filter.
// - The code refuses to look at the matrix table until 'currentStateNum' holds its new value 
//   perfectly still for 6 consecutive loop samples ('debounceThreshold').
// - If noise flickers and vanishes before hitting 6 counts, the counter flushes to 0 and the 
//   glitch is completely erased.
//
// 4. THE ACCEPTANCE & RESET PHASE:
// Only when the threshold of 6 stable samples is reached does the gate open. The code passes the 
// verified transition to 'encoderTable' to safely update 'pulseCount'. Finally, the memory lines 
// are updated to form our new baseline:
//      stableStateNum   = currentStateNum;
//      previousStateNum = currentStateNum;
//================================================================================================//



#define enA 33  // Enable A command line
#define enB 25  // Enable B command line

#define INa 26  // Channel A Direction
#define INb 27  // Channel A Direction
#define INc 14  // Channel B Direction
#define INd 12  // Channel B Direction

#include <math.h>

const int ledChannela = 0;
const int ledChannelb = 1;

// GPIO pins used for left motor
const int leftmotorChannelaPin = 36;
const int leftmotorChannelbPin = 39;
//GPIO pins for right motor
const int rightmotorChannelaPin = 34;
const int rightmotorChannelbPin = 35;

int leftSignalA = 0;
int leftSignalB = 0;
int rightSignalA = 0;
int rightSignalB = 0;

int leftPulseCount = 0;
int rightPulseCount = 0;
//const float singlePulseDist = ( M_PI * 3.32) / 36;       good for just going forward

//========Servo Settings====================================================================//
// the way it works is that the servo receives a signal from the microcontroller every 20ms //
// which is the 50Hz,the servo measures teh total time that the signal is ON for aka pulse  //
// width. There is a contol circuit in the motor which translates a 1ms high signal to an   //
// angle then the rest of the signal is low untill the next pulse arrives after 20ms.       //
// microcontrollers can output a 1.5V it uses PWM to do this mathematically. microcontroller//
// -s use high and low signals only.                                                        //
//                                                                                          //
// The microcontroller gets the 20ms and slices it into 12 bit resolution which is 4096 ticks//
// 4096/20 gets you 204.8 ticks per 1ms so, for a lets say 90 degree angle i need 205 ticks //
// so i need a high signal for 205 ticks then low for the rest. the pwm for the servos is used//
// as a communication line rather than what the motor uses it for an average voltage        //
//==========================================================================================//

// PWM Channel Properties
const int servoFrequency = 50;    //50Hz
const int servoChannel = 2;
const int servoResolution = 12;   //12bit resolution 0 - 4095
// Hardware Pin Configuration
int servoPin = 13;
float steeringAngle=93;  // variable to store the servo position



  

// State mapping how encoder sequence works decoding the way its designed
enum States {S0 = 0, S1 = 1, S2 = 2, S3 = 3};
States leftInitialState;
States rightInitialState;

const int encoderTable[4][4]={
    /*New states      00,  01, 10, 11*/
    /*old state 00*/{  0,  0,  1,   -1},
    /*old state 01*/{  0,  0,  0,   0 },
    /*old state 10*/{ -1,  0,  0,   1 },
    /*old state 11*/{  1,  0, -1,   0 }       
};                

//Debounce filter 
int leftStableStateNum = 0;
int leftDebounceCounter = 0;
int rightStableStateNum = 0;
int rightDebounceCounter = 0;

const int debounceThreshold = 6;     // i need a consecutive 6 of the same state i.e 11 11 11 11 11 11 to use the matix table

int leftCurrentStateNum;
int leftPreviousStateNum;
int rightCurrentStateNum;
int rightPreviousStateNum;

//----------------------------------------------------------------------------------------------------------------------------------------------------------//
//----------------------------------------Setup function things that only need to be done once--------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------//

void setup() {
  // Configuring encoder pins as inputs with pullups active
  pinMode(leftmotorChannelaPin, INPUT_PULLUP);
  pinMode(leftmotorChannelbPin, INPUT_PULLUP);
  pinMode(rightmotorChannelaPin, INPUT_PULLUP);
  pinMode(rightmotorChannelbPin, INPUT_PULLUP);
  
  
  // Configure motor direction pins as outputs
  pinMode(INa, OUTPUT);
  pinMode(INb, OUTPUT);
  pinMode(INc, OUTPUT);
  pinMode(INd, OUTPUT);

  // Attach PWM channels to pins
  ledcAttachPin(enA, ledChannela);
  ledcAttachPin(enB, ledChannelb);
  ledcAttachPin(servoPin, servoChannel);
  

  // Initialize PWM channels (1000 Hz PWM, 8-bit resolution)
  ledcSetup(ledChannela, 200, 8); 
  ledcSetup(ledChannelb, 200, 8); 
  ledcSetup(servoChannel, servoFrequency, servoResolution); //servo setup on PWM2, 50Hz, 12-bit (0-4096)

  // Read initial startup position of encoder
  leftSignalA = digitalRead(leftmotorChannelaPin);
  leftSignalB = digitalRead(leftmotorChannelbPin);
  rightSignalA = digitalRead(rightmotorChannelaPin);
  rightSignalB = digitalRead(rightmotorChannelbPin);
  //----------------------------------------------------------------------------------------------------------------------------------------------------------//
  //----------------------------------------------------------------------------------------------------------------------------------------------------------//
  //----------------------------------------------------------------------------------------------------------------------------------------------------------//
  //----------------------------------------------------------------------------------------------------------------------------------------------------------//


  //--------------------------------------------------------------------------------------------------------------//
  //-------this is linking the initial states defined above to the matrix previous and current functionality------//
  //--------------------------------------------------------------------------------------------------------------//
  //--------------------------------------------------------------------------------------------------------------//
  //--------------------------------------------------------------------------------------------------------------//
  if(leftSignalA == 0 && leftSignalB == 0)      { leftInitialState = S0; leftPreviousStateNum = 0; }
  else if(leftSignalA == 1 && leftSignalB == 0) { leftInitialState = S1; leftPreviousStateNum = 2; }         
  else if(leftSignalA == 1 && leftSignalB == 1) { leftInitialState = S2; leftPreviousStateNum = 3; }         
  else                                          { leftInitialState = S3; leftPreviousStateNum = 1; }          

  if(rightSignalA == 0 && rightSignalB == 0)      { rightInitialState = S0; rightPreviousStateNum = 0; }
  else if(rightSignalA == 1 && rightSignalB == 0) { rightInitialState = S1; rightPreviousStateNum = 2; }         
  else if(rightSignalA == 1 && rightSignalB == 1) { rightInitialState = S2; rightPreviousStateNum = 3; }         
  else                                            { rightInitialState = S3; rightPreviousStateNum = 1; }  
  //initialises what the state is at the start for the debounce filter
  leftStableStateNum = leftPreviousStateNum;
  rightStableStateNum = rightPreviousStateNum;
  // Begin serial communication
  Serial.begin(115200);
  Serial.println("ESP32 Running..."); 
  //Serial.print("Target Pulses to reach: ");
  //Serial.println(NumOfPulses);
}
//--------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------//


//--------------------------------------------------------------------------------------//
//------------------------------Main loop execution-------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//

void loop() {
  //  Read pins instantly
  leftSignalA = digitalRead(leftmotorChannelaPin);
  leftSignalB = digitalRead(leftmotorChannelbPin);
  rightSignalA = digitalRead(rightmotorChannelaPin);
  rightSignalB = digitalRead(rightmotorChannelbPin);

  //  Map pins to matrix state index
  leftCurrentStateNum = (leftSignalA << 1) | leftSignalB;             
  rightCurrentStateNum = (rightSignalA << 1) | rightSignalB; 
  //  Process the encoder changes
  LeftEncoderPulseCount(leftSignalA, leftSignalB);
  RightEncoderPulseCount(rightSignalA, rightSignalB);

  //  Evaluate if target is reached or sequence reached
  //DistanceCalculator(rightPulseCount);

  Navigation();
}
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------//
//------------------------------Motor Controls------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//

void goForwards() {
  // Set motor direction for forwards movement
  digitalWrite(INa, LOW);
  digitalWrite(INb, HIGH);
  digitalWrite(INc, HIGH);
  digitalWrite(INd, LOW);

  // Drive at speed 90
  ledcWrite(ledChannela, 90); 
  ledcWrite(ledChannelb, 90); 
}

void Stop() {
  // 
  digitalWrite(INa, HIGH);
  digitalWrite(INb, HIGH);
  digitalWrite(INc, HIGH);
  digitalWrite(INd, HIGH);

  // Kill power completely
  ledcWrite(ledChannela, 0); 
  ledcWrite(ledChannelb, 0); 
}
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------//
//------------Reading and filtering the encoders using a debounce filter----------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//

int LeftEncoderPulseCount(int leftSignaA, int leftSignalB){
  
  // If the pins match our accepted stable state, reset the counter
  if (leftCurrentStateNum == leftStableStateNum) {
    leftDebounceCounter = 0;
  } 
  // If the pins changed, start counting how long they stay at this new value
  else {
    leftDebounceCounter++;
    
    //  Only accept the state change if it clears the threshold without flickering
    if (leftDebounceCounter >= debounceThreshold) {
      int leftMatrixValue = encoderTable[leftStableStateNum][leftCurrentStateNum];
      leftPulseCount += leftMatrixValue;
      
      //Serial.print("LEFT Live Count: ");
      //Serial.println(leftPulseCount);
      
      // Update our memories to the new confirmed state
      leftStableStateNum = leftCurrentStateNum;
      leftPreviousStateNum = leftCurrentStateNum;
      leftDebounceCounter = 0;
    }
  }
  return leftPulseCount;
}

int RightEncoderPulseCount(int rightSignaA, int rightSignalB){
  
  // If the pins match our accepted stable state, reset the counter
  if (rightCurrentStateNum == rightStableStateNum) {
    rightDebounceCounter = 0;
  } 
  // If the pins changed, start counting how long they stay at this new value
  else {
    rightDebounceCounter++;
    
    //  Only accept the state change if it clears the threshold without flickering
    if (rightDebounceCounter >= debounceThreshold) {
      int rightMatrixValue = encoderTable[rightStableStateNum][rightCurrentStateNum];
      rightPulseCount += rightMatrixValue;
      
      //Serial.print("RIGHT Live Count: ");
      //Serial.println(rightPulseCount);
      
      // Update our memories to the new confirmed state
      rightStableStateNum = rightCurrentStateNum;
      rightPreviousStateNum = rightCurrentStateNum;
      rightDebounceCounter = 0;
    }
  }
  return rightPulseCount;
}
//--------------------------------------------------------------------------------------//
//-------------------------SERVO   -----------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//


void moveServoToAngle(int degrees) {
  // Map 0-180 degrees to the strict 1ms-2ms operational limits (205 to 410 ticks)
  int pwmValue = map(degrees, 0, 180, 102, 512);

  //Serial.print("Target Angle: ");
  //Serial.print(degrees);
  //Serial.print("°  --> Translated to 12-bit PWM Duty Value: ");
  //Serial.println(pwmValue);

  // Send the clean, verified signal directly to the hardware
  ledcWrite(servoChannel, pwmValue);
}
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------//
//---------------------------------------Navigation code--------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//

enum moveType {
  stop = 0, forward = 1, left_turn = 2, right_turn = 3, right_tight = 4, left_tight = 5
};

struct NavScript{
    moveType actions;
    int pulseTarget;
};

// route array of data structures of NavScript

NavScript route[] = {
  
  //===figure of 8====//
  {forward, 195},       //index 0
  {right_turn, 125},     //index 1 
  {left_turn, 740},        //index 2
  {forward, 220},     //index 3 inside of the route array 
  {right_turn, 780},
  {forward, 200},

  {stop, 0}
 //==============================//
 //====tightest turning circle==//
 
 /*
 {forward, 145},
 {right_tight,600}
 */
 
 
};

//need pointers to scroll through this array which is currentRouteIndex and the totalSteps function is needed so if i add or delete steps is adjusts it here
// the total steps is used in an if statement to check if im doing the route of i ahve finished it 
int currentRouteIndex =0;
int totalSteps =sizeof(route)/sizeof(route[0]);

void Navigation(){
  if(currentRouteIndex>=totalSteps){
    Stop();
  }
  NavScript currentTarget = route[currentRouteIndex];
  //code for when the target is reached i reset either encoder and i increase the current route index to +1 to the next step in the sequence 
  if(leftPulseCount > currentTarget.pulseTarget || rightPulseCount > currentTarget.pulseTarget){
    currentRouteIndex++;
    leftPulseCount = 0;
    rightPulseCount =0;
  }

  if(currentTarget.actions == forward){
    moveServoToAngle(93);
    goForwards();
  }
  else if(currentTarget.actions == left_turn){
    //servo angle and motor speed adjustment 
  moveServoToAngle(65);
  digitalWrite(INa, LOW);
  digitalWrite(INb, HIGH);
  digitalWrite(INc, HIGH);
  digitalWrite(INd, LOW);
    // Drive at speed diff speeds
  ledcWrite(ledChannela, 75);      //left motor  
  ledcWrite(ledChannelb, 100);    // right motor
                                  // IMPORTANT TO UNDERSTANDING so this is turning left and the channels are set to go forward but these channels are set in such a way that 
                                  // the actual signal is a channelb = low and at a frequency of 200Hz therefore, a single pulse is 5ms and becuase it is set to a 8 bit
                                  // resolution that means the 5ms is split into 8^2= 256 ticks so the 100 in the function is the amount of ticks that this is on for and 
                                  // is not out of 100 you can switch it untill 255 dont pass 256 it will cause the code to go to 0 
  }
  else if(currentTarget.actions == right_turn){
    //servo angle and motor speed 
  moveServoToAngle(120);
  digitalWrite(INa, LOW);
  digitalWrite(INb, HIGH);
  digitalWrite(INc, HIGH);
  digitalWrite(INd, LOW);
    // Drive at speed diff speeds
  ledcWrite(ledChannela, 200);        //left motor      
  ledcWrite(ledChannelb, 90);         // right motor
  }
  else if(currentTarget.actions == right_tight){
  moveServoToAngle(120);
  digitalWrite(INa, LOW);   //LEFT
  digitalWrite(INb, HIGH);   //LEFT
  digitalWrite(INc, LOW); //RIGHT
  digitalWrite(INd, HIGH);  //RIGHT
    // Drive at speed diff speeds
  ledcWrite(ledChannela, 150);         //left motor
  ledcWrite(ledChannelb, 70);         // right motor

  }


  else if(currentTarget.actions == stop){
    moveServoToAngle(93);
    Stop();
  } return;
}


