/*
PC Controlled Power Supply

Hardware:
3 INA219 current sensors
  /The module has four I2C, these addresses are:
  INA219_I2C_ADDRESS1 0x40 //A0 = 0 A1 = 0
  INA219_I2C_ADDRESS2 0x41 //A0 = 1 A1 = 0
  INA219_I2C_ADDRESS3 0x44 //A0 = 0 A1 = 1
  INA219_I2C_ADDRESS4 0x45 //A0 = 1 A1 = 1
  /
1 Adafruit temperature sensor (MCP9808)
1 I2C LCD (1602)


SCPI Commands Supported:
  *IDN?
    identifies instrument
  SYST:DCS:OUTP?
    outputs the state of the power supply (all measured and maximum values)
  SYST:DCS:CH1:VOLT <value>
  SYST:DCS:CH2:VOLT <value>
    sets output voltage of selected channel, valid for [2,14] V
  SYST:DCS:CH1:VOLT?
  SYST:DCS:CH2:VOLT?
    outputs selected channel voltages
  SYST:DCS:CH1:OUTP?
  SYST:DCS:CH2:OUTP?
    outputs selected channel voltages and currents
  SYST:DCS:CH1:CURR?
  SYST:DCS:CH2:CURR?
    outputs selected channel current
  SYST:DCS:CH1:OUTP
  SYST:DCS:CH2:OUTP
    toggles selected channel output on/off
  SYST:DCS:CH1:MAX:CURR?
  SYST:DCS:CH2:MAX:CURR?
    outputs the maximum current on selected channel
  SYST:DCS:CH1:MAX:CURR
  SYST:DCS:CH2:MAX:CURR
    sets the maximum current on selected channel (maximum current cannot be set above 1500 mA)
  SYST:DCS:TEMP?
    outputs the current temperature and the maximum temperature
  SYST:DCS:TEMP
    sets the maximum temperature value (maximum temperature cannot be set above 120 degrees F)
  SYST:DCS:POW?
    toggles the input relay on/off
  SYST:DCS:RES
    sets all values back to their initial values and turns off relays
*/

#include "Arduino.h"
#include "Vrekrer_scpi_parser.h" //SCPI parser by Vrecker
#include <Adafruit_INA219.h> 
#include <Wire.h>
#include <LiquidCrystal_I2C.h> //by Frank de Brabander
#include "Adafruit_MCP9808.h"

//Function Declarations
void measure_all();
void check_input();
void check_output1();
void check_output2();
void buck_converter_out1();
void buck_converter_out2();
void write_lcd();

//Setup SCPI Parser
SCPI_Parser my_instrument;

//Setup I2C devices
Adafruit_INA219 ina219_out1(0x40); //Current sensor on output channel 1
Adafruit_INA219 ina219_out2(0x45); //Current sensor on output channel 2
Adafruit_INA219 ina219_in(0x41);   //Current sensor on input
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808(); //Temperature Sensor
LiquidCrystal_I2C lcd(0x27,16,2); //Set the LCD address to 0x27 with 16 char, 2 line display

//Define constants
//Pin definitions
const int CH1 = 9; //controls channel 1 voltage (must be PWM)
const int CH2 = 6; //controls channel 2 voltage  (must be PWM)
const int relay_out1 = 8; //controls channel 1 relay
const int relay_out2 = 12; //controls channel 2 relay
const int relay_in = 7; //controls input relay 
const int dmm_in = A0; //gets voltage at input
const int dmm_out1 = A1; //gets voltage at channel 1
const int dmm_out2 = A2; //gets voltage at channel 2
//A5 is SCL
//A4 is SDA
//Additional constants
const float MAX_TEMP = 120; //absolute maximum temperature in degrees
const float MAX_CURR = 1500; //absolute maximum current (mA)

//Define variables
float var = 0;
//Output variables
float voltage1 = 2;
int output1 = 0; //boolean for if channel 1 is on/off
float voltage2 = 2;
int output2 = 0; //boolean for if channel 2 is on/off
int input = 0; //boolean for input relay
//Measured Currents/Voltages/Temp
float shuntvoltage_out1 = 0;
float busvoltage_out1 = 0;
float current_out1 = 0;
float loadvoltage_out1 = 0;
float shuntvoltage_out2 = 0;
float busvoltage_out2 = 0;
float current_out2 = 0;
float loadvoltage_out2 = 0;
float shuntvoltage_in = 0;
float busvoltage_in = 0;
float current_in = 0;
float loadvoltage_in = 0;
float temp = 0;
//Max values
const float max_current_in = 1000; //mA
float max_current_out1 = MAX_CURR; //mA
float max_current_out2 = MAX_CURR; //mA
float max_temp = MAX_TEMP; //degrees F
//PWM variables
int PWM1;
int PWM2;

