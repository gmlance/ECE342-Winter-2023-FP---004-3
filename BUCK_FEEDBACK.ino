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
int pwm = 0;
const int PWM_PIN = 3;  // change this to the pin connected to your PWM device
const int INCREMENT = 1;
const int DELAY_MS = 2000;

float output_voltage = 0.0;
float temp=0.0;
float r1=50000.0;
float r2=10000.0;

void setup() { 
  Serial.begin(9600);  
  //pinMode(potentiometer, INPUT);
  //pinMode(feedback, INPUT);
  pinMode(PWM_PIN, OUTPUT);  
  //TCCR2B = TCCR2B & B11111000 | B00000001;   // on Pin 3 & 5
  //TCCR2B = TCCR2B & B11111000 | B00000001; // for PWM frequency of 31372.55 Hz
  TCCR2B = TCCR2B & B11111000 | B00000010; // for PWM frequency of 3921.16 Hz
  //TCCR2B = TCCR2B & B11111000 | B00000011; // for PWM frequency of 980.39 Hz
  //TCCR2B = TCCR2B & B11111000 | B00000100; // for PWM frequency of 490.20 Hz (The DEFAULT)
  //TCCR2B = TCCR2B & B11111000 | B00000101; // for PWM frequency of 245.10 Hz
  //TCCR2B = TCCR2B & B11111000 | B00000110; // for PWM frequency of 122.55 Hz
  //TCCR2B = TCCR2B & B11111000 | B00000111; // for PWM frequency of 30.64 Hz
}
/*
void loop() {
  static int pwm_value = 0;
  //analogWrite(PWM_PIN, 0);
  analogWrite(PWM_PIN, pwm_value);
  
  if(pwm_value == 0){
    delay(3000);
  }

  pwm_value += INCREMENT;
  if (pwm_value > 255) {
    pwm_value = 0;
  }
  Serial.print(pwm_value);
  Serial.print("\n");
  
  delay(DELAY_MS);
  if(pwm_value == 255){
    delay(3000);
  }
}
*/
void loop() {  
  //float voltage = analogRead(potentiometer);
  //Conversion formula    
  int analog_value = analogRead(A1);     
  temp = (analog_value * 5.0) / 1024.0;   
  output_voltage = temp / (r2/(r1+r2));        
  if (output_voltage < 0.1)   
  {     
    output_voltage=0.0;    
  } 

  Serial.print("v= ");    
  Serial.println(output_voltage);    
  delay(300);

  //float output  = analogRead(feedback);
  float voltage_v = 0.1667*output_voltage;
  float target = 9;

  if (target > output_voltage)
   {
    pwm = pwm+1;
    pwm = constrain(pwm, 1, 254);
   }

  if (target < output_voltage)
   {
    pwm = pwm-1;
    pwm = constrain(pwm, 1, 254);
   }

   analogWrite(PWM_PIN,pwm);
   Serial.print("V_Feedback:");
   Serial.print(voltage_v);
   Serial.print("\n");
}

  

