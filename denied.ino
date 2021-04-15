////////////////////////// Access Denied  ////////////////////////////

void WrongPass() {
  Serial.println(password);
  lcd.clear();
  lcd.print("WRONG PASSWORD");
  lcd.setCursor(0, 1);
  lcd.print("TRY AGAIN");
  lcd.setCursor(14, 1);
  lcd.write(4);
  lcd.setCursor(15, 1);
  lcd.write(2);
  Serial.println("DENIED");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();
  }
  tone(buzz, 3000, 500);
  delay(1500);
  noTone(buzz);
  lcd.clear();
  lcdDefault();
  i = 0;
}

void denied() {
  lcd.clear();
  lcd.print("INVALID CARD");
  lcd.setCursor(0, 1);
  lcd.print("ACCESS DENIED");
  lcd.setCursor(14, 1);
  lcd.write(4);
  lcd.setCursor(15, 1);
  lcd.write(2);
  Serial.println("DENIED");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();
  }
  tone(buzz, 3000, 500);
  delay(1500);
  noTone(buzz);
  lcd.clear();
  lcdDefault();
  i = 0;
}