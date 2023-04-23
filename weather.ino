#include <WiFiUdp.h>
#include <WiFi.h>
#include <Arduino.h>

#include <HTTPClient.h>
#include <ArduinoJson.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

//const char server[] = "dataservice.accuweather.com";
const char* ssid     = "Tribun";
const char* password = "9162103173";

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
}
 
void loop() {
   
   StaticJsonBuffer<2000> jsonBuffer;                   /// буфер на 2000 символов
   JsonObject& root = jsonBuffer.parseObject(line);     // скармиваем String
   if (!root.success()) {
    Serial.println("parseObject() failed");             // если ошибка, сообщаем об этом
     jsonGet();                                         // пинаем сервер еще раз
    return;                                             // и запускаем заного 
  }
  
  
                              /// отправка в Serial
  Serial.println();  
  String name = root["name"];                           // достаем имя, 
  Serial.print("City:");
  Serial.println(name);  
  
  float tempK = root["main"]["temp"];                   // достаем температуру из структуры main
  float tempC = tempK - 273.15;                         // переводим кельвины в цельси
  Serial.print("temp: ");
  Serial.print(tempC);                                  // отправляем значение в сериал
  Serial.println(" C");

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
  
  int pressurehPa = root["main"]["pressure"]; 
  float pressure = pressurehPa/1.333;
  Serial.print("Давление: ");
  Serial.print(pressure);
  Serial.println(" мм.рт.тс.");

  int humidity = root["main"]["humidity"]; 
  Serial.print("Влажность: ");
  Serial.print(humidity);  
  Serial.println(" %");

  float windspeed = root["wind"]["speed"]; 
  Serial.print("Скорость ветра: ");
  Serial.print(windspeed);  
  Serial.println(" м/с");

  int winddeg = root["wind"]["deg"]; 
  Serial.print("Температура ветра :");
  Serial.println(winddeg);  
 

  Serial.println();  
  Serial.println();  
  delay(50000);
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
 
  delay(1500);
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    line = client.readStringUntil('\r'); 
  }
  Serial.print(line);
  Serial.println();
  Serial.println("closing connection");
}

