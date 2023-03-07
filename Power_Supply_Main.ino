/*
PC Controlled Power Supply
Uses SCPI parser by Vrekrer

Hardware:
3 INA219 current sensors
  /The module has four I2C, these addresses are:
  INA219_I2C_ADDRESS1 0x40 //A0 = 0 A1 = 0
  INA219_I2C_ADDRESS2 0x41 //A0 = 1 A1 = 0
  INA219_I2C_ADDRESS3 0x44 //A0 = 0 A1 = 1
  INA219_I2C_ADDRESS4 0x45 //A0 = 1 A1 = 1
  /
1 Adafruit temperature sensor
1 LCD


SCPI Commands Supported:
  *IDN?
    identifies instrument
  SYST:DCS:CH1:VOLT <value>
  SYST:DCS:CH2:VOLT <value>
    sets output voltage of selected channel, valid for [2,14] V
  SYST:DCS:CH1:VOLT?
  SYST:DCS:CH2:VOLT?
    outputs channel voltages
  SYST:DCS:CH1:OUTP
  SYST:DCS:CH2:OUTP
    toggles given channel output on/off
  SYST:DCS:OUTP?
  SYST:DCS:CH1:OUTP?
  SYST:DCS:CH2:OUTP?
    outputs channel voltages
*/

#include "Arduino.h"
#include "Vrekrer_scpi_parser.h"
#include <Adafruit_INA219.h> //by Frank de Brabander
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

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

//Define constants
const int CH1 = 9; //controls channel 1 voltage (must be PWM)
const int CH2 = 6; //controls channel 2 voltage  (must be PWM)
const int relay_out1 = 8; //controls channel 1 relay
const int relay_out2 = 12; //controls channel 2 relay
const int relay_in = 7; //controls input relay 
const int dmm_in = A0; //gets voltage at input
const int dmm_out1 = A1; //gets voltage at channel 1
const int dmm_out2 = A2; //gets voltage at channel 2

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
float max_current_in = 1000; //mA
float max_current_out = 1500; //mA
float max_temp = 120; //degrees F

//constants defined for LCD
int h;
int m;
int s;
int sc;
int mc;
int flag;
int TIME;
const int hs=8;
const int ms=9;
int state1;
int state2;
String vr="Your desired voltage is ";


void setup() {
  my_instrument.RegisterCommand(F("*IDN?"), &Identify);
  my_instrument.SetCommandTreeBase(F("SYSTem:DCSupply"));
    my_instrument.RegisterCommand(F("CH1:VOLTage"), &SetVoltage1);
    my_instrument.RegisterCommand(F("CH1:VOLTage?"), &GetOutput);
    my_instrument.RegisterCommand(F("CH2:VOLTage"), &SetVoltage2);
    my_instrument.RegisterCommand(F("CH2:VOLTage?"), &GetOutput);
    my_instrument.RegisterCommand(F(":OUTPut?"), &GetOutput);
    my_instrument.RegisterCommand(F(":CH1:OUTPut?"), &GetOutput);
    my_instrument.RegisterCommand(F(":CH2:OUTPut?"), &GetOutput);
    my_instrument.RegisterCommand(F(":CH1:OUTPut"), &SetOutput1);
    my_instrument.RegisterCommand(F(":CH2:OUTPut"), &SetOutput2);

  Serial.begin(9600);
  while (!Serial) {
    delay(1); //pause until serial console opens
  }
  uint32_t currentFrequency; //might not be needed
  Serial.println("Initializing PC Controlled DC Power Supply");

  //Initialize Current Sensors
  if (! ina219_out1.begin()) {
    Serial.println("Failed to find INA219 chip for channel 1\n");
    while (1) { delay(10); }
  }
  if (! ina219_out2.begin()) {
    Serial.println("Failed to find INA219 chip for channel 2\n");
    while (1) { delay(10);}
  } 
  if (! ina219_in.begin()) {
    Serial.println("Failed to find INA219 chip for input channel\n");
    while (1) { delay(10);}
  } 

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
  if (current_out1 > max_current_out) {
    digitalWrite(relay_out1, LOW);
    output1 = 0;
    Serial.println("Channel 1 Shutoff.");
    Serial.println("Maximum current exceeded on channel 1.\n");
  }
}

void check_output2() {
  if (current_out2 > max_current_out) {
    digitalWrite(relay_out2, LOW);
    output2 = 0;
    Serial.println("Channel 2 Shutoff.");
    Serial.println("Maximum current exceeded on channel 2.\n");
  }
}

//Buck Converter Functions
void buck_converter_out1() {
  
}

