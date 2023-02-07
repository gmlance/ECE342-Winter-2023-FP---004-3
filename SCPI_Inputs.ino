/*
PC Controlled Power Supply SCPI Commands Proof of Concept
Uses SCPI parser by Vrekrer

Hardware:
resistor from pin 9 to 13 to use built-in LED
resistor and LED connected to pin 6 and gnd

Commands:
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

SCPI_Parser my_instrument;
const int CH1 = 9;
const int CH2 = 6;
float voltage1 = 2;
int output1 = 0;
float voltage2 = 2;
int output2 = 0;
float var = 0;

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
  pinMode(CH1, OUTPUT);
  pinMode(CH2, OUTPUT);
  pinMode(LED_BUILTIN, INPUT);
  analogWrite(CH1, 0);
  analogWrite(CH2, 0);
}

void loop() {
  my_instrument.ProcessInput(Serial, "\n");
}

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
