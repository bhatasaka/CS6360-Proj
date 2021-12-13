/*
 * Header only implementation to do voltage divider calculations.
 *
 * Author: Bryan Hatasaka
 */

#pragma once

class VDivider
{
public:
    /// Createa a voltage divider with R1 being the resistance connected to 
    /// the source voltage and R2 being the lower, measured resistance, 
    /// connected to GND.
    /// 
    ///  -+- Source V
    ///   |
    ///   |
    ///  <R1>
    ///   |
    ///   + - - - +  ADC Measurement
    ///   |       |
    ///  <R2>     |
    ///   |       |
    ///   |       |
    ///  -+-------+ GND
    ///  
    VDivider(uint32_t R1, uint32_t R2)
    {
        vDivider = (((double)R1 + R2) / R2) * fixedFac;
        vOut = adcMaxV * vDivider / adcRes;
    }

    /// Returns an average source voltage of the last 3 ADC measurements, given
    /// a 10-bit ADC measurement for the voltage measured across R2.
    int getSrcVoltage(uint16_t adc)
    {
        // calculate current voltage
        int sum = (adc * vOut) / fixedFac;
        // Store newest value and then sum preivous 2 values
        vHist[vAvgIdx] = sum;
        sum += vHist[(vAvgIdx + 1) % histLen];
        sum += vHist[(vAvgIdx + 2) % histLen];

        // The history is stored by looping through the array backwards
        vAvgIdx = (vAvgIdx == 0) ? 2 : vAvgIdx - 1;

        return sum / histLen;
    }

private:
    // Voltage calcualtions are done with fixed-point
    static const uint32_t fixedFac = 1e6;
    static const uint32_t adcRes = 1024;
    static constexpr float adcMaxV = 3.1;

    // used for 3 point averages
    static const uint32_t histLen = 3;
    int vHist[3];
    uint8_t vAvgIdx = histLen-1;
    uint32_t vOut, vDivider;
};