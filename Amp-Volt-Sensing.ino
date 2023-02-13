/*The module has four I2C, these addresses are:
 INA219_I2C_ADDRESS1 0x40 //A0 = 0 A1 = 0
 INA219_I2C_ADDRESS2 0x41 //A0 = 1 A1 = 0
 INA219_I2C_ADDRESS3 0x44 //A0 = 0 A1 = 1
 INA219_I2C_ADDRESS4 0x45 //A0 = 1 A1 = 1
 */

#include <Adafruit_INA219.h>

Adafruit_INA219 ina219(0x40); //Channel 2 current sensor
 
const int relay1 = 8;
const int relay2 = 12;
 
void setup(void) 
{
  Serial.begin(9600);
  while (!Serial) {
      // will pause Zero, Leonardo, etc until serial console opens
      delay(1);
 
  }
 
  uint32_t currentFrequency;
    
  Serial.println("Hello!");
  
  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  //ina219.begin();
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip\n");
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

    //Setup Current and Voltage Measurements for Channel 2
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
 
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  //Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  //Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  //Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  //Serial.print("Current:       "); Serial.print(current_mA1); Serial.println(" mA\n");
  Serial.println("Current: " + String(current_mA) + " mA\n");
  //Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  //Serial.println("");

  float max_current = 12.0; //mA
  if (max_current < current_mA) {
    digitalWrite(relay1, LOW);
    Serial.print("Maximum Current Exceeded.\n");
  }

  delay(1000);
}