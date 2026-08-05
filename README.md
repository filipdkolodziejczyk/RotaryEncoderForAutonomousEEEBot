# RotaryEncoderForAutonomousEEEBot
Using the PEC12-R to track distance on a autonomous vehicle. Peforming manoevers like: a figure of eight or a tight U-turn.

## Project context and motivations
In my first year i wasn't able to go into depth with using the encoders. We had a challenge at the end of the first project week to travel accuartely 10 meters in the main hall.My solution was to just have a delay() which is eqivalent to 10 meters but this doesn't take into account different floor textures so, my solution was crap. there was additional challeneges like the figure of eight which would set the foundation for latter project weeks which involved parking and peforming certain manovers


I decided to revisit this during my gap year (2025-2026) before i come back for my final 3rd year (2026-2027) and plug some holes and dust off some rust.


 Initially, my code was simple trying to figure out how to use the motors how to set the speeds then, moved onto tracking the the encoder signals. From looking at oscillioscope readings and comparing what the datasheet said about what the signals should look like, i came to the conclusion that  the encoders are very cheap and not the best so i needed to address that in my code.
 
 
 Another problem was me not understanding how fast the microcontroller loops through the code and becuase the encoders are cheap this will be a big problem. Any type of movement or shaking would trigger the encoders which would record these anomolous values and not give me accurate readings. Im not going to lie to you, i put this through ai and it came up with something called a debounce counter so it gave me the skeleton code, implemented it. Quite simple, if the wheel rotated it would need to check whether it stayed at that position for a consecutive amount of readings and only then it would register that as a movement. Quite clever, the satisfaction wasn't as good becuase i didnt figure it out but i made it work. I'm not here to reinvent the wheel.
 
 
In my second year i was introduced to object orientated programming (OOP) in cpp which i found quite difficult at the time, the syntax was okay it was just when will i use this? going back to this, this was such an obvious solution to making the car do a figure of eight and making it look good and logical in the code but, me in first year would never in a million years think to do that; anyway...


Finally, like i mentioned, how can i logially write some code which makes the vehicle do a figure of eight, turn right,left etc... (OOP!!!). Adding the other encoder then making both enocders work together using the same matrix look up table. I found it quite difficult managing so much at once making sure it all work whilst doing all the checks so it doent give me trash. I'm still quite suprised, how much better something can be from the simple switch case and also how much you can do with just an encoder which has 2 signals, Crazy.



## V1.0 Switch case: a full encoder circuit assumptions and behaviours , The concept, the code and justification, faliures and workarounds 

# The circuit: a full encoder circuit assumptions and behaviours 
The first circuit below is what one encoder looks like it has 2 channels a and b which are essentially switches which are controlled by rotating the encoder --> metal contacts touch --> closes switch --> metal contacts disconnect --> switch opens.


the microcontroller pins gpio 36 and 39 in my circuit analysis will be treated as though they are open circuits so, current cannot flow into it. You can think of it as an op amp golden rule.


the capacitors(c1,c2) which forms the rc filter wil be treated as if it was in the steady state (ss) and not the tranient state when the switching states are high or low respectivley so, in ss the cap will be fully charged or discharged,no current will be able to flow through this and into gnd, i will talk about the capacitor in the transient state later. 


<p align="center">
  <img src="assets/FullEncoderCircuit.jpg" alt="Encoder Circuit" width="500"/>
</p>

# The circuit: half an encoder circuit explanation and analysis
It's much easier to understand what one side of the circuit, the other side does the exact same thing but is a step behind or ahead depending if the encoder is going forwards or backwards 


# Encoder circuit switch A is open 


<p align="center">
  <img src="assets/EncoderCircuitSignalA_Open.jpg" alt="Encoder Circuit open" width="500"/>
</p>


I will explain what the condtions where before the switch was opened the instant thats it was opened (transient) and after the was opened (steady state). 


## before it was opened:

( you have to trust that these where the conditions without looking to deeply into this, it will make sense once i explain both open and closed encoder switch)


- The capacitor is fully discharged 


- the current was flow: +5V --> node 0 --> through R1 --> node 1 --> GND 


