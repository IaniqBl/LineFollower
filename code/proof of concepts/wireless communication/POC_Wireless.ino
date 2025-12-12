#include "SerialCommand.h"
#include <SoftwareSerial.h>

#define SerialPort Serial
#define Baudrate 115200
#define BTBaudrate 19200

SoftwareSerial BTSerial(7, 8);   // RX, TX

SerialCommand sCmdUSB(SerialPort);
SerialCommand sCmdBT(BTSerial);

void setup() {
  SerialPort.begin(Baudrate);
  BTSerial.begin(BTBaudrate);

  // USB commando
  sCmdUSB.addCommand("test", [](char* cmd){ handleTest(SerialPort); });

 // BT commando
  sCmdBT.addCommand("test", [](char* cmd){ handleTest(BTSerial); });

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  sCmdUSB.readSerial();
  sCmdBT.readSerial();
}

void handleTest(Stream &port) {
  port.println("TEST");
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}