void setup() {
  my_instrument.RegisterCommand(F("*IDN?"), &Identify);
  my_instrument.SetCommandTreeBase(F("SYSTem:DCSupply"));
    my_instrument.RegisterCommand(F("CH1:VOLTage"), &SetVoltage1);
    my_instrument.RegisterCommand(F("CH1:VOLTage?"), &GetVoltage);
    my_instrument.RegisterCommand(F("CH1:CURRent?"), &GetOutput);
    my_instrument.RegisterCommand(F("CH2:VOLTage"), &SetVoltage2);
    my_instrument.RegisterCommand(F("CH2:VOLTage?"), &GetVoltage);
    my_instrument.RegisterCommand(F("CH2:CURRent?"), &GetOutput);
    my_instrument.RegisterCommand(F(":OUTput?"), &GetState);
    my_instrument.RegisterCommand(F(":CH1:OUTput?"), &GetOutput);
    my_instrument.RegisterCommand(F(":CH2:OUTput?"), &GetOutput);
    my_instrument.RegisterCommand(F(":CH1:OUTput"), &SetOutput1);
    my_instrument.RegisterCommand(F(":CH2:OUTput"), &SetOutput2);
    my_instrument.RegisterCommand(F("CH1:MAXimum:CURRent?"), &GetCurrent1); 
    my_instrument.RegisterCommand(F("CH2:MAXimum:CURRent?"), &GetCurrent2); 
    my_instrument.RegisterCommand(F("CH1:MAXimum:CURRent"), &SetCurrent1); 
    my_instrument.RegisterCommand(F("CH2:MAXimum:CURRent"), &SetCurrent2); 
    my_instrument.RegisterCommand(F("TEMPerature?"), &GetTemp); 
    my_instrument.RegisterCommand(F("TEMPerature"), &SetTemp); 
    my_instrument.RegisterCommand(F("POWer"), &ON_OFF); 
    my_instrument.RegisterCommand(F("RESet"), &Reset); //20

  Serial.begin(9600);
  while (!Serial) {
    delay(1); //pause until serial console opens
  }
  uint32_t currentFrequency; //might not be needed
  Serial.println("Initializing PC Controlled DC Power Supply");

  //Initialize I2C Sensors
  if (! ina219_out1.begin()) {
    Serial.println("Failed to find INA219 current sensor for channel 1\n");
    //while (1) { delay(10); }
  }
  if (! ina219_out2.begin()) {
    Serial.println("Failed to find INA219 current sensor for channel 2\n");
    //while (1) { delay(10);}
  } 
  if (! ina219_in.begin()) {
    Serial.println("Failed to find INA219 current sensor for input channel\n");
    //while (1) { delay(10);}
  } 
  if (!tempsensor.begin(0x18)) {
    Serial.println("Failed to find MCP9808 temperature sensor\n");
  }
  lcd.init();
  lcd.backlight();

  //Welcome Message
  Serial.println("PC Controlled DC Power Supply is ready.\n");

  //Set Arudino Pins of Input/Output
  pinMode(CH1, OUTPUT);
  pinMode(CH2, OUTPUT);
  pinMode(relay_out1, OUTPUT);
  pinMode(relay_out2, OUTPUT);
  pinMode(relay_in, OUTPUT);
  pinMode(dmm_in, INPUT);
  pinMode(dmm_out1, INPUT);
  pinMode(dmm_out2, INPUT);

  //Set initial values
  analogWrite(CH1, 0);
  analogWrite(CH2, 0);
  digitalWrite(relay_out1, LOW);
  digitalWrite(relay_out2, LOW);
  digitalWrite(relay_in, LOW);
}

void loop() {
  my_instrument.ProcessInput(Serial, "\n");
  measure_all();
  if (input != 0) {
    check_input();
  }
  if (output1 != 0) {
    check_output1();
    buck_converter_out1();
  }
  if (output2 != 0) {
    check_output2();
    buck_converter_out2();
  }
  write_lcd();
}

