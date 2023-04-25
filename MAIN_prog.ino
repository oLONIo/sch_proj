#include <WiFiUdp.h>
#include <WiFi.h>
#include <Arduino.h>
#include <NTPClient.h>
#include <U8g2lib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* CS=*/ 5, /* reset=*/ 22);

const long utcOffsetInSeconds = 10800;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//const char server[] = "dataservice.accuweather.com";
const char* ssid     = "Tribun";
const char* password = "9162103173";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

const char* host = "api.openweathermap.org";
String line;

void setup() {
  Serial.begin(115200);
  delay(10);
  
  
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  jsonGet();
  timeClient.begin();
  u8g2.begin();
}
 
void loop() {
   
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

   StaticJsonDocument<2000> root;                   /// буфер на 2000 символов
   deserializeJson(root, line);
   DeserializationError error = deserializeJson(root, line);     // скармиваем String
   if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    jsonGet();
    return;
  }
  
  timeClient.update();

                              /// отправка в Serial
  Serial.println();  
  String name = root["name"];                           // достаем имя, 
  Serial.print("City:");
  Serial.println(name);
  u8g2.setCursor(2,10);
  u8g2.print(name);

  u8g2.print(", ");
  u8g2.print(daysOfTheWeek[timeClient.getDay()]);
  
  float tempK = root["main"]["temp"];                   // достаем температуру из структуры main
  float tempC = tempK - 273.15;                         // переводим кельвины в цельси
  Serial.print("temp: ");
  Serial.print(tempC);                                  // отправляем значение в сериал
  Serial.println(" C");
  u8g2.setCursor(2,20);
  u8g2.print(tempC);
  u8g2.print(" C");

  float tempKmin = root["main"]["temp_min"];            // и так далее
  float tempCmin = tempKmin - 273.15;
  Serial.print("Мин. температура: ");
  Serial.print(tempCmin);
  Serial.println(" C");

  float tempKmax = root["main"]["temp_max"];
  float tempCmax = tempKmax - 273.15;
  Serial.print("Макс. температура: ");
  Serial.print(tempCmax);
  Serial.println(" C");
  
  float feels1 = root["main"]["feels_like"];
  float feels = feels1 - 273.15;
  Serial.print("Ощущается: ");
  Serial.print(feels);
  Serial.println(" C");
  u8g2.setCursor(2,50);
  u8g2.print("Feels: ");
  u8g2.print(feels);
  u8g2.print(" C");
  
  int pressurehPa = root["main"]["pressure"]; 
  float pressure = pressurehPa/1.333;
  Serial.print("Давление: ");
  Serial.print(pressure);
  Serial.println(" мм.рт.тс.");
  u8g2.setCursor(2,60);
  u8g2.print(pressure);
  u8g2.print(" mmhg");

  int humidity = root["main"]["humidity"]; 
  Serial.print("Влажность: ");
  Serial.print(humidity);  
  Serial.println(" %");
  u8g2.setCursor(2,30);
  u8g2.print(humidity);
  u8g2.print(" %");

  float windspeed = root["wind"]["speed"]; 
  Serial.print("Скорость ветра: ");
  Serial.print(windspeed);  
  Serial.println(" м/с");
  u8g2.setCursor(2,40);
  u8g2.print(windspeed);
  u8g2.print(" m/s");

  int winddeg = root["wind"]["deg"]; 
  Serial.print("Температура ветра :");
  Serial.println(winddeg);  
 

  Serial.println();  
  Serial.println();  
  delay(1000);

  String hours = String(timeClient.getHours());
  String minutes = String(timeClient.getMinutes());
  String seconds = String(timeClient.getSeconds());
  u8g2.setCursor(70,30);
  u8g2.print(hours);
  u8g2.print(":");
  u8g2.print(minutes);
  u8g2.print(":");
  u8g2.print(seconds);

  u8g2.sendBuffer();

}
 

void jsonGet() {
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
    client.println("GET /data/2.5/weather?lat=55.7504461&lon=37.6174943&appid=9443470704066155bc0056b00626688b HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
 
  delay(500);
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    line = client.readStringUntil('\r');
  }
  Serial.print(line);
  Serial.println();
  Serial.println("closing connection");
}
