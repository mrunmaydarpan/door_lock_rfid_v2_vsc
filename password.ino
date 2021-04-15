void passLoop() {
  if (inLockB.pressed()) {
    if (lockState == 0)
      lockState = 1;
    lcd.clear();
  }

  while (lockState == 1) {
    lcd.home();
    lcd.print("NO ENTRY");
    lcd.setCursor(0, 1);
    lcd.print("ASK TO UNLOCK");
    digitalWrite(inLED, HIGH);
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      pixels.show();
    }
    if (inLockB.released()) {
      lcd.clear();
      lcdDefault();
      lockState = 0;
      digitalWrite(inLED, LOW);
    }
  }
  key = keypad.getKey();

  switch (keypad.getState()) {
  case PRESSED:
    if (key == '1' || key == '2' || key == '3' || key == '4' || key == '5' ||
        key == '6' || key == '7' || key == '8' || key == '9' || key == '0') {
      tone(buzz, 3000, 100);
      password[i++] = key;
      lcd.setCursor(i - 1, 1);
      lcd.print("*");
      delay(100);
    } else {
      switch (key) {
      case '*':
        change();
        break;
      case '#':
        pass_reset();
        break;
      }
    }
  }
  if (i == 4) {
    checkPass();
  }
}

void change() {
  int j = 0;
  lcd.clear();
  lcd.print("CURRENT PASSWORD");
  Serial.println("CURRENT PASSWORD");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 255));
    pixels.show();
  }
  while (j < 4) {
    char key = keypad.getKey();
    switch (keypad.getState()) {
    case PRESSED:
      if (key == '1' || key == '2' || key == '3' || key == '4' || key == '5' ||
          key == '6' || key == '7' || key == '8' || key == '9' || key == '0') {
        new_password[j++] = key;
        lcd.setCursor(j - 1, 1);
        lcd.print(key);
      } else {
        switch (key) {
        case '#':
          lcd.clear();
          lcdDefault();
          return;
        }
      }
    }
    key = 0;
  }
  if ((strncmp(new_password, initial_password, 4))) {
    lcd.clear();
    lcd.print("WRONG PASSWORD");
    lcd.setCursor(0, 1);
    lcd.print("TRY AGAIN");
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      pixels.show();
    }
    delay(1000);
    lcd.clear();
    lcdDefault();
  } else {
    j = 0;
    lcd.clear();
    lcd.print("NEW PASSWORD:");
    lcd.setCursor(0, 1);
    while (j < 4) {
      char key = keypad.getKey();
      switch (keypad.getState()) {
      case PRESSED:
        if (key == '1' || key == '2' || key == '3' || key == '4' ||
            key == '5' || key == '6' || key == '7' || key == '8' ||
            key == '9' || key == '0') {
          initial_password[j] = key;
          lcd.print(key);
          EEPROM.write(j + 500, key);
          j++;
        }
      }
    }
    lcd.clear();
    lcd.print("PASSWORD CHANGED");
    Serial.print("NEW PASSWORD: ");
    Serial.println(initial_password);
    Serial.print("EEPROM DATA: ");
    for (int k = 500; k < 504; k++) {
      Serial.print(EEPROM.read(k));
      Serial.print(" ");
    }
    Serial.println("");
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 255, 0));
      pixels.show();
    }
    delay(1000);
    lcd.clear();
    key = 0;
    lcdDefault();
  }
}

void checkPass() {
  if (!(strncmp(password, initial_password, 4))) {
    CorrectPass();
  } else {
    WrongPass();
  }
  Serial.println("");
  i = 0;
}

void pass_reset() {
  i = 0;
  lcd.clear();
  lcd.print("RESETED");
  Serial.println("RESETED");
  delay(1000);
  lcd.clear();
  lcdDefault();
}

void initial_Password() {
  for (int k = 500; k < 504; k++) {
    if (EEPROM.read(k) < 48 || EEPROM.read(k) > 57) {
      Serial.println("RESETING PASSWORD");
      EEPROM.write(500, 49);
      EEPROM.write(501, 50);
      EEPROM.write(502, 51);
      EEPROM.write(503, 52);
    }
    initial_password[k - 500] = EEPROM.read(k);
  }
  Serial.print("PASSWORD: ");
  Serial.println(initial_password);
  Serial.print("EEPROM DATA: ");
  for (int k = 500; k < 504; k++) {
    Serial.print(EEPROM.read(k));
    Serial.print(" ");
  }
  Serial.println("");
}