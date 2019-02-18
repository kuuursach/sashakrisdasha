#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> 
#include <dht11.h>       // Добавляем библиотеку DHT11


dht11 DHT;               // Объявление переменной класса dht11
#define DHT11_PIN 4 // Датчик DHT11 подключен к цифровому пину номер 4

int readdht, temp, humidity;

int ldr = 38;
int light = 0;


//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiSSID[] = "iPhone (Дарья)"; // WiFi access point SSID
const char WiFiPSK[] = "123456788"; // WiFi password - empty string for open access points



const char TWPlatformBaseURL[] = "https://pp-1901311033s6.devportal.ptc.io";
const char appKey[] = "909cb96a-e3eb-4b76-8ac3-ca7f96f9997b"; 
const char thingName[] = "Greenhouse";





// this will set as the Accept header for all the HTTP requests to the ThingWorx server
// valid values are: application/json, text/xml, text/csv, text/html (default)

#define ACCEPT_TYPE "application/json"  

/////////////////////
//Attempt to make a WiFi connection. Checks if connection has been made once per second until timeout is reached
//returns TRUE if successful or FALSE if timed out
/////////////////////


// освещение
//String lamp;

boolean lamp_bool;

// проветривание 
//String condition;
boolean condition_bool;

// полив
//String pomp;
boolean pomp_bool;


// Частота опроса датчика DHT11
#define DHT11_UPDATE_TIME 10000
// Частота опроса аналоговых датчиков (СВЕТ)
#define ANALOG_UPDATE_TIME 10000
// Частота вывода данных на сервер IoT
#define IOT_UPDATE_TIME 60000

// Таймер обновления опроса датчика DHT11
long timer_dht11 = 0;
// Таймер обновления опроса аналоговых датчиков
long timer_analog = 0;
// Таймер обновления вывода на сервер IoT
long timer_iot = 0;





boolean connectToWiFi(int timeout){

  Serial.println("Connecting to: " + String(WiFiSSID));
  WiFi.begin(WiFiSSID, WiFiPSK);

  // loop while WiFi is not connected waiting one second between checks
  uint8_t tries = 0; // counter for how many times we have checked
  while ((WiFi.status() != WL_CONNECTED) && (tries < timeout) ){ // stop checking if connection has been made OR we have timed out
    tries++;
    Serial.printf(".");// print . for progress bar
    Serial.println(WiFi.status());
    delay(2000);
  }
  Serial.println("*"); //visual indication that board is connected or timeout

  if (WiFi.status() == WL_CONNECTED){ //check that WiFi is connected, print status and device IP address before returning
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else { //if WiFi is not connected we must have exceeded WiFi connection timeout
    return false;
  }

}

int Get (String nameOfThing, String property, bool check){
        HTTPClient http;
        int httpCode = -1;
        String response = "";
        Serial.print("[sendValue] begin...");
        String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/"+ nameOfThing +"/Properties/"+ property +"?appKey=" + String(appKey);
        Serial.println("URL>" + fullRequestURL + "<");
        http.begin(fullRequestURL);
        
        http.addHeader("Accept",ACCEPT_TYPE,false,false);
        
        http.addHeader("Content-Type",ACCEPT_TYPE,false,false);
        httpCode = http.GET();
        // httpCode will be negative on error
        if(httpCode > 0) {

            response = http.getString();
            Serial.printf("[sendValue] response code:%d body>",httpCode);
            Serial.println(response + "<\n");
             http.end();
            
      StaticJsonBuffer <750> jsonBuffer;
      JsonObject& json_array = jsonBuffer.parseObject(response);
      String  Fu = json_array["rows"][0][property];
      Serial.println(Fu);
    if (Fu[0] == 't')
    {check = true;}
    else check = false;
        } else {
            Serial.printf("[sendValue] PUT... failed, error: %s\n\n", http.errorToString(httpCode).c_str());
           http.end();
        }
         
        return check;}


int sendValue(String nameOfThing, String property, int  value){
        HTTPClient http;
        int httpCode = -1;
        String response = "";
        Serial.print("[sendValue] begin...");
        String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/"+ nameOfThing +"/Properties/"+ property +"?appKey=" + String(appKey);
        Serial.println("URL>" + fullRequestURL + "<");
        http.begin(fullRequestURL);
        
        http.addHeader("Accept",ACCEPT_TYPE,false,false);
        
        http.addHeader("Content-Type",ACCEPT_TYPE,false,false);
        
        String putBody = "{\"" + property + "\" : " + String(value) + "}";
        Serial.println("[sendValue] PUT body>" + putBody + "<");
        // start connection and send HTTP header
        httpCode = http.PUT(putBody);
        // httpCode will be negative on error
        if(httpCode > 0) {

            response = http.getString();
            Serial.printf("[sendValue] response code:%d body>",httpCode);
            Serial.println(response + "<\n");

        } else {
            Serial.printf("[sendValue] PUT... failed, error: %s\n\n", http.errorToString(httpCode).c_str());
        }
        http.end();
        return httpCode;
}




void setup() {

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();
    Serial.println();
    Serial.println();

    Serial.printf("Starting...\n");

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("   WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }
    
    
    connectToWiFi(10); // подключение к WiFi (кол-во попыток)
    
   /* int chk = DHT.read(DHT11_PIN);
    int humidity = DHT.humidity;
    int temp = DHT.temperature;
    light = analogRead(ldr);
     
    if (WiFi.status() == WL_CONNECTED) { // confirm WiFi is connected then loop forever as long as WiFi is connected
      sendValue(thingName, "light", light);
      sendValue(thingName, "humidity", humidity);
      sendValue(thingName, "temp", temp);
      

    }
    Serial.printf("****Wifi connection dropped****\n");
    WiFi.disconnect(true);*/
}

void loop()
{
  // Опрос датчика DHT11
  if (millis() > timer_dht11 + DHT11_UPDATE_TIME)
  {
    // Опрос датчиков
    readSensorDHT11();
    // Сброс таймера
    timer_dht11 = millis();
  }

   // Опрос аналоговых датчиков
  if (millis() > timer_analog + ANALOG_UPDATE_TIME)
  {
    // Опрос датчиков
    readSensorAnalog();
    // Сброс таймера
    timer_analog = millis();
  }

   // Вывод данных на сервер IoT
  if (millis() > timer_iot + IOT_UPDATE_TIME)
  {
    // Вывод данных на сервер IoT
      sendValue(thingName, "light", light);
      sendValue(thingName, "humidity", humidity);
      sendValue(thingName, "temp", temp);
      // Забираем данные 
      pomp_bool = Get(thingName,"pomp", pomp_bool);
      condition_bool = Get(thingName,"condition", condition_bool);
      lamp_bool = Get(thingName,"lamp", lamp_bool);
      Serial.println(pomp_bool);
      Serial.println(condition_bool);
      Serial.println(lamp_bool);
    // Сброс таймера
    timer_iot = millis();
  }

  
}

// Чтение датчика DHT11
void readSensorDHT11()
{
  // DHT11
   readdht = DHT.read(DHT11_PIN);
   humidity = DHT.humidity;
   temp = DHT.temperature;
 
}


// Чтение аналоговых датчиков
void readSensorAnalog()
{
  light = analogRead(ldr);
}



