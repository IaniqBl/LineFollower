// Motor 1 pinnen
const int M1_IN1 = 11   ;
const int M1_IN2 = 10;
 
// Motor 2 pinnen
const int M2_IN1 = 9;
const int M2_IN2 = 3;
 
// PWM instellingen
const int MAX_PWM = 180; // => Max 6 V bij batterij van 8.4V
const int Stap = 2; // Versnelling
const int Pauze = 80; // Vertragin tussen stappen
const int loopduur = 2000; // Volle snelheid
 
void setup() {
  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);
}
 
//cylclus
void motorCycle(int IN1, int IN2) {
  // Versnellen vooruit
  for (int pwm = 0; pwm <= MAX_PWM; pwm += Stap) {
    analogWrite(IN1, pwm);
    analogWrite(IN2, 0);
    delay(Pauze);
  }
 
  // Constant vooruit
  analogWrite(IN1, MAX_PWM);
  analogWrite(IN2, 0);
  delay(loopduur);
 
  // Afremmen
  for (int pwm = MAX_PWM; pwm >= 0; pwm -= Stap) {
    analogWrite(IN1, pwm);
    analogWrite(IN2, 0);
    delay(Pauze);
  }
 
  delay(500);
 
  // Versnellen achteruit
  for (int pwm = 0; pwm <= MAX_PWM; pwm += Stap) {
    analogWrite(IN1, 0);
    analogWrite(IN2, pwm);
    delay(Pauze);
  }
 
  // Constant achteruit
  analogWrite(IN1, 0);
  analogWrite(IN2, MAX_PWM);
  delay(loopduur);
 
  // Afremmen
  for (int pwm = MAX_PWM; pwm >= 0; pwm -= Stap) {
    analogWrite(IN1, 0);
    analogWrite(IN2, pwm);
    delay(Pauze);
  }
 
  delay(500);
}
 
void loop() {
  motorCycle(M1_IN1, M1_IN2); // Motor 1 
  motorCycle(M2_IN1, M2_IN2); // Motor 2 
}