- the microcontroller at node 2 was seeing 0V which means that the signal was LOW, 0 or 0V


## The instant that encoder switch is opened : (TRANSIENT)

- Current flow +5V --> node 0 --> through R1 --> node 1 --> through R2 --> node 2 --> charging c1 --> GND


- the time constant of the capacitor = R1 * R2 * C1 

-  in the encoder it usually is a spring which contacts the metal. This induces vibrations which will cause the voltage to oscillate for a few microseconds before settling down for its intended state. This is prevelant in all encoders but can be mitigated through the use of gold contacts or a better spring constant of the spring used. So to summaries the encoder will bounce between 0V and 5V, between state switch on and state switch off. This causes the transients/ noise. This is called CONTACT BOUNCE.


The micocontroller processes instruction at a rate of 240MHz so it will register every single switch state change which could cause erratic behaviour when it comes to tracking the distance travelled.


WITHOUT a capacitor node 2 will bounce up and down untill it settle. this will cuase the noise and will ruin the tracking. WITH a capacitor the voltage spikes will be smoothed out which the microcontroller can interpret as a rising edge. the frequency of the spike will be faster than the time it takes to charge the cpacitor (this will be explained in depth below).


Time consant of the cap:
defined by: tau = r*c ==> tau = R1*R2*C1 and for charging capacitor 1 tau = 63% charge of the cap im not going into the maths here


It takes about 5 * tau for a complete charge discharge


if tau << frequency of oscillation 0V to 5V:


cap charges and discharges instantly so there is no smoothing


if tau == frequency of oscillation 0V to 5V:

the voltage at node 2 becomes wobbly at at a median value like 2.5V which is no good for a microcontroller which has thresholds for what it considers as high or low 


if tau > frequency of oscillation 0V to 5V:

The capacitor is slightly too slow(5ms) to react to the individual spikes (50us changes). this means that the voltage is averaged out which is what i need. 

if  tau >> frequency of oscillation 0V to 5V: The capacitor will take too long to charge / discharge so the signal will change in real time but the cpacitor is taking too long hence there will be data lost.


## The encoder switch is open (STEADY STATE):


- To reiterate, ther current can't flow through the encoder becuase the switch is opened. The current can't flow through the capacitor anymore becuase the voltage at is no longer bouncing so no current flows to the gnd through the cap ( I = C*dV/dt) and the assumption we had that the microcontroller is treated a if it is an open circuit means that anywhere on the circuit the voltage is 5V becuase no current can flow. The microcontroller sees this and at node 2 the voltage is 5V therefore, the signal is high.


# summary : when the encoder switch is open, signal A = high






# Encoder circuit switch A is closed


<p align="center">
  <img src="assets/EncoderCircuitSignalA_Closed.jpg" alt="Encoder Circuit closed" width="500"/>
</p>


Now that i have disscused the open circuit the conditions before the switch was closed makes more sense now.


## before encoder was closed:

it must've been open so: 

- capacitor is fully charged 

- no current is flowing through the circuit 

- same microcontroller assumption that its a open circuit from node 2 to microcontroller but, because the switch is open signal A = high


## the instant the encoder was closed (TRANSIENT)


- Current flows: +5V --> node 0 --> through R1 --> node 1 --> through ENCODER --> GND


- because the capacitor is fully charged it starts to discharge through R2 only and that node 1 is connected to GND hence,
tau = R2*C1

- microcontroller as disscused in the open case sees the signal start to bounce, the capacitor smooths the signal  


## The encoder switch is closed (STEADY STATE):

- c1 has now fully discharged, becuase the current is only flowing from node 1 to gnd through the encoder. Node 2 now becomes 0V hence signal A = low as seen from the microcontroller.

# why is the pull-up resistor needed here?

- if there was no resistor for the closed case that would mean that the current going from the supply to the gnd through the encoder would be infinite (I = V/R, R = 0) but, in reality this would trigger the current limiter protection or burn through your components

- it also serves a purpose of being part of the reistance which form the calculation for the time constant for the open switch version of the circuit





# summary : when the encoder switch is open, signal A = low




