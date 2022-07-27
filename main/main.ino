/******************************************************************************/
/**
*   @file main.ino
*   @author Espen Holsen
*
*   This program is intended for a ECG project in 
*   IELET3109 - Advanced Sensor Systems at NTNU.
*
*   No license. Use however you want.    
*/
/******************************************************************************/

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ADS1115_WE.h> 
#include <Wire.h>

ADS1115_WE adc;
AsyncWebServer http(80);         // HTTP server on port 80
AsyncWebSocket webSocket("/ws");

const char* SSID = "";
const char* PASS = "";

/******************************************************************************/
/**
 *  @brief  Non-blocking. Acquires voltage from the ADC. The reading is formated 
 *          as JSON and broadcasted to all WebSocket clients. Skips if no new
 *          reading is available.
 */
/******************************************************************************/
void broadcastWebSocket() {
    if (!adc.isBusy()) {
        Serial.println(micros());
        float voltage = adc.getResult_V();
        adc.startSingleMeasurement();
        char json[20];
        sprintf(json, "{\"ECG\":%f}", voltage);
        webSocket.textAll(json);
    }
}

/******************************************************************************/
/**
 *  @brief  Configures the ADC, WiFi, HTTP server and WebSockets server. 
 *          Choose ADC gain so that the range is larger than the range of the
 *          expected signal. Sets data rate to 860 samples/s.
 */
/******************************************************************************/
void setup() {
    Wire.begin();
    adc.init();
    adc.setVoltageRange_mV(ADS1115_RANGE_2048);
    adc.setConvRate(ADS1115_860_SPS);
    adc.setPermanentAutoRangeMode(true);
    adc.startSingleMeasurement();
    
    WiFi.begin(SSID, PASS);
    Serial.begin(115200); // To show IP address
    Serial.println("\nConnecting to WiFi:");
    while (WiFi.status() != WL_CONNECTED);     
    Serial.print("\tSSID: ");
    Serial.println(SSID);
    Serial.print("\tIP:   ");
    Serial.println(WiFi.localIP());
    Serial.end();
    
    SPIFFS.begin();
    http.addHandler(&webSocket);
    http.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html");
    });
    http.begin();
}
 
/******************************************************************************/
/**
 *  @brief  Handles WebSocket clients. Data is attempted to broadcast every
 *          iteration. Non-blocking. Code that blocks may interfere with the
 *          servers.
 */
/******************************************************************************/
void loop() {
    webSocket.cleanupClients();
    broadcastWebSocket();
}