#define enA 33  // Enable A command line
#define enB 25  // Enable B command line

#define INa 26  // Channel A Direction
#define INb 27  // Channel A Direction
#define INc 14  // Channel B Direction
#define INd 12  // Channel B Direction

#include <math.h>

const int ledChannela = 0;
const int ledChannelb = 1;
//gpio pins used for left motor
const int leftmotorChannelaPin = 36;
const int leftmotorChannelbPin = 39;

int i = 0;
int leftSignalA = 0;
int leftSignalB = 0;
int pulseCount = 0;

//enum assigns variables to numbers but it is always starts with 0 then 1 so on..
enum States {S0 = 0, S1 = 1, S2 = 2, S3 = 3};
States currentState;
//define single pulse distance IN CM
const float singlePulseDist = (M_PI*3.1)/5;
//Input distance wanted into this variable IN CM
const int InputDist = 38;

void setup() {

  //conffiguring encoder pins as inputs
  pinMode(leftmotorChannelaPin,INPUT);
  pinMode(leftmotorChannelbPin,INPUT);
  
  // Configure motor direction pins as outputs
  pinMode(INa, OUTPUT);
  pinMode(INb, OUTPUT);
  pinMode(INc, OUTPUT);
  pinMode(INd, OUTPUT);

  // Attach PWM channels to pins
  ledcAttachPin(enA, ledChannela);
  ledcAttachPin(enB, ledChannelb);

  // Initialize PWM channels
  ledcSetup(ledChannela, 1000, 8); // 1000 Hz PWM, 8-bit resolution
  ledcSetup(ledChannelb, 1000, 8); // 1000 Hz PWM, 8-bit resolution

  //define a cuurent state depending on real position of encoder and also reaidng it just once before it starts 
  leftSignalA = digitalRead(leftmotorChannelaPin);
  leftSignalB = digitalRead(leftmotorChannelbPin);
  if(leftSignalA == 0 && leftSignalB == 0){
    currentState = S0;
  }
  else if(leftSignalA == 1 && leftSignalB == 0){
    currentState = S1;
  }
  else if(leftSignalA == 1 && leftSignalB == 1){
    currentState = S2;
  }
else{
  currentState = S3;
}
  // Begin serial communication
  Serial.begin(115200);
  Serial.println("ESP32 Running"); 
}

void loop() {
  leftSignalA = digitalRead(leftmotorChannelaPin);
  leftSignalB = digitalRead(leftmotorChannelbPin);
  //Serial.println(singlePulseDist);

  //Serial.print("LeftMotor Channel A : ");
  //Serial.println(leftSignalA);

  //Serial.print("LeftMotor Channel B : ");
  //Serial.println(leftSignalB);
  LeftEncoderPulseCount(leftSignalA,leftSignalB);

  goForwards();
  // Add a delay or other functions to control when directions change or stop
  DistanceCalculator (pulseCount);

  
}

void goForwards() {


  // Set motor direction for forwards movement
  digitalWrite(INa, LOW);
  digitalWrite(INb, HIGH);
  digitalWrite(INc, HIGH);
  digitalWrite(INd, LOW);


  // Set speed (duty cycle) for both motors
  ledcWrite(ledChannela, 90); // Full speed for motor A
  ledcWrite(ledChannelb, 90); // Full speed for motor B

  //delay(1000); // Example delay - adjust as needed for your application

  // You might want to add code here to stop the motors after moving forward
}

void Stop() {
  // Set motor direction for forwards movement
  digitalWrite(INa, HIGH);
  digitalWrite(INb, LOW);
  digitalWrite(INc, LOW);
  digitalWrite(INd, HIGH);
  // Set speed (duty cycle) for both motors
  ledcWrite(ledChannela, 100); // Full speed for motor A
  ledcWrite(ledChannelb, 100); // Full speed for motor B
  delay(60);
  digitalWrite(INa, HIGH);
  digitalWrite(INb, LOW);
  digitalWrite(INc, LOW);
  digitalWrite(INd, HIGH);
  // Set speed (duty cycle) for both motors
  ledcWrite(ledChannela, 0); // Full speed for motor A
  ledcWrite(ledChannelb, 0); // Full speed for motor B
  delay(100000000);
  Serial.print("Distance travelled/cm : ");
  Serial.println(InputDist);
}

void DistanceCalculator (int pulseCount){
  int NumOfPulses = (InputDist-10)/singlePulseDist;       //19cm is the distance from the back to the front wheels to take into account that the back wheels will be set back 
  if(pulseCount>NumOfPulses){
    Stop();
  }
}

int LeftEncoderPulseCount (int leftSignaA, int leftSignalB){

      switch(currentState){
        case S0:                                    //currently at S0
         if(leftSignalA == 1 && leftSignalB == 0){
           currentState = S1;                          // MOVE TO NEXT STATE
          pulseCount ++;
          Serial.print("Pulse count: ");                              //mealy machine output
          Serial.println(pulseCount);                 // printing mealy machine output
         }
       else if(leftSignalA == 0 && leftSignalB == 1) {
        currentState = S3;                            //backwards movement
       }
       break;
    
        case S1:
        if(leftSignalA == 1 && leftSignalB == 1){
          currentState = S2;
          pulseCount ++;
          Serial.print("Pulse count: ");
          Serial.println(pulseCount); 
        }
        else if(leftSignalA == 0 && leftSignalB ==0){
          currentState = S0;                        // backwards movement
        }
        break;
      
        case S2:
        if(leftSignalA == 0 && leftSignalB == 1){
          currentState = S3;
          pulseCount ++;
          Serial.print("Pulse count: ");
          Serial.println(pulseCount); 
        }
        else if(leftSignalA == 1 && leftSignalB == 0)
        currentState = S1;                          // backward movement
        break;

        case S3:
        if(leftSignalA == 0 && leftSignalB == 0){
          currentState = S0;
          pulseCount ++;
          Serial.print("Pulse count: ");
          Serial.println(pulseCount);
        }
        else if (leftSignalA == 1 && leftSignalB == 1){
          currentState = S2;                        //backwards
        }
        break;
      }
      return pulseCount;
}
      

