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

int leftSignalA = 0;
int leftSignalB = 0;
int pulseCount = 0;


const float singlePulseDist = ( M_PI * 3.32) / 36;       

// Input distance wanted into this variable IN CM
const int InputDist = 500;


int NumOfPulses = (int)(InputDist / singlePulseDist);   

// State mapping variables
enum States {S0 = 0, S1 = 1, S2 = 2, S3 = 3};
States currentState;

const int encoderTable[4][4]={
    /*New states      00,  01, 10, 11*/
    /*old state 00*/{  0,  0,  1,   -1},
    /*old state 01*/{  0,  0,  0,   0 },
    /*old state 10*/{ -1,  0,  0,   1 },
    /*old state 11*/{  1,  0, -1,   0 }       
};                

//Debounce filter 
int stableStateNum = 0;
int debounceCounter = 0;
const int debounceThreshold = 6;     // i need a consecutive 15 of the same state to use the matix table

int currentStateNum;
int previousStateNum;

void setup() {
  // Configuring encoder pins as inputs with pullups active
  pinMode(leftmotorChannelaPin, INPUT_PULLUP);
  pinMode(leftmotorChannelbPin, INPUT_PULLUP);
  
  // Configure motor direction pins as outputs
  pinMode(INa, OUTPUT);
  pinMode(INb, OUTPUT);
  pinMode(INc, OUTPUT);
  pinMode(INd, OUTPUT);

  // Attach PWM channels to pins
  ledcAttachPin(enA, ledChannela);
  ledcAttachPin(enB, ledChannelb);

  // Initialize PWM channels (1000 Hz PWM, 8-bit resolution)
  ledcSetup(ledChannela, 1000, 8); 
  ledcSetup(ledChannelb, 1000, 8); 

  // Read initial startup position of encoder
  leftSignalA = digitalRead(leftmotorChannelaPin);
  leftSignalB = digitalRead(leftmotorChannelbPin);
  
  if(leftSignalA == 0 && leftSignalB == 0)      { currentState = S0; previousStateNum = 0; }
  else if(leftSignalA == 1 && leftSignalB == 0) { currentState = S1; previousStateNum = 2; }         
  else if(leftSignalA == 1 && leftSignalB == 1) { currentState = S2; previousStateNum = 3; }         
  else                                          { currentState = S3; previousStateNum = 1; }          

  //initialises what the state is at the start for the debounce filter
  stableStateNum = previousStateNum;

  // Begin serial communication
  Serial.begin(115200);
  Serial.println("ESP32 Running..."); 
  Serial.print("Target Pulses to reach: ");
  Serial.println(NumOfPulses);
}

void loop() {
  //  Read pins instantly
  leftSignalA = digitalRead(leftmotorChannelaPin);
  leftSignalB = digitalRead(leftmotorChannelbPin);

  //  Map pins to matrix state index
  currentStateNum = (leftSignalA << 1) | leftSignalB;             

  //  Process the encoder changes
  LeftEncoderPulseCount(leftSignalA, leftSignalB);

  //  Evaluate if target is reached
  DistanceCalculator(pulseCount);
}

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
  
  // Print final resting position 
  static bool printed = false;
  if(!printed) {
   // Serial.print("Target Reached! Final Pulse Count: ");
    Serial.println(pulseCount);
    printed = true;
  }
}

void DistanceCalculator(int currentPulses){      
  if(currentPulses < NumOfPulses){
    goForwards();
  }
  else{
    Stop();
  }
}

int LeftEncoderPulseCount(int leftSignaA, int leftSignalB){
  
  // If the pins match our accepted stable state, reset the counter
  if (currentStateNum == stableStateNum) {
    debounceCounter = 0;
  } 
  // If the pins changed, start counting how long they stay at this new value
  else {
    debounceCounter++;
    
    //  Only accept the state change if it clears the threshold without flickering
    if (debounceCounter >= debounceThreshold) {
      int MatrixValue = encoderTable[stableStateNum][currentStateNum];
      pulseCount += MatrixValue;
      
      //Serial.print("Live Count: ");
      Serial.println(pulseCount);
      
      // Update our memories to the new confirmed state
      stableStateNum = currentStateNum;
      previousStateNum = currentStateNum;
      debounceCounter = 0;
    }
  }
  return pulseCount;
}
