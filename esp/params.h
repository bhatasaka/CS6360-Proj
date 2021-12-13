#pragma once

// ======== Network ===========
const char* ssid = "McLaren";
const char* password = "Aventador";
const uint serverPort = 6360;

// ======== Boost ============
const int boostPin = 13;
const int boostInitFreq = 50000;
const int boostChannel = 1;
const unsigned int boostPWMRes = 10;
// out of 1023, 810 gives around 70-80 volts
const unsigned int boostDuty = 810;
const unsigned int boostDutyHighV = 760; // lower duty, higher voltage
const unsigned int boostDutySuperV = 700; // lower duty, higher voltage
const unsigned int boostDutyLowV = 930; // Higher duty, lower voltage

const int boostVSensePin = 33;
const uint32_t boostVSenseR1 = 940;
const uint32_t boostVSenseR2 = 32;
const uint32_t boostUpperThresh = 82;
const uint32_t boostLowerThresh = 60;
const uint32_t boostLowestThresh = 48;


// Analog notes:
/*
 * The maximum voltage it's able to sense seems to be about 3.1V.
 * The minimum voltage is about 0.15V
 */

// ======== Electrodes ============
const int electrodePin = 14;
// const int electrodeInitFreq = 50;
const int electrodeInitFreq = 30;
const int electrodeChannel = 2;
const unsigned int electrodePWMRes = 12;
// taking a microsecond value and dividing it by this gives the duty cycle
// const float electrodePWMDutyCalc = (((double)1/electrodeInitFreq) / 4096) * 10e5;
// const float electrodePWMDutyCalc = 4.8828125; // 50 hz
const float electrodePWMDutyCalc = 8.138020833; // 30 hz


// ======== Electrode Voltage ========
const int eVoltControlPin = 12;
const int eVoltSensePin = 32;
const uint32_t eVoltR1 = 570;
const uint32_t eVoltR2 = 22;