//Get measurements from current sensors
void measure_all() {
  //measure current sensor out1
  shuntvoltage_out1 = ina219_out1.getShuntVoltage_mV();
  busvoltage_out1 = ina219_out1.getBusVoltage_V();
  current_out1 = ina219_out1.getCurrent_mA();
  loadvoltage_out1 = busvoltage_out1 + (shuntvoltage_out1 / 1000);

  //measure current sensor out2
  shuntvoltage_out2 = ina219_out2.getShuntVoltage_mV();
  busvoltage_out2 = ina219_out2.getBusVoltage_V();
  current_out2 = ina219_out2.getCurrent_mA();
  loadvoltage_out2 = busvoltage_out2 + (shuntvoltage_out2 / 1000);
  
  //measure current sensor out1
  shuntvoltage_in = ina219_in.getShuntVoltage_mV();
  busvoltage_in = ina219_in.getBusVoltage_V();
  current_in = ina219_in.getCurrent_mA();
  loadvoltage_in = busvoltage_in + (shuntvoltage_in / 1000);

  //measure temperature
  tempsensor.wake();
  temp = tempsensor.readTempF();
  tempsensor.shutdown_wake(1); //Shutdown to reduce power consumption to 0.1 uA  
}

//Relay functions
void check_input() {
  if (current_in > max_current_in) {
    digitalWrite(relay_in, LOW);
    digitalWrite(relay_out1, LOW);
    digitalWrite(relay_out2, LOW);
    input = 0;
    output1 = 0;
    output2 = 0;
    Serial.println("System Shutoff.");
    Serial.println("Maximum current exceeded at the input.\n");
  }
  else if (temp > max_temp) {
    digitalWrite(relay_in, LOW);
    digitalWrite(relay_out1, LOW);
    digitalWrite(relay_out2, LOW);
    input = 0;
    output1 = 0;
    output2 = 0;
    Serial.println("System Shutoff.");
    Serial.println("Maximum temperature exceeded.\n");
  }    
}

void check_output1() {
  if (current_out1 > max_current_out1) {
    digitalWrite(relay_out1, LOW);
    output1 = 0;
    Serial.println("Channel 1 Shutoff.");
    Serial.println("Maximum current exceeded on channel 1.\n");
  }
}

void check_output2() {
  if (current_out2 > max_current_out2) {
    digitalWrite(relay_out2, LOW);
    output2 = 0;
    Serial.println("Channel 2 Shutoff.");
    Serial.println("Maximum current exceeded on channel 2.\n");
  }
}

//Buck Converter Functions
void buck_converter_out1() {
  while (abs(dmm_out1 - voltage1) > 0.2) {  
    var = analogRead(dmm_out1);

    if (voltage1 > var) { //If desired voltage is larger than measured voltage
      PWM1 = PWM1 - 1;
      PWM1 = constrain(PWM1, 1, 254);    
    }
    if (voltage1 < var) { //If desired voltage is smaller than measured voltage
      PWM1 = PWM1 + 1;
      PWM1 = constrain(PWM1, 1, 254);    
    }  

    analogWrite(CH1, PWM1);
  }
}

void buck_converter_out2() {
  while (abs(dmm_out1 - voltage2) > 0.2) {
    var = analogRead(dmm_out2);

    if (voltage2 > var) { //If desired voltage is larger than measured voltage
      PWM2 = PWM2 - 1;
      PWM2 = constrain(PWM2, 1, 254);    
    }
    if (voltage2 < var) { //If desired voltage is smaller than measured voltage
      PWM2 = PWM2 + 1;
      PWM2 = constrain(PWM2, 1, 254);    
    }  

    analogWrite(CH2, PWM2);
  }
}

//LCD Function
void write_lcd() {
  lcd.setCursor(0,0);
  lcd.print("CH1 Voltage: " + String(loadvoltage_out1) + "V");
  lcd.setCursor(0,1);
  lcd.print("CH2 Voltage: " + String(loadvoltage_out2) + "V"); 
}

//SCPI functions
void Identify(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println(F("\nPC Controlled Power Supply"));
}

void SetVoltage1(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    var = String(parameters[0]).toFloat();
    if (var >= 2 & var <= 14) {
      voltage1 = var;
    }
    else {
      interface.println(F("\nVoltage must be between 2 and 14."));
    }
  }
}

void SetVoltage2(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    var = String(parameters[0]).toFloat();
    if (var >= 2 & var <= 14) {
      voltage2 = var;
    }
    else {
      interface.println(F("\nVoltage must be between 2 and 14."));
    }
  }
}

