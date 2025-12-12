#include "SerialCommand.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>

#define SerialPort Serial
#define Baudrate 115200
#define BTBaudrate 19200

bool run;

volatile bool KnopGedrukt = false;
volatile unsigned long lastInterruptTime = 0;

SoftwareSerial BTSerial(7, 8);   // RX, TX

SerialCommand sCmdUSB(SerialPort);
SerialCommand sCmdBT(BTSerial);

unsigned long previous, calculationTime;

struct param_t {
  unsigned long cycleTime;
  int power;
  float kp, ki, kd, diff;
} params;

void onButton() {
  unsigned long now = millis();
  if (now - lastInterruptTime > 150) {   // debounce
    KnopGedrukt = true;
}
lastInterruptTime = now;
}
void setup() {
  SerialPort.begin(Baudrate);
  BTSerial.begin(BTBaudrate);

  pinMode(2, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(2), onButton, FALLING);

  // USB commando’s
  sCmdUSB.addCommand("set", [](char* cmd){ handleSet(sCmdUSB, SerialPort); });
  sCmdUSB.addCommand("debug", [](char* cmd){ handleDebug(SerialPort); });
  sCmdUSB.setDefaultHandler([](char* cmd){ onUnknown(SerialPort, cmd); });

  // BT commando’s
  sCmdBT.addCommand("set", [](char* cmd){ handleSet(sCmdBT, BTSerial); });
  sCmdBT.addCommand("run", onRun);
  sCmdBT.addCommand("stop", onStop);
  sCmdBT.addCommand("debug", [](char* cmd){ handleDebug(BTSerial); });
  sCmdBT.setDefaultHandler([](char* cmd){ onUnknown(BTSerial, cmd); });

  EEPROM.get(0, params);
  SerialPort.println("EEPROM geladen:");
}

void loop() {
  if (KnopGedrukt) {
    KnopGedrukt = false;
     if (run) {
       onStop();
      } 
      else {
       onRun();
      }
  }

  sCmdUSB.readSerial();
  sCmdBT.readSerial();

  unsigned long current = micros();
  if (current - previous >= params.cycleTime) {
    previous = current;
    unsigned long current = micros();
    //Code
    static bool ledState = false;

    if (run) {
        ledState = !ledState;                 // toggle elke cyclus
        digitalWrite(LED_BUILTIN, ledState);
    } else {
        digitalWrite(LED_BUILTIN, LOW);
    }
    //
    unsigned long difference = micros() - current;
    if (difference > calculationTime) calculationTime = difference;
  }
}

// handlers //
void onUnknown(Stream &port, char *command) {
  port.print("unknown command: \"");
  port.print(command);
  port.println("\"");
}

void onRun() {
  run = true;
  digitalWrite(LED_BUILTIN, HIGH); // LED uit
  BTSerial.println("run mode activated");
}

void onStop() {
  run = false;
  digitalWrite(LED_BUILTIN, LOW);   // LED meteen uit
  BTSerial.println("stop mode activated");
  SerialPort.println("stop mode activated");
}

void handleSet(SerialCommand &cmd, Stream &port) {
  char* param = cmd.next();
  char* value = cmd.next();

  if (!param || !value) return;

  if (strcmp(param, "cycle") == 0)
  {
    long newCycleTime = atol(value);
    float ratio = ((float) newCycleTime) / ((float) params.cycleTime);

    params.ki *= ratio;
    params.kd /= ratio;

    params.cycleTime = newCycleTime;
  }
  if (strcmp(param, "ki") == 0)
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.ki = atof(value) * cycleTimeInSec;
  }
  if (strcmp(param, "kd") == 0)
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.kd = atof(value) / cycleTimeInSec;
  }
  if (strcmp(param, "power") == 0)    params.power = atol(value);
  if (strcmp(param, "diff") == 0)    params.diff = atof(value);
  if (strcmp(param, "kp") == 0)    params.kp = atof(value);

  EEPROM.put(0, params);
  port.println("OK parameter updated");
}

void handleDebug(Stream &port) {
  port.print("cycle time: "); port.println(params.cycleTime);
  port.print("power: "); port.println(params.power);
  port.print("diff: "); port.println(params.diff);

  float cycleTimeInSec = ((float) params.cycleTime) / 1000000;

  port.print("Kp: "); port.println(params.kp);

  float ki = params.ki / cycleTimeInSec;
  port.print("Ki: "); port.println(ki);

  float kd = params.kd * cycleTimeInSec;
  port.print("Kd: "); port.println(kd);

  port.print("calculation time: "); port.println(calculationTime);
  calculationTime = 0;
}
