#include <Arduino.h>
#include "SIradio.h"

// SIradio(sdnPin, csPin, nirqPin)
SIradio radio(15, 17, 14);

String msgStr;

void setup()
{
  Serial.begin(115200);
  delay(10000); // allow time to open the serial monitor

  if (!radio.begin())
  {
    Serial.println("Radio initialization failed!");
  }
  else
  {
    Serial.println("Radio initialized. You may now enter your messages.");
  }
}

void loop()
{
  // --- TX path: if the user typed something, send it ---
  radio.radio_RX_Mode();
  delay(5);
  if (Serial.available() != 0)
  {
    msgStr = Serial.readString();
    msgStr.trim();

    if (msgStr.length() > 0)
    {
      Serial.print("Sending: ");
      Serial.println(msgStr);

      if (radio.sendMessage(msgStr))
      {
        Serial.println("Packet sent");
        delay(20);
      }
      else
        Serial.println("Packet was not sent");
    }
  }
  // --- RX path: no outgoing message, so listen instead ---
  else
  {
    // radio.radio_RX_Mode();
    // delay(10);

    if (radio.check_Received_Packet())
    {
      Serial.print("Received: ");
      radio.printMsg();
    }
  }
}