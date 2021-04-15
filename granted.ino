///////////////////////////  Access Granted  /////////////////////////

void CorrectPass() {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
    pixels.show();
  }
  Serial.println(password);
  lcd.clear();
  lcd.print("DOOR UNLOCKED");
  lcd.setCursor(0, 1);
  lcd.print("WELCOME");
  lcd.setCursor(15, 1);
  lcd.write(3);
  Serial.println("ACCEPTED");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
    pixels.show();
  }
  digitalWrite(relay, HIGH);
  delay(50);
  digitalWrite(relay, LOW);
  beep();
  delay(1300);
  lcd.clear();
  lcdDefault();
  i = 0;
}

void granted() {
  Serial.println(password);
  lcd.clear();
  lcd.print("CARD ACCEPTED");
  lcd.setCursor(0, 1);
  lcd.print("WELCOME");
  lcd.setCursor(15, 1);
  lcd.write(3);
  Serial.println("ACCEPTED");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
    pixels.show();
  }
  digitalWrite(relay, HIGH);
  delay(20);
  digitalWrite(relay, LOW);
  beep();
  delay(1300);
  lcd.clear();
  lcdDefault();
  i = 0;
}