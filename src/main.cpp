#include <Arduino.h>
#include "SIradio.h"

// SIradio(sdnPin, csPin, nirqPin)
SIradio radio(15, 17, 14);

String msgStr;

void setup()
{
  Serial.begin(115200);
  delay(10000); // allow time to open the serial monitor

  radio.begin() ? Serial.println("Radio config completed") : Serial.println("Radio config fialed");
}

void loop()
{
  if (Serial.available() != 0)
  {
    msgStr = Serial.readString();
    msgStr.trim();

    Serial.println("Sending: " + msgStr);
    radio.sendPacket(msgStr);
    uint16_t j = 0;
    bool int_status = radio.packetSent();
    while (!int_status && (j < 500))
    {
      int_status = radio.packetSent();
      delayMicroseconds(10);
    }
    int_status ? Serial.println("Packet sent") : Serial.println("Packet not sent");
    // radio.radio_RX_Mode();
  }

  // if (radio.check_Received_Packet())
  //   radio.printMsg();
}