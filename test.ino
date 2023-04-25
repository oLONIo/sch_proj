#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include <Wire.h>
#include <U8g2lib.h>
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* CS=*/ 5, /* reset=*/ 22);
#include "iRusFont.h"

const char* ssid     = "Tribun";                                     // ssid
const char* password = "9162103173";                                // пароль

const char* ntpServer = "pool.ntp.org";       // сервер времени
const long  gmtOffset_sec = 10800;            // 3600*3=10800 (UTC+3) Москва
const int   daylightOffset_sec = 0;               // переход на летнее время

String openWeatherMapApiKey = "9443470704066155bc0056b00626688b";     // ApiKey
String cityID = "524901";                                   // ID города

// Выделить буфер для документа JSON
// Внутри скобок 200-это емкость пула памяти в байтах
// Не забудьте изменить это значение, чтобы оно соответствовало вашему документу JSON.

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

String S = "";                                // для всякой ...
unsigned long previousMillis = 0;             // предыдущее значение времени
const long interval = 500;                    // интервал переключений
uint8_t sec05 = 0;                            // для отсчета полсекунды
uint8_t  minOWCount = 0;                      // для отсчета минут для запроса openweathermap
const uint8_t minOW = 10;                     // запрашивать погоду каждые 10 минут
uint16_t  minDTCount = 0;                     // для отсчета минут для синхронизации времени
const uint16_t minDT = 1440;                  // синхронизировать время каждые 24*60= 1440 минут (сутки)

void setup()
{
  Serial.begin(115200);
  u8g2.begin();
  u8g2.enableUTF8Print();                            // включаем подсветку
  rusClear();                                 // очистка экрана

  iPrintString ("Подключаемся к", 0, 0);      // текст, столбец, строка
  iPrintString (ssid, 0, 1);                  // ssid
  WiFi.begin(ssid, password);

  u8g2.setCursor(0, 2);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    u8g2.print(".");
  }
  u8g2.setCursor(0, 3);                        // Печатаем IP - шник
  u8g2.print("IP:");
  u8g2.print(WiFi.localIP());
  delay(4000);
  rusClear();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  // Синхронизируем внутренние часы с временем из интернета
  printWeather ();                                           // просим погоду
}

void loop()
{
  u8g2.setFont(u8g2_font_unifont_t_cyrillic);
  u8g2.setFontDirection(0);
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sec05++;
    // раз в полсекунды мигаем точками
    if (sec05 & 1) iPrintString (":", 2, 0); else iPrintString (" ", 2, 0);

    // раз в одну секунду обновляем время и температуру
    if (sec05 & 1) {
      printLocalTime();
    }

    // прошла одна минута
    if (sec05 >= 120) {
      sec05 = 0;
      minOWCount++;
      minDTCount++;

    //Прошло 10 минут
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
    u8g2.print("         ");
    u8g2.setCursor (11, 1);
    if (tempC == 0) u8g2.print(" ");
    if (tempC > 0) u8g2.print("+");
    u8g2.print(tempC);
    u8g2.print("C");
    u8g2.print(" ");
    u8g2.print(main_humidity);
    u8g2.print("%");

    u8g2.setCursor (0, 2);
    u8g2.print(wind_speed, 1);
    u8g2.print("M/C");

    // скорость и направление ветра
    uint8_t z = 0;
    if (wind_speed >= 10) z = 8; else z = 7;
    if (wind_deg >= 0 && wind_deg <= 22) iPrintString    ("C ",  z, 2);
    if (wind_deg >= 23 && wind_deg <= 67) iPrintString   ("CB",  z, 2);
    if (wind_deg >= 68 && wind_deg <= 112) iPrintString  ("B ",  z, 2);
    if (wind_deg >= 113 && wind_deg <= 157) iPrintString ("ЮВ",  z, 2);
    if (wind_deg >= 158 && wind_deg <= 202) iPrintString ("Ю ",  z, 2);
    if (wind_deg >= 203 && wind_deg <= 247) iPrintString ("Ю3",  z, 2);
    if (wind_deg >= 248 && wind_deg <= 292) iPrintString ("3 ",  z, 2);
    if (wind_deg >= 293 && wind_deg <= 337) iPrintString ("С3",  z, 2);
    if (wind_deg >= 338 && wind_deg <= 360) iPrintString ("С ",  z, 2);

    // давление в мм рт. ст.
    u8g2.setCursor (11, 2);
    u8g2.print(pressureMM);
    u8g2.print("MM");

    // Название города
    u8g2.setCursor (0, 3);
    const char* name = doc["name"];            // "Cherepovets"
    u8g2.print(name);

    u8g2.setCursor (10, 3);
    u8g2.print(" ");

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

    long sys_sunset = sys["sunset"];           // Закат
    //Преобразуем sys_sunset в структуру
    my_time  = localtime (&sys_sunset);

    u8g2.print(" ");
    u8g2.print(my_time, "%H");
    u8g2.print(my_time, "%M");

  } else {
    //нету подключения к ЛВС
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
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
  u8g2.print(" ");
  iPrintString (weekday[timeinfo.tm_wday], 6, 0);      // день недели
  u8g2.print(" ");
  u8g2.print(timeinfo.tm_mday);                         // день месяца
  if (timeinfo.tm_mday < 10) iPrintString (months[timeinfo.tm_mon], 11, 0);
  else iPrintString (months[timeinfo.tm_mon], 12, 0);  // название месяца
}
