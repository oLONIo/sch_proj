#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <HTTPClient.h>

#define ST7920_DELAY_1 DELAY_NS(0)
#define ST7920_DELAY_2 DELAY_NS(188)
#define ST7920_DELAY_3 DELAY_NS(0)

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


const char* ssid     = "Tribun";
const char* password = "9162103173";

const long utcOffsetInSeconds = 10800;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Определение NTP-клиента для получения времени
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* CS=*/ 5, /* reset=*/ 22);
//U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 20);

void setup(void)
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED )
  {
    delay ( 500 );
    Serial.print ( "." );
  }
  timeClient.begin();
  u8g2.begin();
}

void loop(void) 
{
  timeClient.update();

  u8g2.clearBuffer();  // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
  u8g2.drawStr(45,20, daysOfTheWeek[timeClient.getDay()]);  // write something to the internal memory
  //u8g2.drawStr(30,25, timeClient.getHours() + ":" + timeClient.getMinutes());
  String hours = String(timeClient.getHours());
  u8g2.setCursor(60,36);
  u8g2.print(hours);
  String minutes = String(timeClient.getMinutes());
  u8g2.setCursor(60,46);
  u8g2.print(minutes);
  String seconds = String(timeClient.getSeconds());
  u8g2.setCursor(60,56);
  u8g2.print(seconds);
  u8g2.sendBuffer();  // transfer internal memory to the display
  delay(500);

  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
}
