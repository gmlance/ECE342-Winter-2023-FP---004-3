/*The module has four I2C, these addresses are:
 INA219_I2C_ADDRESS1 0x40 //A0 = 0 A1 = 0
 INA219_I2C_ADDRESS2 0x41 //A0 = 1 A1 = 0
 INA219_I2C_ADDRESS3 0x44 //A0 = 0 A1 = 1
 INA219_I2C_ADDRESS4 0x45 //A0 = 1 A1 = 1
 */

#include <Adafruit_INA219.h>
 
Adafruit_INA219 ina219_1(0x40); //Channel 1 current sensor
Adafruit_INA219 ina219_2(0x45); //Channel 2 current sensor
 
const int relay1 = 8;
const int relay2 = 12;
 
void setup(void) 
{
  Serial.begin(9600);
  while (!Serial) {
      // will pause until serial console opens
      delay(1);
 
  }
 
  uint32_t currentFrequency;
    
  Serial.println("Hello!");
  
  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  //ina219.begin();
  if (! ina219_1.begin()) {
    Serial.println("Failed to find INA219 chip for channel 1\n");
    while (1) { delay(10); }
  }
  if (! ina219_2.begin()) {
    Serial.println("Failed to find INA219 chip for channel 2\n");
    while (1) { delay(10);}
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  //ina219.setCalibration_16V_400mA();
 
  Serial.println("Measuring voltage and current with INA219 ...");

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
}
 
void loop() 
{
  //Setup Current and Voltage Measurements for Channel 1
  float shuntvoltage1 = 0;
  float busvoltage1 = 0;
  float current_mA1 = 0;
  float loadvoltage1 = 0;
  float power_mW1 = 0;
 
  shuntvoltage1 = ina219_1.getShuntVoltage_mV();
  busvoltage1 = ina219_1.getBusVoltage_V();
  current_mA1 = ina219_1.getCurrent_mA();
  power_mW1 = ina219_1.getPower_mW();
  loadvoltage1 = busvoltage1 + (shuntvoltage1 / 1000);

    //Setup Current and Voltage Measurements for Channel 2
  float shuntvoltage2 = 0;
  float busvoltage2 = 0;
  float current_mA2 = 0;
  float loadvoltage2 = 0;
  float power_mW2 = 0;
 
  shuntvoltage2 = ina219_2.getShuntVoltage_mV();
  busvoltage2 = ina219_2.getBusVoltage_V();
  current_mA2 = ina219_2.getCurrent_mA();
  power_mW2 = ina219_2.getPower_mW();
  loadvoltage2 = busvoltage2 + (shuntvoltage2 / 1000);
  
  //Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  //Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  //Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  //Serial.print("Current:       "); Serial.print(current_mA1); Serial.println(" mA\n");
  Serial.println("Current 1: " + String(current_mA1) + " mA");
  Serial.println("Current 2: " + String(current_mA2) + " mA\n");
  //Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  //Serial.println("");

  float max_current = 25.0; //mA
  if (max_current < current_mA1) {
    digitalWrite(relay1, LOW);
    Serial.print("Maximum Current Exceeded on Channel 1.\n");
  }
  if (max_current < current_mA2) {
    digitalWrite(relay2, LOW);
    Serial.print("Maximum Current Exceeded on Channel 2.\n");
  }

  delay(1000);
}