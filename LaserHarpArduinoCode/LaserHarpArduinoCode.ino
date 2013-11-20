
#define NUM_SCALES 1
#define NUM_NOTES 12
#define NUM_NOTES_AT_ONCE 2
#define START_PIN 2
#define LIGHT_TOLERANCE 60
#define NO_NOTE 101 //This is used as a place holder if there is no note to be played in the chord
#define BROKEN_KEY_THRESHOLD 200 //this is the value after which a key should be considered broken
#define MAX_VELOCITY 120 //this is the max possible velocity of a note
#define NUM_FUNCTIONS 6 //the number of additional functions allowed by the harp
#define TIME_GAP 10 //the number of cycles before taking another mode change sense.
#define NUM_OCTAVES 4
//Define the ping pins
const int leftPing = A0;
const int rightPing = A1;
int leftPingMaxDistance;
int rightPingMaxDistance;

const int noteON = 144;//144 = 10010000 in binary, note on command
const int noteOFF = 128;//128 = 10000000 in binary, note off command
const int controllerChange = 160; //160 = 10110000 in binary, controller change command

//Controller values
const int footPedal = 4; //The foot pedal controller

const int light_threshold = 50;
int velocity = 127;//velocity of MIDI notes, must be between 0 and 127
int octave_boost = 0;//The number of octaves the scale should be shifted by
int step_shift = 0; //The number of steps all of the notes should be shifted by
int current_scale = 0; //The current scale that we are playing
//int notes[NUM_NOTES] = {48,49,50,51,52,53,54,55,56,57,58,59,60};
//The default array structure will be a 3-d array for all the notes. The outermost array will allow you
//to index to the scale you would like. Within the scale, the next innermost array will allow you to select
//the broken string. Finally, all of the notes in that string's array will be played
//Harry Potter song notes
//int notes[NUM_SCALES][NUM_NOTES][NUM_NOTES_AT_ONCE] = {{{3},{6},{8},{9},{10},{11},{12},{13},{15},{16},{17},{18}}};
int notes[NUM_SCALES][NUM_NOTES][NUM_NOTES_AT_ONCE] = {{{3,NO_NOTE},{6,NO_NOTE},{8,NO_NOTE},{9,NO_NOTE},{0,NO_NOTE},{10,NO_NOTE},{11,NO_NOTE},{12,NO_NOTE},{13,NO_NOTE},{15,NO_NOTE},{16,NO_NOTE},{17,NO_NOTE}}};
boolean on[NUM_NOTES]; //A boolean array indicating whether a note is on or not
int thresholds[NUM_NOTES]; //The light thresholds for all the notes
long rightDuration; //The duration of the left ping sensor
long leftDuration; //The duration of the right ping sensor
long prevLeftDistance;
//Define LED pins
int led1 = A2;
int led2 = A3;
int led3 = A4;
//Define the current value change state
int valueState=1;
//Define the current value of the dynamic state change
int changeState=0;
//Ping counter for the mode change
int leftCounter = 0;
//NOTE: This setup assumes that the laser will be started in its final lighting with all the lasers shining on the sensors
void setup()                    // run once, when the sketch starts
{
   //Set serial to output at the MIDI baud rate
  Serial.begin(31250);
   //Serial.begin(4800);
  //Initialize the note array to having no notes played
  for(int i=0;i<NUM_NOTES;i++)
  {
    on[i]=1;
  }
  //Get the threshold values for all photoresistors
  for(int i=START_PIN;i<START_PIN+NUM_NOTES;i++){
    //Take an average of 4 measurements to try to minimize error
    int sum=0;
    for(int j=0;j<4;j++)
    {
      sum+=RCinit(i);
    }
    thresholds[i-START_PIN]=sum/4+LIGHT_TOLERANCE;
    //Serial.println(thresholds[i-START_PIN]);
  }
  //Get the max value of the ping pin
  leftPingMaxDistance = getPing(leftPing);
  rightPingMaxDistance = getPing(rightPing);
  //Set up the LED pin modes
  pinMode(led1, OUTPUT);   
  pinMode(led2,OUTPUT);
  pinMode(led3,OUTPUT); 
  lightLeds(1);
}
//The main reading/sensing loop for the robot.
void loop()
{
  //Read the distance sensors and act accordingly
  runDistanceCalculations();
  //int rightDistance = getPing(rightPing);
  //MIDImessage(176,4,120*((double)rightDistance/rightPingMaxDistance));
 
 
 
 //Read the value for all photoresistors and act accordingly
 for(int i=START_PIN;i<START_PIN+NUM_NOTES;i++){
   int time = RCtime(i); // Figure out the time constant of the RC circuit
   int laser_present = time<thresholds[i-START_PIN]; //See if the laser is present
   if(laser_present!=on[i-START_PIN]) //If the laser state is different than its previous state, we need to either play or pause the note
   {
     on[i-START_PIN] = laser_present; // Set the state to the current value
     if(laser_present) // If the laser is gone, kill all of the laser's values
     {
       //Make sure to kill notes in all octaves
       for(int k=0;k<NUM_NOTES_AT_ONCE;k++)
       {
         for(int j=0;j<NUM_OCTAVES+3;j++)
         {
           //And scales
           for(int l=0;l<NUM_SCALES;l++)
           MIDImessage(noteOFF, notes[l][i-START_PIN][k]+12*(j+1), velocity); //Turn the note on
         }
       }
     }
     else{ //If the laser is now gone, turn all of the laser's notes on
      for(int k=0;k<NUM_NOTES_AT_ONCE;k++)
       {
       MIDImessage(noteON, notes[current_scale][i-START_PIN][k]+12*octave_boost, velocity); //Turn the note on
       }
     }
   }   
 }
}
//Act according to the distance specifications
void runDistanceCalculations(){
  
  //Figure out our mode from the left PING
  if(leftCounter>TIME_GAP){
    leftCounter = 0;
  //First figure out what mode we're on
  int leftTime = getPing(leftPing);
  //Make sure the hand isn't absent
  if(leftTime<leftPingMaxDistance*(NUM_FUNCTIONS)/(NUM_FUNCTIONS+2)){
  //Calculate the state and leave the last bit of distance for a hand absent command
  valueState = leftTime/(leftPingMaxDistance/(NUM_FUNCTIONS+2));
  //Make sure we don't go over the number of functions allowed.. This is for callibration purposes
  if(valueState > NUM_FUNCTIONS)
     valueState = NUM_FUNCTIONS;
  if(valueState <1)
     valueState = 1;
  //Light up the LED matrix so the user knows the current system state
  lightLeds(valueState);
  }
  }
  else
  leftCounter++;
  
  //Act on the mode using the right ping
  int rightTime = getPing(rightPing);
  //Make sure the hand isn't absent by reserving the top 5/6 of the board
  if(rightTime>rightPingMaxDistance*5/6)
    return;
  //Get the value to output to the system
   int midiOutput = 120*((double)rightTime/(rightPingMaxDistance*5/6));
  //Now decide what we're going to do based on the state
  //The first two are coded into the system. Ones after that are unassigned so the user can 
  //assign values to them in FLStudio
  switch (valueState)
  {
    
     //Change the scale
     case 2: 
     current_scale = rightTime/(rightPingMaxDistance*5/6/(NUM_SCALES));
     if(current_scale<0)
     current_scale=0;
     break;
     case 3:
        MIDImessage(176,4,midiOutput);
        break;
     case 4:
        MIDImessage(176,5,midiOutput);
        break;
     case 5:
        MIDImessage(176,6,midiOutput);
        break;
     case 6:
        MIDImessage(176,7,midiOutput);
        break;
        //By default, change the octave
      default:
      octave_boost = rightTime/(rightPingMaxDistance*5/6/(NUM_OCTAVES))+1;
      break;
     
     

  }  
  
    //TODO add smoothing
  return;
  
  
}
//send MIDI message
void MIDImessage(int command, int MIDInote, int MIDIvelocity) {
  if(MIDInote==NO_NOTE) return;
  Serial.write(command);//send note on or note off command 
  Serial.write(MIDInote);//send pitch data
  Serial.write(MIDIvelocity);//send velocity data
}
//Get the RC time constant but don't perform optimization checking
long RCinit(int sensPin){
   long result = 0;
   pinMode(sensPin, OUTPUT);       // make pin OUTPUT
   digitalWrite(sensPin, HIGH);    // make pin HIGH to discharge capacitor - study the schematic
   delay(1);                       // wait a  ms to make sure cap is discharged

   pinMode(sensPin, INPUT);        // turn pin into an input and time till pin goes low
   digitalWrite(sensPin, LOW);     // turn pullups off - or it won't work
   while(digitalRead(sensPin)){    // wait for pin to go low
      if(result>BROKEN_KEY_THRESHOLD)
      {
        return result;
      }
      result++;
   }

   return result;                   // report results 
}
//Get RC time constant with optimization checking
long RCtime(int sensPin){
   long result = 0;
   pinMode(sensPin, OUTPUT);       // make pin OUTPUT
   digitalWrite(sensPin, HIGH);    // make pin HIGH to discharge capacitor - study the schematic
   delay(1);                       // wait a  ms to make sure cap is discharged

   pinMode(sensPin, INPUT);        // turn pin into an input and time till pin goes low
   digitalWrite(sensPin, LOW);     // turn pullups off - or it won't work
   while(digitalRead(sensPin)){    // wait for pin to go low
      result++;
      if(result>BROKEN_KEY_THRESHOLD)return 0; //If the key is broken, just act like a laser is there
      if(result>thresholds[sensPin-START_PIN]) //If the result is greater than the light threshold, stop because we know the laser is not there
      {
        return result+100; // Return a big number. The laser isn't there
      }
   }
   return result;                   // report results   
}   
//Read the time value of the ping and return it
long getPing(int sensPin){
 long result =0;
pinMode(sensPin, OUTPUT);
digitalWrite(sensPin, LOW);
delayMicroseconds(2);
  digitalWrite(sensPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(sensPin, LOW);
  pinMode(sensPin, INPUT);
  result = pulseIn(sensPin, HIGH);
  return result;
}
//Light the LED lights to show the current change value state of the system
void lightLeds(int num){
  if(num==0) digitalWrite(led1,HIGH);
  digitalWrite(led1,num%2);
  digitalWrite(led2,num/2%2);
  digitalWrite(led3,num/2/2%2);
}
