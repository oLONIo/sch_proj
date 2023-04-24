#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include <Wire.h>
#include <U8g2lib.h>

#define ST7920_DELAY_1 DELAY_NS(0)
#define ST7920_DELAY_2 DELAY_NS(188)
#define ST7920_DELAY_3 DELAY_NS(0)

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* CS=*/ 5, /* reset=*/ 22);

const char* ssid     = "Tribun";					  // ssid WI FI
const char* password = "9162103173";					  // пароль от WI FI

const char* ntpServer = "pool.ntp.org";       // сервер времени
const long  gmtOffset_sec = 10800;            // 3600*3=10800 (UTC+3) Москва
const int   daylightOffset_sec = 0;           // переход на летнее время

String openWeatherMapApiKey = "9443470704066155bc0056b00626688b"; 			  // ApiKey с https://openweathermap.org/
String cityID = "524901";                           // ID города с https://openweathermap.org/

// Выделить буфер для документа JSON
// Внутри скобок 200-это емкость пула памяти в байтах
// Не забудьте изменить это значение, чтобы оно соответствовало вашему документу JSON.
// arduinojson.org/v6/assistant чтобы вычислить емкость
StaticJsonDocument<801> doc;
String jsonBuffer;

// массив строк месяцы
const char *months[]  = {
  "Января",   // 0  mem 4
  "Февраля",  // 1  mem 4
  "Марта",    // 2  mem 2
  "Апреля",   // 3  mem 3
  "Мая",      // 4  mem 2
  "Июня",     // 5  mem 4
  "Июля",     // 6  mem 4
  "Августа",  // 7  mem 3
  "Сентября", // 8  mem 4
  "Октября",  // 9  mem 4
  "Ноября",   // 10 mem 3
  "Декабря",  // 11 mem 4
};
// массив строк - дни недели
const char *weekday[]  = {
  "Вс",   // 0
  "Пн",   // 1
  "Вт",   // 2
  "Ср",   // 3
  "Чт",   // 4
  "Пт",   // 5
  "Сб",   // 6
};

String S = "";                                // для всякой хренотени

unsigned long previousMillis = 0;             // предыдущее значение времени
const long interval = 500;                    // интервал переключений
uint8_t sec05 = 0;                            // для отсчета полсекунды
uint8_t  minOWCount = 0;                      // для отсчета минут для запроса openweathermap
const uint8_t minOW = 2;                      // запрашивать погоду каждые 5 минут
uint16_t  minDTCount = 0;                     // для отсчета минут для синхронизации времени
const uint16_t minDT = 1440;                  // синхронизировать время каждые 24*60= 1440 минут (сутки)

void setup()
{
  Serial.begin(115200);
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  u8g2.drawStr(0,10, "Подключаемся к");      // текст, столбец, строка                  
  u8g2.drawStr(0,20, ssid);                  // ssid
  WiFi.begin(ssid, password);

  u8g2.setCursor(0, 11);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    u8g2.print('.');
  }
  u8g2.clearBuffer();
  u8g2.setCursor(0, 3);                        // Печатаем IP - шник
  u8g2.print("IP:");
  u8g2.print(WiFi.localIP());
  delay(4000);
  // Синхронизируем внутренние часы с временем из интернета
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // просим погоду
  printWeather ();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sec05++;
    // раз в полсекунды мигаем точками
    if (sec05 & 1) u8g2.drawStr(2, 0, ":"); else u8g2.drawStr(2, 0, " ");

    // раз в одну секунду обновляем время и температуру
    if (sec05 & 60) {
      printLocalTime();
    }

    // прошла одна минута
    if (sec05 >= 120) {
      sec05 = 0;
      minOWCount++;
      minDTCount++;

      //Прошло 5 минут
      if (minOWCount >= minOW) {
        minOWCount = 0;
        // печатаем погоду
        printWeather();
      }

      //Прошло 1440 минут (сутки)
      if (minDTCount >= minDT) {
        minDTCount = 0;
        //синхронизируем время
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      }
    }
  }

}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  // Your IP address with path or Domain name with URL path
  http.begin(serverName);
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  String payload = "{}";
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  return payload;
}

