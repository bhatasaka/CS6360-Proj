using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LifterManager : MonoBehaviour
{
    public Bar bar;

    const float margin = 0.05f;
    const float maxHeight = 0.8f;

    public int Voltage = 20;
    public int PulseWidth = 400;
    public int Frequency = 30;

    public string IP = "192.168.0.231";
    public int Port = 6360;

    public bool ShockEnabled { get => shockEnable; }

    private Lifter lifter;
    private AsynchronousClient client;

    PlayerControls controls;
    bool shockEnable = false;
    float zeroPos;
    float prevPercentSend;
    float controllerY;


    // Start is called before the first frame update
    void Awake()
    {
        Voltage = 20;
        PulseWidth = 400;
        Frequency = 30;

        client = new AsynchronousClient(port: this.Port, IP: this.IP);
        client.StartClient();

        lifter = new Lifter();

        controls = new PlayerControls();
        controls.Gameplay.ToggleShock.performed += (ctx) => ToggleShock();
        controls.Gameplay.Move.performed += (ctx) => controllerY = ctx.ReadValue<Vector3>().y;
    }

    // Update is called once per frame
    void Update()
    {
        if (!shockEnable)
            return;

        // Calculate how far the controller is above the zero position as a percentage
        // of the maxHeight. Clamp it between 0 and 1
        var relHeight = Mathf.Clamp((controllerY - zeroPos) / maxHeight, 0, maxHeight);

        // Only send network updates if we've changed more than a margin
        if (Mathf.Abs(relHeight - prevPercentSend) > margin)
        {
            //send message
            prevPercentSend = relHeight;

            if (client.Connection == AsynchronousClient.ConnectionStatus.NotConnected)
            {
                client.StartClient();
                Debug.Log("Reconnecting...");
            }
            else if (client.Connection == AsynchronousClient.ConnectionStatus.Connected)
            {
                lifter.Voltage = Voltage;
                lifter.PulseWidth = PulseWidth;
                lifter.Frequency = Frequency;
                client.Send(lifter.BuildFulllMessage());
            }
        }

        // Set the bar height
        bar.SetHeight(relHeight);
    }

    void ToggleShock()
    {
        shockEnable = !shockEnable;
        if (shockEnable)
        {
            bar.Zero();
            zeroPos = controllerY;
            prevPercentSend = 0;
            lifter.Enable = true;
        }
        else
        {
            lifter.Enable = false;
            client.Send(lifter.BuildEnableMessage());
        }
    }

    void OnEnable()
    {
        controls.Gameplay.Enable();
    }

    void OnDisable()
    {
        controls.Gameplay.Disable();
    }
}
