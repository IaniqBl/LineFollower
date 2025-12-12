#include "SerialCommand.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>

#define SerialPort Serial
#define Baudrate 115200
#define BTBaudrate 19200

// DRV8833 motor control pins
#define MotorLeftForward 3
#define MotorLeftBackward 9
#define MotorRightForward 10
#define MotorRightBackward 11

#define MAX_PWM 255


bool run;
float iTerm = 0;
float lastErr = 0;

volatile bool buttonPressed = false;
volatile unsigned long lastInterruptTime = 0;

SoftwareSerial BTSerial(7, 8);   // RX, TX

SerialCommand sCmdUSB(SerialPort);
SerialCommand sCmdBT(BTSerial);

unsigned long previous, calculationTime;
const int sensor[] = {A0, A1, A2, A3, A4, A5, A6, A7};

struct param_t {
  unsigned long cycleTime;
  int black[8];
  int white[8];
  int power;
  float kp, ki, kd, diff;
} params;

void onRun();

void onButtonPress() {
  unsigned long now = millis();
  if (now - lastInterruptTime > 150) {   // debounce
    buttonPressed = true;
}
lastInterruptTime = now;
}
void setup() {

  // === ADC prescaler op 32 zetten (ADPS2=1, ADPS1=1, ADPS0=0) ===
  ADCSRA &= ~((1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0));  // eerst wissen
  ADCSRA |=  (1<<ADPS2) | (1<<ADPS1);                  // prescaler = 32
  ADMUX = (1<<REFS0);  // expliciet AVcc + rechts-uitgelijnd

  SerialPort.begin(Baudrate);
  BTSerial.begin(BTBaudrate);

  // motor pins als OUTPUT
  pinMode(MotorLeftForward, OUTPUT);
  pinMode(MotorLeftBackward, OUTPUT);
  pinMode(MotorRightForward, OUTPUT);
  pinMode(MotorRightBackward, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), onButtonPress, FALLING);


  // USB commando’s
  sCmdUSB.addCommand("set", [](char* cmd){ handleSet(sCmdUSB, SerialPort); });
  sCmdUSB.addCommand("debug", [](char* cmd){ handleDebug(SerialPort); });
  sCmdUSB.addCommand("calibrate", [](char* cmd){ handleCalibrate(sCmdUSB, SerialPort); });
  sCmdUSB.setDefaultHandler([](char* cmd){ onUnknown(SerialPort, cmd); });


  // BT commando’s
  sCmdBT.addCommand("set", [](char* cmd){ handleSet(sCmdBT, BTSerial); });
  sCmdBT.addCommand("run", onRun);
  sCmdBT.addCommand("stop", onStop);
  sCmdBT.addCommand("debug", [](char* cmd){ handleDebug(BTSerial); });
  sCmdBT.addCommand("calibrate", [](char* cmd){ handleCalibrate(sCmdBT, BTSerial); });
  sCmdBT.setDefaultHandler([](char* cmd){ onUnknown(BTSerial, cmd); });

  iTerm = 0;  // Reset I-term bij opstarten
  lastErr = 0;   // Reset D-term

  EEPROM.get(0, params);
  SerialPort.println("EEPROM geladen:");
  handleDebug(BTSerial);  // laat direct alle waarden zien na opstarten

  SerialPort.println("ready");
  BTSerial.println("ready");
}

