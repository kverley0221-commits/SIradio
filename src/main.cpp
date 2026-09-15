#include <Arduino.h>
#include "SIradio.h"

// SIradio(sdnPin, csPin, nirqPin)
SIradio radio(15, 17, 14);

String msgStr;

void setup()
{
  Serial.begin(115200);
  delay(10000); // allow time to open the serial monitor

  radio.begin();
  radio.radio_RX_Mode();
}

void loop()
{
  if (Serial.available() != 0)
  {
    msgStr = Serial.readString();
    msgStr.trim();

    Serial.print("Sending: ");
    Serial.println(msgStr);

    if (radio.sendMessage(msgStr))
    {
      Serial.println("Packet sent");
      delay(20);
    }
    else
      Serial.println("Packet was not sent");
    radio.radio_RX_Mode();
  }

  if (radio.check_Received_Packet())
    radio.printMsg();
}