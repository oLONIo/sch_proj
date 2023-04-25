void rusClear () {
  u8g2.clearBuffer();
  u8g2.clearDisplay();
}

void iPrintString (String S, uint8_t col, uint8_t row) {
  u8g2.clearDisplay();
  u8g2.clearBuffer();
  u8g2.setCursor(col, row);
  u8g2.print(S);
  u8g2.sendBuffer();
  col++;
        // u8g2.setCursor(19, 3);
        // u8g2.print(numberCreateChar);
}