void loop() {
  if (buttonPressed) {
    buttonPressed = false;
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

    int normalised[8];
    for (int i = 0; i < 8; i++)
    {
      long value = readADC(i);
      normalised[i] = map(value, params.black[i], params.white[i], 0, 1000);
    }

    float position = 0;
    int index = 0;

    // Zoek de sensor met de donkerste (laagste) waarde = lijn
    for (int i = 1; i < 8; i++) {
      if (normalised[i] < normalised[index]) index = i;
    }

    if (normalised[index] > 800) { 
      run = false; // stopt direct
      position = 0; //
      } 


    else if (index == 0) {
      position = -52.5;             // 7 × 15mm / 2 = 52.5 mm links
    }
    else if (index == 7) {
      position = +52.5;             // 52.5 mm rechts
    }
    //Kwadratische interpolatie
    else {
      long sZero      = normalised[index];
      long sMinusOne  = normalised[index - 1];
      long sPlusOne   = normalised[index + 1];

      float b = (sPlusOne - sMinusOne) / 2.0;
      float a = sPlusOne - b - sZero;

      // Veiligheid: als a ≈ 0 (bijna rechte lijn overgang) gewoon lineair interpoleren
      if (abs(a) < 3.0) {  // 3 is een veilige grens
        position = index - 3.5;                     // midden van de drie sensoren
      } else {
        position = index - 3.5 + (-b / (2.0 * a));  // klassieke kwadratische formule
      }
      position *= 15.0;  // afstand tussen sensoren in mm
    }
    
    float error = -position; //error = setpoint - input
    float output = error * params.kp;

    if (run) {
      // I-term
      iTerm += params.ki * error;
      iTerm = constrain(iTerm, -510.0, 510.0);  // anti-windup
      output += iTerm;

      // D-term
      float dTerm = params.kd * (error - lastErr);
      output += dTerm;
      lastErr = error;  // update alleen tijdens rijden
    }

    output = constrain(output, -510, 510);

    int powerLeft  = 0;
    int powerRight = 0;

    if (run) {
      // Basis snelheid
      int base   = params.power;
      // Differentieel gedeelte (hoe harder je stuurt, hoe meer verschil links/rechts)
      int diff   = (int)(output * params.diff);      // bijv. output=30 → diff=30*0.8 = 24
      // Stuur-correctie (de echte PID output)
      int steer  = (int)output;

      // Links en rechts berekenen
      powerLeft  = base + diff + steer;
      powerRight = base - diff - steer;

      // Begrenzen op -255..255
      powerLeft  = constrain(powerLeft,  -255, 255);
      powerRight = constrain(powerRight, -255, 255);
    }

    driveMotor(MotorLeftForward,  MotorLeftBackward,  powerLeft);
    driveMotor(MotorRightForward, MotorRightBackward, powerRight);
  }

  unsigned long difference = micros() - current;
  if (difference > calculationTime) calculationTime = difference;
}

void driveMotor(int ain1, int ain2, int power) {

  if (power > 0) {
    // Vooruit
    analogWrite(ain1,  power);
    analogWrite(ain2,  0);
  }
  else if (power < 0) {
    // Achteruit
    analogWrite(ain1,  0);
    analogWrite(ain2, -power);
  }
  else {
    // Stop
    analogWrite(ain1,  0);
    analogWrite(ain2,  0);
  }
}

uint16_t readADC(uint8_t channel) {
  ADMUX = (1<<REFS0) | (channel & 0x0F);  // AVcc + rechts-uitgelijnd
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADC;
}


// handlers //
void onUnknown(Stream &port, char *command) {
  port.print("unknown command: \"");
  port.print(command);
  port.println("\"");
}

void onRun() {
  run = true;
  iTerm = 0;  // Reset iTerm
  lastErr = 0;  // Reset D-term
  BTSerial.println("run mode activated");
}

void onStop() {
  run = false;
  iTerm = 0;  // Reset iTerm
  lastErr = 0;  // Reset D-term

  driveMotor(MotorLeftForward,  MotorLeftBackward,  0);
  driveMotor(MotorRightForward, MotorRightBackward, 0);
  
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

void handleCalibrate(SerialCommand &cmd, Stream &port) {
  char* param = cmd.next();
  if (!param) return;

  if (strcmp(param, "black") == 0 || strcmp(param, "white") == 0) {
    bool isBlack = (strcmp(param, "black") == 0);
    port.print("Kalibreren "); port.println(param);

    // Normale ADC snelheid voor nauwkeurigheid
    ADCSRA = (ADCSRA & ~7) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // prescaler 128
    delay(500); // rustig tijd om te plaatsen

    const int samples = 20;
    long sum[8] = {0};
    for (int s = 0; s < samples; s++) {
      for (int i = 0; i < 8; i++) sum[i] += analogRead(sensor[i]);
      delay(5);
    }

    port.print(isBlack ? "black : " : "white : ");
    for (int i = 0; i < 8; i++) {
      int avg = sum[i] / samples;
      if (isBlack) params.black[i] = avg;
      else         params.white[i] = avg;
      port.print(avg);
      if (i < 7) port.print(", ");
    }
    port.println();

    // Terug naar jouw snelle prescaler
    ADCSRA &= ~7;
    ADCSRA |= (1<<ADPS2) | (1<<ADPS1);   // prescaler 32

    EEPROM.put(0, params);
    port.println("klaar + opgeslagen in EEPROM");
  }
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

  port.print("black : ");
  for(int i = 0; i < 8; i++) {
    port.print(params.black[i]);
    if (i < 7) port.print(", ");
  }
  port.println();

  port.print("white : ");
  for(int i = 0; i < 8; i++) {
    port.print(params.white[i]);
    if (i < 7) port.print(", ");
  }
  port.println();

  port.print("calculation time: "); port.println(calculationTime);
  calculationTime = 0;
}
