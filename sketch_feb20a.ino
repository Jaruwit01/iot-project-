/*************************************************************

  This is a simple demo of sending and receiving some data.
  Be sure to check out other examples!
 *************************************************************/

/* Fill-in information from Blynk Device Info here */
#define TdsSensorPin A0
#define VREF 5.0           // analog reference voltage(Volt) of the ADC
#define SCOUNT 30          // sum of sample point
int analogBuffer[SCOUNT];  // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;

#define BLYNK_TEMPLATE_ID "TMPL6k6TgOFX3"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "f-4SfefjNBvVZhHqfTjZEzzKn5yvNBPe"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#define LINE_TOKEN "jNMvaFHwyvenbuUKJsAD0pGJy0Q6Fy9lup8rj8WtHry"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TridentTD_LineNotify.h>
#include <Wire.h>

#define TRIGGER_PIN D6  // Replace D6 with the actual pin you connect the sensor's trigger to
#define ECHO_PIN D5     // Replace D5 with the actual pin you connect the sensor's echo to
#define LED_PIN D7      // Replace D7 with the actual pin you connect the LED to

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "WhatTheHell 2.4";
char pass[] = "01010101";

int water_max = 7;
int water_min = 40;
bool fullstate = false;
bool emptystate = false;
bool drink = false;
bool notDrink = false;

BlynkTimer timer;

// Flag to determine whether to measure distance or not
bool measureDistance = true;



// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0) {
  // Set incoming value from pin V0 to a variable
  if (param.asInt() == 1) {
    measureDistance = true;  // Enable distance measurement
    digitalWrite(LED_PIN, HIGH);
  } else {
    measureDistance = false;  // Disable distance measurement
    digitalWrite(LED_PIN, LOW);
  }
}

BLYNK_WRITE(V2) // Write handler for water_max
{
  water_max = param.asInt();
  Blynk.virtualWrite(V4, water_max); // Update water_max value on Blynk app
}

BLYNK_WRITE(V3) // Write handler for water_min
{
  water_min = param.asInt();
  Blynk.virtualWrite(V4, water_min); // Update water_min value on Blynk app
}

// This function sends Arduino's uptime and controls the LED
void myTimerEvent() {

  // Send trigger signal
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Wait for Echo to respond
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate distance
  long distance = duration * 0.034 / 2;  // Speed of sound in air is 34 cm/ms
  // Send distance value to Blynk

  if (distance <= water_max) {
    digitalWrite(LED_PIN, LOW);
    if (!fullstate) {
      LINE.notify("น้ำเต็มแล้ว");
      fullstate = true;
      emptystate = false;
    }
  } else if (distance >= water_min) {
    digitalWrite(LED_PIN, HIGH);
    if (!emptystate) {
      LINE.notify("น้ำจาหมดแล้ว");
      fullstate = false;
      emptystate = true;
    }
  }
  Serial.print(distance);
  Serial.println(" cm");
  Blynk.virtualWrite(V1, distance);
}
void TSD() {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U)  //every 40 milliseconds,read the analog value from the ADC
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);  //read the analog value and store into the buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;                                                                                                   // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);                                                                                                                //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge = averageVoltage / compensationCoefficient;                                                                                                             //temperature compensation
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5;  //convert voltage value to tds value
    if (tdsValue >= 50 && tdsValue <= 250) {
      if (!drink) {
        LINE.notify("Water is drinkable");
        drink = true;
        notDrink = false;
      }
    } else if (tdsValue > 250 && tdsValue <= 500) {
      if (!notDrink) {
        LINE.notify("Filter cartridge should be replaced");
        drink = false;
        notDrink = true;
      }
    }
    Serial.print("TDS Value:");
    Serial.print(tdsValue, 0);
    Serial.println("ppm");
    Blynk.virtualWrite(V5, tdsValue);
  }
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}

void setup() {
  // Debug console
  Serial.begin(115200);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TdsSensorPin, INPUT);
  Serial.println(LINE.getVersion());
  WiFi.begin(ssid, pass);
  Serial.printf("WiFi connecting to %s\n", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(400);
  }

  Serial.printf("\nWiFi connected\nIP : ");
  Serial.println(WiFi.localIP());

  myTimerEvent();
  LINE.setToken(LINE_TOKEN);
  timer.setInterval(200L, myTimerEvent);
  timer.setInterval(500L, TSD);
}

void loop() {
  Blynk.run();
  TSD();
  timer.run();
}
