/* 
 *  Pin constants, these are what I used with the Teensy LC, which has a built in DAC.
 *  Feel free to change these to meet your needs.
 */
const int majorPin = 1;
const int minorPin = 2;
const int offsetPin = 3;
const int potPin = A0;
const int dacPin = A12;
const float gainError = 1.011f;
const float offsetError = 5.558f;

// values used to divide up the knob into 2 octaves
const int adcResolution = 1 << 12; // 12 bits
const int dacResolution = 1 << 12; // 12 bits
const int chromaticDiv = (adcResolution / 25) + 1;
const int diatonicDiv = (adcResolution / 15) + 1;
// sometimes a small offset is needed to align the center detent with 1 octave
const int knobOffset = 0;

unsigned int dacVal = 0; // start on 0v
unsigned int targetVal = 0;
unsigned int diff = 0;
int knob = 0;
int noteIndex = 0; // a value from 0-24 or 0-14, depending on scale, representing 2 octaves + a final note
int noteNum = 0; // a value from 0-36, representing a semi-tone value
int currentScale = 0; // chromatic

int knobDiv = chromaticDiv; // start in chromatic
int vOffset = 0; // start at 0v, use 1241 to start at 1v
int slew = 4; // 4ms delay for slew, you can play with this or add a pot to control it. 0 will result in stepping, and I found 10 is as high as you'd want to go.

// scale offsets to be used to calculate DAC value
unsigned int scales[3][25] = {
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
  { 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24 },
  { 0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 20, 22, 24 }
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
  // vOffset = digitalRead(offsetPin) * voltOffset;

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
  noteIndex = floor(knob / knobDiv);
  // Serial.println(string(knob) - 
  // find the target value that corresponds to the voltage for the note number from the scale look up 
  // add octave offset if enabled
  noteNum = scales[currentScale][noteIndex] + (digitalRead(offsetPin) * 12);
  // convert from note number to code point
  targetVal = ((((float)noteNum / 12.0f) / 3.3f) * (dacResolution - 1) * gainError) + offsetError;

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
      // Serial.println(String(knob) + " - " + String(noteIndex + 1) + " - " + String(dacVal) + " - " + String(targetVal));
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
      // Serial.println(String(knob) + " - " + String(noteIndex + 1) + " - " + String(dacVal) + " - " + String(targetVal));
    } else {
      // set it to our target
      dacVal = targetVal;
    }
  }
  
  // write to the DAC
  analogWrite(dacPin, dacVal);
}
