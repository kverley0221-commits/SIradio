#include <Arduino.h>
#include "SIradio.h"

// SIradio(sdnPin, csPin, nirqPin)
SIradio radio(15, 17);

String msgStr;

void setup()
{
  Serial.begin(115200);
  delay(10000); // allow time to open the serial monitor

  radio.begin() ? Serial.println("Radio config completed") : Serial.println("Radio config fialed");
  radio.rxMode();
}

void loop()
{
//   if (Serial.available() != 0)
//   {
//     msgStr = Serial.readString();
//     msgStr.trim();

//     Serial.println("Sending: " + msgStr);
//     radio.sendPacket(msgStr);
//     bool int_status = radio.packetSent();
//     while (!int_status)
//     {
//       int_status = radio.packetSent();
//       delayMicroseconds(100);
//     }
//     int_status ? Serial.println("Packet sent") : Serial.println("Packet not sent");
//     // radio.radio_RX_Mode();
//   }

  while (!radio.packetReceived())
    ;
  Serial.print("Packet Received: ");
  radio.getMsg(msgStr);
  Serial.println(msgStr);
  msgStr = "";
  // if (radio.check_Received_Packet())
  //   radio.printMsg();
}