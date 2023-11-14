//#include <Arduino.h>
#include "myEspLib.h"

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme; // I2C


# define analog_voltage_pin A0   /* analog voltage */
# define sensor_power_pin 5     /* power up bme sensor */
# define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */


uint32_t battery_voltage_mv = 0;
uint64_t time_to_sleep = 300;   /* Time ESP32 will go to sleep (in seconds) */  


float p, t, a, h;
#define SEALEVELPRESSURE_HPA (1013.25)


void printValues() {
  /*
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" °C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
    */
}

float battery_voltage  = 0.0;

String battery_status ="Ok";
char mqtt_message[300];

//RTC_DATA_ATTR int bootcount = 0;

bool bme_status;

void setup()
{  
 /* setup pin modes */
  pinMode(analog_voltage_pin, INPUT);  
  pinMode(sensor_power_pin, OUTPUT);

  /*power up capacitive sensor */
  digitalWrite(sensor_power_pin, HIGH);

  //slow down esp to save battery
  setCpuFrequencyMhz(80);

  //Serial.end();
  //Serial.begin(115200);
  //bootcount ++;

  delay(100);
//  Serial.println("Waking Up");



  bme_status = bme.begin(0x76); 



  /*####################################################*/
  /* Voltage Monitor */
  for(int i = 0; i < 16; i++) {
    battery_voltage_mv += analogReadMilliVolts(analog_voltage_pin); // ADC with correction - read multiple times  
    delay(200);  
  }
  battery_voltage = (battery_voltage_mv*2/16)/1000.0;     // voltage divider halfs values attenuation ratio 1/2, mV --> V
  //  Serial.print("analogReadMilliVolts : ");
  //  Serial.println(battery_voltage, 3);    
  

  if(battery_voltage < 3.4 && battery_voltage > 0){
    battery_status = "Battery-Low";
  }

  time_to_sleep = time_to_sleep * uS_TO_S_FACTOR;
    
  /*####################################################*/
  /* Read the bme sensor values */
  t = bme.readTemperature();    
  p = bme.readPressure()/ 100.0;
  a = bme.readAltitude(SEALEVELPRESSURE_HPA);
  h = bme.readHumidity();

  /* debug purposes only */
  printValues();
  

    /* Do not use sprintf as it may crash due to buffer overflow in case you ma strings and floats */
    snprintf(mqtt_message, sizeof(mqtt_message), "{"
      "\"temperature\":%.2f,"
      "\"pressure\":%.1f,"
      "\"altitude\":%.2f,"
      "\"humidity\":%.2f,"
      "\"deep_sleep\":%d,"
      "\"battery_voltage\":%.2f,"
      "\"battery_status\":\"%s\""
      "}\n",
    t, p, a, h, time_to_sleep/uS_TO_S_FACTOR, battery_voltage, battery_status.c_str());
    //Serial.println(mqtt_message);

  /*power down bme sensor */
  digitalWrite(sensor_power_pin, LOW); 

  /*####################################################*/
  /* connecto to WiFi, MQTT & publish message */
  if(connectNetwork()){
    if (mqttClient.connected()) { 

      /* prepare for retained publish of the message */

      int msg_hst_length = strlen(mqtt_message);
      boolean retained = true;

      /* old - publish not retained 
      mqttClient.publish("esp/solarbox/01", mqtt_message); 
      */     
      mqttClient.publish("esp/solarbox/01", (byte*)mqtt_message, msg_hst_length, retained); 
    } 
  }


  /*####################################################*/
  /* GO DEEP SLEEP */
  //Wait a little bit to make sure publish is finished
  delay(2000);

  disconnectNetwork();  //disconnect mqtt & Wifi
  //Serial.println("Done - activating deepsleep mode");

  esp_sleep_enable_timer_wakeup(time_to_sleep);
  //Serial.flush(); // wait that all serial information is send
  //Serial.end();   //stop serial connection
  esp_deep_sleep_start(); 



}

void loop() {
}


