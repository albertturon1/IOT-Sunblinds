#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <SPI.h>

#include <WiFi.h>
#include "time.h"
#include <string.h>

//-------------CONSTANTS
//DISPLAY
#define SCREEN_SIZE 240
#define LINE_HEIGHT 25
#define PADDING_LEFT 5
#define TFT_CS 14  //not used
#define TFT_RST 17
#define TFT_DC 15
#define TFT_MOSI 23
#define TFT_SCLK 18
#define NAVIGATION_BUTTON_PIN 5
#define STATE_BUTTON_PIN 15

//BUTTON VARIABLES
int lastState = HIGH;  // the previous state from the input pin
int navigationButtonState;       // the current reading from the input pin
int activeSunblind = 0;

const int sunblindsLength = 3;

// const char* ssid = "MASZT TESTOWY 5G 200% MOCY";
const char* ssid = "TP-Link_CE3C";
const char* password = "78831353";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 7200;

//---------------STRUCTS---------------
struct Sunblind {
  String name;
  int id;
  bool state;
};

struct DateTime {
  String time;
  String date;
};

//---------------Sunblinds---------------
// Sunblind createSunblind(String name, int id, bool state) {
//   Sunblind sunblind;
//   sunblind.name = name;
//   sunblind.id = id;
//   sunblind.state = state;
//   return sunblind;
// }

// Sunblind* getSunblinds() {
//   static struct Sunblind sunblinds[sunblindsLength];
//   sunblinds[0] = createSunblind("Salon", 0, true);
//   sunblinds[1] = createSunblind("Kuchnia", 1, true);
//   sunblinds[2] = createSunblind("Sypialnia", 2, true);
//   return sunblinds;
// }

Sunblind sunblinds[3] = {
  {"Salon", 0, true},
  {"Kuchnia", 1, true},
  {"Sypialnia", 2, true},
};

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

//---------------Parralel loops---------------
//--------Button handler--------
TaskHandle_t buttonTaskHandle;

void buttonTask(void* pvParameters) {
  bool navigationButtonPressed = false;
  while (1) {
    int navigationButtonState = digitalRead(NAVIGATION_BUTTON_PIN);
    if (navigationButtonState == LOW && !navigationButtonPressed) {

      if (activeSunblind == 2) {
        activeSunblind = 0;
      } else {
        activeSunblind++;
      }
      renderText();
      navigationButtonPressed = true;
    }
    if (navigationButtonState == HIGH && navigationButtonPressed) {
      navigationButtonPressed = false;
    }
    vTaskDelay(pdMS_TO_TICKS(50));  // debounce delay
  }
}


void setup(void) {
  Serial.begin(115200);
  Serial.flush();  // Clear the Serial monitor buffer

  // initialize TFT
  tft.init(SCREEN_SIZE, SCREEN_SIZE, SPI_MODE3);  // Init ST7789 240x240, SPI_MODE3 NECESSARY
  tft.fillScreen(ST77XX_BLACK);
  // padding();
  tft.setRotation(2);
  tft.setTextColor(ST77XX_WHITE);

  pinMode(NAVIGATION_BUTTON_PIN, INPUT_PULLUP);
  renderText();

  // Initialize your task (2nd loop)
  xTaskCreatePinnedToCore(buttonTask, "Button Task", 2048, NULL, 1, &buttonTaskHandle, 1);
  setTime();  //connecting to wifi and setting time

}

void loop() {
  renderText();
}

//---------------Helpers---------------

void renderText() {
  // struct Sunblind* sunblinds = getSunblinds();

  DateTime dateTime = localDateTime();
  printDateTime(dateTime);

  drawColumnText("", 2);

  drawColumnText(sunblinds[activeSunblind].name, 3);
  drawColumnText(sunblindState(sunblinds[activeSunblind].state), 4);

  drawColumnText("", 5);

  drawColumnText("Swiatlo WIP", 6);
  drawColumnText("Temperatura WIP", 7);

  drawColumnText("", 8);

  drawColumnText("Planowana zmianaWIP", 9);
}

String sunblindState(bool state) {
  if (!state) return "Zamnknieta";
  return "Otwarta";
}

void printDateTime(DateTime dateTime) {
  delay(1000);
  tft.fillScreen(ST77XX_BLACK);  // Clear screen

  drawColumnText(dateTime.date, 0);
  drawColumnText(dateTime.time, 1);
}

void setTime() {
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.fillRect(5, 5, SCREEN_SIZE, 40, ST77XX_BLACK);
    drawText("Connecting to WiFi...", 5, 5);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  tft.fillScreen(ST77XX_BLACK);

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  DateTime dateTime = localDateTime();
  printDateTime(dateTime);

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}


// void drawColumn(char* texts[], int x, int y) {
//   size_t textsLength = sizeof(texts);  // calculate number of text strings

//   for (int i = 0; i < textsLength; i++) {
//     tft.setCursor(5, 5 + i * 15);
//     tft.print(texts[i]);
//   }
// }

void drawText(String text, int x, int y) {
  tft.setCursor(x, y);
  tft.setTextSize(2);
  tft.setTextWrap(true);
  tft.print(text);
}

void drawColumnText(String text, int i) {
  int interval = lineHeight(i);
  drawText(text, 3, interval);
}

DateTime localDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to DateTime");
    return { time: "Failed to get time", date: "Failed to get date" };
  }

  char date_buffer[11];
  strftime(date_buffer, 11, "%d-%m-%Y", &timeinfo);
  String date = date_buffer;

  char time_buffer[9];
  strftime(time_buffer, 9, "%T", &timeinfo);  // Format time as HH:MM:SS
  String time = time_buffer;

  return { date, time };
}

int lineHeight(int interval) {
  return 5 + 20 * interval;
}


// DateTime getDateTimeFromWiFi() {
//   // Connect to Wi-Fi
//   Serial.print("Connecting to ");
//   Serial.println(ssid);
//   WiFi.begin(ssid, password);

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("");
//   Serial.println("WiFi connected.");

//   // Init and get the time
//   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//   DateTime dateTime = localDateTime();

//   //disconnect WiFi as it's no longer needed
//   WiFi.disconnect(true);
//   WiFi.mode(WIFI_OFF);
//   return dateTime;
// }