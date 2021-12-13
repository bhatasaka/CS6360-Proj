#include <WiFi.h>
#include "params.h"
#include "VDivider.h"

TaskHandle_t SocketServerTask;
struct electrodeControl_s
{
  volatile uint8_t setVoltage;
};

volatile bool enableElectrodeG = false;
volatile uint8_t setVoltageG = 0;
volatile uint16_t pulseWidthG = 0;
volatile uint16_t pulseFreqG = electrodeInitFreq;

struct electrodeControl_s electrodeControl;

void setup()
{
  Serial.begin(115200);
  setupBoost();
  setupElectrodes();

  electrodeControl.setVoltage = 0;
  setVoltageG = 0;
 
  delay(800);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());

  xTaskCreatePinnedToCore(
                    SocketServer,       /* Task function. */
                    "SocketServer",     /* name of task. */
                    10000,              /* Stack size of task */
                    NULL,               /* parameter of the task */
                    1,                  /* priority of the task */
                    &SocketServerTask,  /* Task handle to keep track of created task */
                    0);                 /* pin task to core 0 */
}

void loop()
{
  loopBoost();
  loopElectrodes();

  delay(1);
}


unsigned int currentBoostDuty;
VDivider boostDivider(boostVSenseR1, boostVSenseR2);
void setupBoost()
{
  ledcAttachPin(boostPin, boostChannel);
  ledcSetup(boostChannel, boostInitFreq, boostPWMRes);
  ledcWrite(boostChannel, boostDuty);
  currentBoostDuty = boostDuty;

  // Setup analog stuff
  pinMode(boostVSensePin, INPUT);
  analogSetClockDiv(64); // Slow down readings to try to get it more stable
  analogSetWidth(10); // 10-bit readings
}

// Checks if the voltage is below the threshold and alters the pwm accordingly.
// Checks if the voltage is above the threshold and dramatically alters the pwm accordingly.
void loopBoost()
{
  auto adc = analogRead(boostVSensePin);
  
  // Serial.print("adc: ");
  // Serial.println(adc);

  auto avg = boostDivider.getSrcVoltage(adc);
  unsigned int newDuty = boostDuty;

  if(avg > boostUpperThresh)
    newDuty = boostDutyLowV;
  else if (avg < boostLowerThresh && avg > boostLowestThresh)
    newDuty = boostDutyHighV;
  else if (avg < boostLowestThresh)
    newDuty = boostDutySuperV;
  
  if(newDuty != currentBoostDuty)
  {
    ledcWrite(boostChannel, newDuty);
    currentBoostDuty = newDuty;
  }
  // Serial.print("avg: ");
  // Serial.println(avg);
  // delay(500);
}


VDivider eVoltDivider(eVoltR1, eVoltR2);
void setupElectrodes()
{
  ledcAttachPin(electrodePin, electrodeChannel);
  ledcSetup(electrodeChannel, pulseFreqG, electrodePWMRes);
  ledcWrite(electrodeChannel, 0); // Initialize it to 0 duty cycle
  
  digitalWrite(eVoltControlPin, LOW);
  pinMode(eVoltControlPin, OUTPUT);
}

// Regulates the voltage the electrodes will be applying to the muscles.
// 
// Check the set voltage and read the capacitor voltage
// If it's too low, then turn on the control pin,
// otherwise turn off the control pin
void loopElectrodes()
{
  auto adc = analogRead(eVoltSensePin);
  auto currentV = eVoltDivider.getSrcVoltage(adc);
  // if(electrodeControl.setVoltage > currentV)
  if(setVoltageG > currentV)
    digitalWrite(eVoltControlPin, HIGH);
  else
    digitalWrite(eVoltControlPin, LOW);

  
  // Serial.print("adc: ");
  // Serial.println(adc);
  // Serial.print("currentV: ");
  // Serial.println(currentV);
  // delay(500);
}

void disableElectrodes()
{
  ledcWrite(electrodeChannel, 0);
}

WiFiClient client;
WiFiServer Server(serverPort);
const uint32_t bufSize = 32;

enum MsgStates
{
  MSG_waitingForStart,
  MSG_reading
};

uint8_t msgState = MSG_waitingForStart;

