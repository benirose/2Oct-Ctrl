/* 
 *  Pin constants, these are what I used with the Teensy LC, which has a built in DAC.
 *  Feel free to change these to meet your needs.
 */
const int majorPin = 1;
const int minorPin = 2;
const int offsetPin = 3;
const int potPin = A0;
const int dacPin = A12;

/* 
 * These are the values to divide the readings of the pitch knob to get 2 octaves worth of notes. Use different values  
 * depending on if you are reading at 10-bit or 12-bit.
 * 42 for chromatic, 73 for diatonic @ 10-bit
 * 164 for chromatic, 274 for diatonic @ 12-bit
 */
const int chromaticDiv = 164;
const int diatonicDiv = 274;
const int voltOffset = 1241;

unsigned int dacVal = 0; // start on 0v
unsigned int targetVal = 0;
unsigned int diff = 0;
int knob = 0;
int noteNum = 0; // a value from 0-24 or 0-14, depending on scale, representing 2 octaves + a final note
int currentScale = 0; // chromatic

int knobDiv = chromaticDiv; // start in chromatic
int vOffset = 0; // start at 0v, use 1241 to start at 1v
int slew = 4; // 4ms delay for slew, you can play with this or add a pot to control it. 0 will result in stepping, and I found 10 is as high as you'd want to go.

/*
 * These are scale note values calculated to output proper pitches following the 1v/oct standard when using a 12-bit DAC.
 */
unsigned int scales[3][25] = {
  { 0, 103, 206, 310, 413, 517, 620, 724, 827, 930, 1034, 1137, 1241, 1344, 1448, 1551, 1654, 1758, 1861, 1965, 2068, 2172, 2275, 2378, 2482 },
  { 0, 206, 413, 517, 724, 930, 1137, 1241, 1448, 1654, 1758, 1965, 2172, 2378, 2484 },
  { 0, 206, 310, 517, 724, 827, 1034, 1241, 1448, 1551, 1758, 1965, 2068, 2275, 2484 }
};



void setup() {
  pinMode(majorPin, INPUT_PULLUP); // minor scale
  pinMode(minorPin, INPUT_PULLUP); // major scale
  pinMode(offsetPin, INPUT_PULLUP); // 1v offset
  analogReadAveraging(16);
  analogReadResolution(12);
  analogWriteResolution(12);
  Serial.begin(9600);
}

void loop() {
  // read offest switch and set offset accordingly
  vOffset = digitalRead(offsetPin) * voltOffset;

  // read scale switch inputs and set knob division and scale index accordingly
  // scale switch is a 3 position, on-off-on switch, using two inputs for major and minor, and chromatic if neither
  if (digitalRead(majorPin) == LOW) {
    // major
    knobDiv = diatonicDiv;
    currentScale = 1;
  } else if (digitalRead(minorPin) == LOW) {
    // minor
    knobDiv = diatonicDiv;
    currentScale = 2;
  } else {
    knobDiv = chromaticDiv;
    currentScale = 0;
  }

  // read knob position and determine what number note it is in the 2 octaves
  knob = analogRead(potPin);
  noteNum = floor(knob / knobDiv);
  // find the target value that corresponds to the voltage for the note number from the scale look up 
  targetVal = scales[currentScale][noteNum] + vOffset;

  /* glide to the note, the further away it is from our current DAC values the more we will glide to it */

  // if new note is higher than current
  if (targetVal > dacVal) {
    // get the difference
    diff = targetVal - dacVal;
    // if we're more than 20 away
    if (diff > 20) {
      // jump by half the difference
      dacVal = dacVal + (diff / 10);
      delay(slew);
      //Serial.println(String(knob) + " - " + String(noteNum + 1) + " - " + String(dacVal) + " - " + String(targetVal));
    } else {
      // set it to our target
      dacVal = targetVal;
    }
  }

  // if new note is lower than current
  if (targetVal < dacVal) {
    // get the difference
    diff = dacVal - targetVal;
    // if we're more than 20 away;
    if (diff > 20) {
      // jump by half the difference
      dacVal = dacVal - (diff / 10);
      delay(slew);
      //Serial.println(String(knob) + " - " + String(noteNum + 1) + " - " + String(dacVal) + " - " + String(targetVal));
    } else {
      // set it to our target
      dacVal = targetVal;
    }
  }
  
  // write to the DAC
  analogWrite(dacPin, dacVal);
}
