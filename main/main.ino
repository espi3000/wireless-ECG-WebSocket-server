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
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 adc;
WebServer http(80);             // HTTP server on port 80
WebSocketsServer webSocket(81); // WebSockets server on port 81

const char* SSID = "Holsen 2.4";
const char* PASS = "2311021058";

/******************************************************************************/
/**
*   @brief  Runs when a client sends a request to the IP. Opens the html file 
*           in the "/data" directory. The file is returned to the client with
*           a 200 OK response.        
*/
/******************************************************************************/
void handleGetRequest() {
    File webpage = SPIFFS.open("/index.html", "r");
    http.streamFile(webpage, "text/html");
    webpage.close();
}

/******************************************************************************/
/**
 *  @brief  Non-blocking. Acquires reading from the ADC and calculates the
 *          volts. The reading is formated as JSON and broadcasted to all 
 *          WebSocket clients. Skips if no new reading is available.
 */
/******************************************************************************/
void broadcastWebSocket() {
    if (adc.conversionComplete()) {
        int16_t reading = adc.getLastConversionResults();
        float volts = adc.computeVolts(reading);
        char json[20];
        sprintf(json, "{\"ECG\":%f}", volts);
        webSocket.broadcastTXT(json);
        adc.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, false);
    }
}

/******************************************************************************/
/**
 *  @brief  Configures the ADC, WiFi, HTTP server and WebSockets server. 
 *          Choose ADC gain so that the range is larger than the range of the
 *          expected signal. Sets data rate to 254 samples/s.
 * 
 *          Variable name:  Gain:       Range:      Resolution:
 *          GAIN_TWOTHIRDS  2/3x gain   +/- 6.144V  0.1875mV
 *          GAIN_ONE        1x gain     +/- 4.096V  0.125mV
 *          GAIN_TWO        2x gain     +/- 2.048V  0.0625mV
 *          GAIN_FOUR       4x gain     +/- 1.024V  0.03125mV
 *          GAIN_EIGHT      8x gain     +/- 0.512V  0.015625mV
 *          GAIN_SIXTEEN    16x gain    +/- 0.256V  0.0078125mV
 */
/******************************************************************************/
void setup() {
    adc.setGain(GAIN_TWO);
    adc.setDataRate(254);
    adc.begin();
    adc.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, false);
    
    WiFi.begin(SSID, PASS);
    Serial.begin(115200); // To show IP address
    Serial.println(adc.getDataRate());
    Serial.println("\nConnecting to WiFi:");
    while (WiFi.status() != WL_CONNECTED);     
    Serial.print("\tSSID: ");
    Serial.println(SSID);
    Serial.print("\tIP:   ");
    Serial.println(WiFi.localIP());
    Serial.end();
    
    http.on("/", handleGetRequest); // Runs when <IP>/ is requested by client
    http.begin();
    webSocket.begin();
    SPIFFS.begin();
}
 
/******************************************************************************/
/**
 *  @brief  Handles HTTP GET requests and WebSocket clients. Data is attempted 
 *          to broadcast every iteration. Non-blocking. Code that blocks may
 *          interfere with the servers.
 */
/******************************************************************************/
void loop() {
    http.handleClient();
    webSocket.loop();
    broadcastWebSocket();
}