void printWeather () {
  // проверяем подключение
  if (WiFi.status() == WL_CONNECTED) {

    // Формируем строку для получения API
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?id=" + cityID + "&appid=" + openWeatherMapApiKey;
    // Запрашиваем строку json
    jsonBuffer = httpGETRequest(serverPath.c_str());
    Serial.println(jsonBuffer);
    deserializeJson(doc, jsonBuffer);

    JsonObject main = doc["main"];
    float main_temp = main["temp"];                      // температура Кельвины
    int8_t tempC = round(main_temp - 273.15);            // температура С
    int main_pressure = main["pressure"];                // давление hPa
    uint16_t pressureMM = round(main_pressure * 0.75);   // давление мм рт столба
    pressureMM = pressureMM - 11;                        // х/з
    int main_humidity = main["humidity"];                // влажность
    float wind_speed = doc["wind"]["speed"];             // скорость ветра
    int wind_deg = doc["wind"]["deg"];                   // направление ветра

    u8g2.setCursor (11, 1);
    //u8g2.print('|');
    if (tempC == 0) u8g2.print(' ');
    if (tempC > 0) u8g2.print('+');
    u8g2.print(tempC);
    u8g2.print('C');
    u8g2.print(' ');
    u8g2.print(main_humidity);
    u8g2.print('%');

    u8g2.setCursor (0, 2);
    u8g2.print(wind_speed, 1);
    u8g2.print("M/C");

    // скорость и направление ветра
    uint8_t z = 0;
    if (wind_speed >= 10) z = 8; else z = 7;
    if (wind_deg >= 0 && wind_deg <= 22) u8g2.drawStr(z, 2,"C ");
    if (wind_deg >= 23 && wind_deg <= 67) u8g2.drawStr(z, 2,"CB");
    if (wind_deg >= 68 && wind_deg <= 112) u8g2.drawStr(z, 2,"B ");
    if (wind_deg >= 113 && wind_deg <= 157) u8g2.drawStr(z, 2,"ЮВ");
    if (wind_deg >= 158 && wind_deg <= 202) u8g2.drawStr(z, 2,"Ю ");
    if (wind_deg >= 203 && wind_deg <= 247) u8g2.drawStr(z, 2,"Ю3");
    if (wind_deg >= 248 && wind_deg <= 292) u8g2.drawStr(z, 2,"3 ");
    if (wind_deg >= 293 && wind_deg <= 337) u8g2.drawStr(z, 2,"С3");
    if (wind_deg >= 338 && wind_deg <= 360) u8g2.drawStr(z, 2,"С ");

    // давление в мм рт. ст.
    u8g2.setCursor (11, 2);
    u8g2.print(pressureMM);
    u8g2.print("MM");

    // Название города
    u8g2.setCursor (0, 3);
    const char* name = doc["Москва"]; // "Cherepovets"
    u8g2.print(name);

    u8g2.setCursor (10, 3);
    u8g2.print(' ');

    JsonObject sys = doc["sys"];
    long sys_sunrise = sys["sunrise"];         // Восход
    // Указатель, в который будет помещен адрес структуры с
    // преобразованным временем
    struct tm *my_time;
    //Преобразуем sys_sunrise в структуру
    my_time  = localtime (&sys_sunrise);
    u8g2.setCursor (11, 3);
    u8g2.print(my_time, "%H");
    u8g2.print(my_time, "%M");

    long sys_sunset = sys["sunset"];   // Закат
    //Преобразуем sys_sunset в структуру
    my_time  = localtime (&sys_sunset);

    u8g2.print(' ');
    u8g2.print(my_time, "%H");
    u8g2.print(my_time, "%M");

  } else {
    //нету подключения к ЛВС
  }

  //проверка времени сработки процедуры
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  /*
    u8g2.setCursor(11, 3);
    u8g2.print(&timeinfo, "%H");
    u8g2.setCursor(13, 3);
    u8g2.print(&timeinfo, "%M");
  */
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  u8g2.setCursor(0, 0);
  u8g2.print(&timeinfo, "%H");
  u8g2.setCursor(3, 0);
  u8g2.print(&timeinfo, "%M");

  u8g2.print(' ');
  //u8g2.print(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  u8g2.drawStr(6, 0, weekday[timeinfo.tm_wday]);      // день недели
  u8g2.print(' ');
  u8g2.print(timeinfo.tm_mday);                         // день месяца
  if (timeinfo.tm_mday < 10) u8g2.drawStr(11, 0, months[timeinfo.tm_mon]);
  else u8g2.drawStr(12, 0, months[timeinfo.tm_mon]);  // название месяца

}