void buck_converter_out2() {

}

//LCD Functions
void write_lcd() {
    /********CHANNEL 1 STUFF******/
  //take in the voltage if channel 1
  //h=0;
  lcd.setCursor(0,0);
  lcd.print("CH1 VALUE-(OoO)-");
//   while(Serial.available()==0){
    
//   }

//   //take in the channel 1 value and displaying it to the screen
  lcd.setCursor(10,0);
  s=dmm_out1;
  //Serial.println(vr+s);
  
  //error handling for too high channel 1 voltage
  if(s>14){
    //lcd.clear();
    //Serial.println("Voltage is too high");    
    lcd.setCursor(0,0);
    lcd.print("CH1 RISKY(0o0)  ");
    lcd.setCursor(0,0);
  }
  //error handling for too low channel 1 voltage
  else if(s<2){
    //Serial.println("Voltage is too low");    
    lcd.setCursor(0,0);
    lcd.print("CH1 SMALL(-_-)  ");
  }

  /********CHANNEL 2 STUFF******/
  //takes in the voltage of channel 2
    
  }
  //lcd.setCursor(10,0);
  m=dmm_out2;
 // Serial.println(vr+m);

  //error handling for too high channel 2 voltage
  if(m>14){
   //Serial.println("Voltage is too high");    
   lcd.setCursor(0,1);
   lcd.print("CH2 RISKY(0o0)  ");
   lcd.setCursor(0,1);
  }
  //error handling for too low channel 2 voltage
  else if(m<2){
    //Serial.println("Voltage is too low");   
    lcd.setCursor(0,0);
    lcd.print("CH2 SMALL(-_-)  ");
  }
  delay(1000);
  h=0;
  
  //final value display print
  while(h!=1 && s>0 && m>0){
    lcd.clear();  
  if(s>14){//error handling for too high channel 1 voltage
    lcd.setCursor(10,0);
    lcd.print("HIGH   ");
    lcd.setCursor(0,0);
    lcd.print("Channel 1:");
  }
  else if(s<2){//error handling for too low channel 1 voltage
    lcd.setCursor(10,0);
    lcd.print("LOW    ");
    lcd.setCursor(0,0);
    lcd.print("Channel 1:");
  }
  else{//print for voltage within range of 2V to 14V 
    lcd.setCursor(0,0);
    lcd.print("Channel 1:");
    lcd.setCursor(15,0);
    lcd.print("V");
    lcd.setCursor(10,0);
    lcd.print("     ");
    lcd.setCursor(13,0);    
    lcd.print(s);    
  }
  
//channel 2 if statements
  if(m>14){//error handling for too high channel 2 voltage
    lcd.setCursor(10,1);
    lcd.print("HIGH    ");
    lcd.setCursor(0,1);
    lcd.print("Channel 2:");
  }
  else if(m<2){//error handling for too high channel 2 voltage
    lcd.setCursor(10,1);
    lcd.print("LOW    ");
    lcd.setCursor(0,1);
    lcd.print("Channel 2:");
  }
  else{//print for voltage within range of 2V to 14V
    lcd.setCursor(0,1);
    lcd.print("Channel 2:");
    lcd.setCursor(15,1);
    lcd.print("V");
    lcd.setCursor(10,1);
    lcd.print("     ");
    lcd.setCursor(13,1);    
    lcd.print(m);    
  }
  //Serial.println("Would you like to enter a new set of values? Enter 0");
  //while(Serial.available()==0){
    
  //}
  //channel 1 value
  lcd.setCursor(10,0);
  //h=Serial.parseInt();
  if(h==0){
    lcd.clear();
    h=1;
  }
  }
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
      if (output1 == 1){
        analogWrite(CH1, 255*((voltage1-2)/12));
      }
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
      if (output2 == 1){
        analogWrite(CH2, (voltage2-2)/255);
      }
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
    interface.println("\nChannel 1: " + String(voltage1) + " V");
  }
  if (output2 == 0) {
    interface.println("Channel 2: OFF [" + String(voltage2) + " V]");
  }
  else {
    interface.println("Channel 2: " + String(voltage2) + " V");
  }
}

void SetOutput1(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (output1 == 0) {
    output1 = 1;
    analogWrite(CH1, (voltage1-2)/255);
  }
  else {
    output1 = 0;
    analogWrite(CH1, 0);
  }
}

void SetOutput2(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (output2 == 0) {
    output2 = 1;
    analogWrite(CH2, (voltage2-2)/255);
  }
  else {
    output2 = 0;
    analogWrite(CH2, 0);
  }
}
