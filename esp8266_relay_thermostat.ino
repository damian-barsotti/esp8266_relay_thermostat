/*
  ESP8266 Relay Temperature by Damian Barsotti
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "src/esp8266_controllers/HTReader/HTReader.h"

#include "config_local.h" // File for testing outside git
#include "config.h"

ESP8266WebServer server(80);

float target_temperature = INIT_TARGET_TEMP;
float current_temperature, current_humidity;

HTReader sensor(
    DHTPIN, DHTTYPE, SLEEPING_TIME_IN_MSECONDS,
    temp_slope, temp_shift,
    humid_slope, humid_shift,
    n_reads);

bool relay_closed;

void publish_data_sensor(float p_temperature, float p_humidity)
{
    current_temperature = p_temperature;
    current_humidity = p_humidity;
}

void serial_print_current_sensor()
{
    Serial.print("Current temperature: ");
    Serial.print(current_temperature);
    Serial.print(", Target temperature: ");
    Serial.print(target_temperature);
    Serial.print(", Current humidity: ");
    Serial.println(current_humidity);
}

bool setup_wifi()
{

    // init the WiFi connection
    Serial.println();
    Serial.print("INFO: Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);

    //  Enable light sleep
    wifi_set_sleep_type(LIGHT_SLEEP_T);

    if (local_IP != IPAddress(0, 0, 0, 0) && !WiFi.config(local_IP, gateway, subnet))
    {
        Serial.println("STA Failed to configure fixed IP");
        return false;
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    static const int max_attempt = 100;
    int attempt = 0;
    while (attempt < max_attempt && WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        attempt++;
    }
    Serial.println("");

    return (attempt < max_attempt);
}

void relay_temp()
{

    relay_closed = current_temperature < target_temperature;
    if (relay_closed)
        digitalWrite(RELAYPIN, HIGH);
    else
        digitalWrite(RELAYPIN, LOW);
}

const char MAIN_page_template[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>

<h2>Poio Relay<h2>
<h3> HTML Form ESP8266</h3>

<p>
  Current temperature: %.2f
</p>
<p>
  Current humidity: %.2f
</p>

<form action="/action_page">
  Target temperature:<br>
  <input type="range" id="target_temp" name="target_temp" min="16" max="30" step="0.5" value="%.2f" 
    oninput="this.nextElementSibling.value = this.value"">
  <output>%.2f</output>
  <br><br>
  <input type="submit" value="Submit">
</form> 

<p>
  Relay state: %s
</p>
</body>
</html>
)=====";

const char GO_back[] PROGMEM = "<a href='/'> Go Back </a>";

//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot()
{
    char MAIN_page[600];
    const char *str_relay_state;

    if (relay_closed)
        str_relay_state = "closed";
    else
        str_relay_state = "open";

    snprintf(MAIN_page, 600, MAIN_page_template, current_temperature, current_humidity,
             target_temperature, target_temperature, str_relay_state);
    Serial.println(strlen(MAIN_page));
    server.send(200, "text/html", MAIN_page); // Send web page
}

//===============================================================
// This routine is executed when you press submit
//===============================================================
void handleForm()
{

    float send_temperature = server.arg("target_temp").toFloat();

    Serial.print("Received temp: ");
    Serial.println(send_temperature);
    Serial.print("Actual target temp: ");
    Serial.println(target_temperature);

    if (send_temperature != target_temperature)
    {
        target_temperature = send_temperature;
        relay_temp();
    }

    Serial.print("Target Temperature: ");
    Serial.println(target_temperature);

    server.send(200, "text/html", GO_back); // Send web page
}

void setup()
{
    float temperature, humidity;

    // init the serial
    Serial.begin(115200);
    // Take some time to open up the Serial Monitor
    delay(1000);
    // pinMode(RELAYPIN, OUTPUT);

    pinMode(RELAYPIN, OUTPUT);

    // Restart ESP if max attempt reached
    if (!setup_wifi())
    {
        Serial.println("ERROR: max_attempt reached to WiFi connect");
        Serial.print("Waiting and Restaring");
        WiFi.disconnect();
        delay(1000);
        ESP.restart();
    }

    Serial.println(String("IP: ") + WiFi.localIP().toString());
    sensor.begin();
    while (sensor.error())
    {
        Serial.println("ERROR: sensor read. Retrying ...");
        delay(sensor.delay_ms());
        sensor.reset();
    }

    publish_data_sensor(sensor.getTemp(), sensor.getHumid());
    serial_print_current_sensor();

    server.on("/", handleRoot);            // Which routine to handle at root location
    server.on("/action_page", handleForm); // form action is handled here

    server.begin(); // Start server
    Serial.println("HTTP server started");
}

// the loop function runs over and over again forever
void loop()
{

    if (sensor.beginLoop())
    {
        publish_data_sensor(sensor.getTemp(), sensor.getHumid());
        serial_print_current_sensor();
        relay_temp();
    }

    if (sensor.error())
        Serial.println("Failed to read from sensor!");

    server.handleClient(); // Handle client requests

    delay(SLEEPING_TIME_IN_MSECONDS);
}