void GetOutput(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (output1 == 0) {
    interface.println("\nChannel 1: OFF [" + String(voltage1) + " V]");
  }
  else {
    interface.println("\nChannel 1 Voltage: " + String(loadvoltage_out1) + " V");
    interface.println("Channel 1 Current: " + String(current_out1) + "mA");
  }
  if (output2 == 0) {
    interface.println("Channel 2: OFF [" + String(voltage2) + " V]");
  }
  else {
    interface.println("Channel 2 Voltage: " + String(voltage2) + " V");
    interface.println("Channel 2 Current: " + String(current_out2) + "mA");
  }
}

void GetVoltage(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (output1 == 0) {
    interface.println("\nChannel 1: OFF [" + String(voltage1) + " V]");
  }
  else {
    interface.println("\nChannel 1 Voltage: " + String(loadvoltage_out1) + " V");
  }
  if (output2 == 0) {
    interface.println("Channel 2: OFF [" + String(voltage2) + " V]");
  }
  else {
    interface.println("Channel 2 Voltage: " + String(voltage2) + " V");
  }  
}

void SetOutput1(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (input == 0) {
    interface.println("Power Supply is off.\n Use (SYST:DCS:POW) to turn on.");
  }
  else {
    if (output1 == 0) {
      output1 = 1;
    }
    else {
      output1 = 0;
    }
  }
}

void SetOutput2(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (input == 0) {
    interface.println("Power Supply is off.\n Use (SYST:DCS:POW) to turn on.");
  }
  else {
    if (output2 == 0) {
      output2 = 1;
    }
    else {
      output2 = 0;
    }
  }
}

void GetCurrent1(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println("Channel 1 Maximum Current: " + String(max_current_out1) + " mA");
}

void GetCurrent2(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println("Channel 2 Maximum Current: " + String(max_current_out2) + " mA");
}

void SetCurrent1(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    var = String(parameters[0]).toFloat();
    if (var <= MAX_CURR) {
      max_current_out1 = var;
    }
    else {
      interface.println("Maximum current cannot exceed " + String(MAX_CURR) + "mA");
    }
  }
}

void SetCurrent2(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    var = String(parameters[0]).toFloat();
    if (var <= MAX_CURR) {
      max_current_out2 = var;
    }
    else {
      interface.println("Maximum current cannot exceed " + String(MAX_CURR) + "mA");
    }
  }
}

void GetTemp(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println("Internal Temperature: " + String(temp) + " degrees F");
  interface.println("Maximum Temperature: " + String(max_temp) + " degrees F");
}

void SetTemp(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    var = String(parameters[0]).toFloat();
    if (var <= MAX_TEMP) {
      max_temp = var;
    }
    else {
      interface.println("Maximum temperature cannot exceed " + String(MAX_TEMP) + "degrees F");
    }
  }
}

void ON_OFF(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (input == 0) { //if off turn input relay on
    input = 1;
  }
  else { //if on turn everything off
    input = 0;
    output1 = 0;
    output2 = 0;
  }
}

void GetState (SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (input == 0) {
    interface.println("\nPower Supply is off.");
  }
  else {
    interface.println("\nInput Voltage: " + String(loadvoltage_in) + " V");
    interface.println("Input Current: " + String(current_in) + " mA");
    if (output1 == 0) {
      interface.println("\nChannel 1: OFF [" + String(voltage1) + " V]");
    }
    else {
      interface.println("\nChannel 1 Voltage: " + String(loadvoltage_out1) + " V");
      interface.println("Channel 1 Current: " + String(current_out1) + "mA");
    }
    if (output2 == 0) {
      interface.println("Channel 2: OFF [" + String(voltage2) + " V]");
    }
    else {
      interface.println("Channel 2 Voltage: " + String(voltage2) + " V");
      interface.println("Channel 2 Current: " + String(current_out2) + "mA");
    }
  }

  //Print temperature
  interface.println("Internal Temperature: " + String(temp) + " degrees F");  
}

void Reset (SCPI_C commands, SCPI_P parameters, Stream& interface) {
  float voltage1 = 2; //Set channel 1 voltage to 2V
  int output1 = 0; //Turn off channel 1
  float voltage2 = 2; //Set channel 2 voltage to 2V
  int output2 = 0; //Turn off channel 2
  int input = 0; //Turn off input relay
  
  //Reset maximum values
  float max_current_out1 = MAX_CURR; //1500 mA
  float max_current_out2 = MAX_CURR; //1500 mA
  float max_temp = MAX_TEMP; //120 degrees F
}
