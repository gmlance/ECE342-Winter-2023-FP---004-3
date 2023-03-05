/* This is an example code for a BUCK converter circuit made with arduino
 * I've used arduino NANO. We have to set the timer of the PWM on pin D3
 * The feedback is connected to A1 and we set the desired voltage with a
 * potnetiometer connected to A0.
 * 
 * Subscribe: http://www.youtube.com/c/electronoobs
 * webpage: http://www.electronoobs.com/eng_circuitos_tut10.php
 */

 
int potentiometer = A0;
int feedback = A1;
int PWM = 3;
int pwm = 0;

void setup() {
  pinMode(potentiometer, INPUT);
  pinMode(feedback, INPUT);
  pinMode(PWM, OUTPUT);  
  TCCR2B = TCCR2B & B11111000 | B00000001;   // on Pin 3 & 5
  //TCCR2B = TCCR2B & B11111000 | B00000001; // for PWM frequency of 31372.55 Hz
  //TCCR2B = TCCR2B & B11111000 | B00000010; // for PWM frequency of 3921.16 Hz
  //TCCR2B = TCCR2B & B11111000 | B00000011; // for PWM frequency of 980.39 Hz
  //TCCR2B = TCCR2B & B11111000 | B00000100; // for PWM frequency of 490.20 Hz (The DEFAULT)
  //TCCR2B = TCCR2B & B11111000 | B00000101; // for PWM frequency of 245.10 Hz
  //TCCR2B = TCCR2B & B11111000 | B00000110; // for PWM frequency of 122.55 Hz
  //TCCR2B = TCCR2B & B11111000 | B00000111; // for PWM frequency of 30.64 Hz
}

void loop() {  
  //float voltage = analogRead(potentiometer);
  float output  = analogRead(feedback);
  float voltage = 2.5;

  if (voltage > output)
   {
    pwm = pwm-1;
    pwm = constrain(pwm, 1, 254);
   }

  if (voltage < output)
   {
    pwm = pwm+1;
    pwm = constrain(pwm, 1, 254);
   }

   analogWrite(PWM,125);
}
