volatile unsigned long lastInterruptTime = 0;

void InterruptButton() {
  unsigned long now = millis();

  if (now - lastInterruptTime > 150) {   // debounce
    if (digitalRead(2) == LOW) {        
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  }

  lastInterruptTime = now;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), InterruptButton, CHANGE);
}

void loop() {
  
}