void SocketServer(void *pvParams)
{
  pinMode(2, OUTPUT);
  uint32_t msgIdx = 0;
  uint8_t socketBuf[bufSize];
  uint8_t msgBuf[bufSize * 2];

  Server.begin();
  while(true)
  {
    CheckForConnections();
    if(client.available()) {
      int len = client.read(socketBuf, bufSize);

      for(int i = 0; i < len; i++)
      {
        switch(msgState)
        {
          case MSG_waitingForStart:
            if(socketBuf[i] == '+')
              msgState = MSG_reading;
            break;
          
          case MSG_reading:
          {
            if(socketBuf[i] == ';')
            {
              msgBuf[msgIdx] = '\0';
              ParsePacket((char *)msgBuf, msgIdx);
              // Msg ending character
              msgIdx = 0;
              msgState = MSG_waitingForStart;
            }
            else if(msgIdx < bufSize * 2)
              msgBuf[msgIdx++] = socketBuf[i];
            else
            {
              Serial.println("Message buffer overlflowed");
              msgState = MSG_waitingForStart;
              msgIdx = 0;
            }
            break;
          }

          default:
            msgState = MSG_waitingForStart;
        }
      } 
    }
    // If we get disconnected, disable electrodes for safety
    if(!client.connected())
      disableElectrodes();

    delay(1); // calls FreeRTOS vTaskDelay to feed the watchdog
  }
}

void CheckForConnections()
{
  if (Server.hasClient())
  {
    // If we are already connected to another computer, 
    // then reject the new connection. Otherwise accept
    // the connection. 
    if (client.connected())
    {
      Serial.println("Connection rejected");
      Server.available().stop();
    }
    else
    {
      Serial.println("Connection accepted");
      client = Server.available();
    }
  }
}

inline char * ParseTok(char *buf)
{
  return strtok(buf, " ");
}

enum ParseState
{
  P_EEnable,
  P_EVoltage,
  P_EWidth,
  P_EFreq,
  P_Done
};

// Packet consists of 1-4 integers separated by spaces.
// Packet starts with a '+' character and ends with a ';'
//
// 1: 1 or 0 - pulse or don't pulse
// 2: Voltage - 0-49 volts
// 3: pulse width - 40-600 us
// 4: frequency - 30-50 hz
//
// Example packet:
// +1 37 400;
// Will set the pulse to on at 37 volts and a 400us duration
void ParsePacket(char *packet, int size)
{
  ParseState parseState = P_EEnable;
  packet = ParseTok(packet);
  while(true)
  {
    if(packet == nullptr)
      return;

    auto len = strlen(packet);

    // Skip this param if we get an '*' character
    if(strncmp("*", packet, len) == 0)
    {
      packet = ParseTok(nullptr);
      continue;  
    }

    int packetNum = ParseNumber(packet, len);
    if(packetNum != -1)
    {
      Serial.println(packetNum);

      switch(parseState)
      {
        case P_EEnable:
          if(packetNum == 0)
          {
            disableElectrodes();
            enableElectrodeG = false;
            Serial.println("Disabled electrodes");
          }
          else if (packetNum == 1)
          {
            enableElectrodeG = true;
            ledcWrite(electrodeChannel, pulseWidthG);
            Serial.println("Enabled electrodes");
          }

          parseState = P_EVoltage;
          break;

        case P_EVoltage:
          if(packetNum < 50) // Cap at 50v (which is actually around 57v)
          {
            // electrodeControl.setVoltage = packetNum;
            setVoltageG = packetNum;
            Serial.println("Set new voltage");
          }
          
          parseState = P_EWidth;
          break;

        case P_EWidth:
          if(packetNum >= 40 && packetNum <= 600)
          {
            pulseWidthG = packetNum/electrodePWMDutyCalc;
            Serial.print("Set new duty to ");
            Serial.println(packetNum/electrodePWMDutyCalc);
          }
          parseState = P_EFreq;
          break;

        case P_EFreq:
          if(packetNum >= 30 && packetNum <= 50)
          {
            pulseFreqG = packetNum;
            ledcSetup(electrodeChannel, packetNum, electrodePWMRes);
            if(enableElectrodeG)
              ledcWrite(electrodeChannel, pulseWidthG);
            Serial.println("Set new freq");
          }
          parseState = P_Done;
        break;

        default:
          Serial.println("Weird parse state, returning...");
          return;
      }
    }
    else
    {
      Serial.println("bad packet");
      return;
    }

    packet = ParseTok(nullptr);
  }
}

// Parse an unsigned number that is less than 30 digits
// returns -1 when the character array contains non-number ascii
// buf needs to be null terminated
int32_t ParseNumber(char * buf, int len)
{
  // Length between 0 and 30
  if(len <= 0 || len > 30)
    return -1;

  for(int i = 0; i < len; i++)
  {
    if (buf[i] < '0' || buf[i] > '9')
    {
      Serial.println("bad chars");
      return -1;
    }
  }

  return atoi((char *)buf);
}