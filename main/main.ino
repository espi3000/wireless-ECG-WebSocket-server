#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketServer.h>
#include <SPIFFS.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 adc;
WebServer http(80);             // Create HTTP server object on port 80
WebSocketsServer webSocket(81); // Create WebSockets server object on port 81

const char* SSID = "";
const char* PASS = "";

void handleGetRequest() {
    File webpage = SPIFFS.open("/index.html", "r"); // Open file located in data directory
    http.streamFile(webpage, "text/html");          // Return webpage with 200 OK response
    webpage.close();                                // Close the file to clear resources 
}

void broadcastWebSocket(int16_t) {
    if (!adc.conversionComplete()) {
        return; // Returns if no data is available
    }
    int16_t reading = adc.getLastConversionResults();
    adc.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, false);
    // float volts = adc.computeVolts(reading)
    
    char json[14]; // int16_t is max 6 bytes when formated as char*
    sprintf(json, "{\"ECG\":%d}", reading);
    webSocket.broadcastTXT(json);
}

void setup() {
    Serial.begin(115200); // Serial port for debugging purposes

    adc.setGain(GAIN_ONE); // TWOTHIRDS Remember multiplier
    adc.begin();
    // adc.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, false); // Start first reading
    
    if (!SPIFFS.begin()) { // Initialize SPIFFS
        Serial.println("An Error has occurred while mounting SPIFFS");
    }
    
    WiFi.begin(SSID, PASS); // Connect to Wi-Fi
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println(WiFi.localIP());  // Print ESP32 Local IP Address
    
    http.on("/", handleGetRequest); // The pointer to our function is passed
    http.begin();
    webSocket.begin();
}
 
void loop() {
    http.handleClient();
    webSocket.loop();

    int16_t reading = adc.readADC_Differential_0_1(); // This line of code is blocking from execution until a conversion is done (worst case 1.16ms for 860S/s)
    char* readingTxt = itoa(reading); // Convert int16_t to char*
    webSocket.broadcastTXT(readingTxt); // This function cannot use ints larger than int8_t
}

/*
uint16_t pulseTrain(uint16_t period) {
    static uint16_t time = millis();
    static uint16_t state = false;
    if ((millis() - time) > period/2) {
        time = millis();
        state = !state;
    }
    return state;
}

uint16_t waveform() {
    return 100*pulseTrain(1000) + random(50) - 25;
}
*/
