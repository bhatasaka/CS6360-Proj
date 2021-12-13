using System.Collections;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

public class Lifter
{
    private int enable;

    public Lifter()
    {
        Enable = false;
        Voltage = 0;
        PulseWidth = 0;
        Frequency = 0;
    }

    public bool Enable
    {
        get => enable != 0;
        set => enable = value ? 1 : 0; 
    }
    public int Voltage { get; set; }
    public int PulseWidth { get; set; }
    public int Frequency { get; set; }

    public string BuildFulllMessage()
    {
        return $"+{enable} {Voltage} {PulseWidth} {Frequency};";
    }

    public string BuildEnableMessage()
    {
        return $"+{enable};";
    }

    /// <summary>
    /// Sets the percentage of the maximum application power.
    /// Handles the logic to set the proper parameters for a comfortable feel.
    /// </summary>
    /// <param name="percentage"></param>
    public void SetPercentage(float percentage)
    {

    }
}
