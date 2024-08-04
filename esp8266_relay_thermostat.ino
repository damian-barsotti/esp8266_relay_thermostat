/*
  ESP8266 Relay Temperature by Damian Barsotti
*/

#include <ESP8266WebServer.h>

#include "src/esp8266_controllers/Wifi/Wifi.h"
#include "src/esp8266_controllers/HTReader/HTReader.h"
#include "src/esp8266_controllers/Mqtt/Mqtt.h"

#if __has_include("config_local.h")
#include "config_local.h" // File for testing outside git
Wifi wifi(Serial, WIFI_SSID, WIFI_PASSWORD, local_IP, gateway, subnet);
#else
#include "config.h"
Wifi wifi(Serial, WIFI_SSID, WIFI_PASSWORD);
#endif

ESP8266WebServer server(80);

float target_temperature = INIT_TARGET_TEMP;
float current_temperature, current_humidity;
bool relay_closed = true;
const char *relay_mode = COMM_ON;

HTReader ht_sensor(
    DHTPIN, DHTTYPE, SLEEPING_TIME_IN_MSECONDS,
    temp_slope, temp_shift,
    humid_slope, humid_shift,
    n_reads);

std::array topics{MQTT_SENSOR_TOPIC,
                  MQTT_REALY_POWER_SET_TOPIC,
                  MQTT_RELAY_MODE_SET_TOPIC,
                  MQTT_RELAY_TEMP_SET_TOPIC,
                  MQTT_RELAY_GET_TOPIC};

Mqtt mqtt(Serial, MQTT_SERVER_IP, MQTT_SERVER_PORT,
          MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
          MQTT_LOG_TOPIC,
          topics.data(), topics.size(),
          callback);

String header_log(char const *level, int n_log)
{
    return String(level) + " " + String(n_log) + ": ";
}

String header_log_info(int n_log)
{
    return header_log("INFO", n_log);
}

String header_log_warn(int n_log)
{
    return header_log("WARN", n_log);
}

int n_log_info = 0;
int n_log_warn = 0;

void logger_info(String msg)
{
    if (mqtt.log(header_log_info(n_log_info) + msg))
        n_log_info++;
}

void logger_warn(String msg)
{
    if (mqtt.log(header_log_warn(n_log_warn) + msg))
        n_log_warn++;
}

// function called to publish the temperature and the humidity
void publish_data_sensor(float p_temperature, float p_humidity)
{
    DynamicJsonDocument root(200);

    current_temperature = p_temperature;
    current_humidity = p_humidity;
    root["temperature"] = (String)p_temperature;
    root["humidity"] = (String)p_humidity;

    mqtt.publish(root, MQTT_SENSOR_TOPIC);
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

void serial_print_relay_state()
{
    if (relay_closed)
        Serial.print("Relay closed. ");
    else
        Serial.print("Relay open. ");
    Serial.print("Relay mode ");
    Serial.println(relay_mode);
}

void relay_temp()
{

    if (relay_mode == COMM_AUTO)
    {
        relay_closed = current_temperature < target_temperature;
    }

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

// function called to publish the relay thermostat state
void publish_relay_thermostat_state()
{
    DynamicJsonDocument root(200);

    root["temperature"] = String(target_temperature);
    root["mode"] = String(relay_mode);
    mqtt.publish(root, MQTT_RELAY_GET_TOPIC);
}

// function called when a MQTT message arrived
void callback(char *topic, byte *payload, unsigned int length)
{
    char command[length + 1];

    mqtt.announce_callback();

    Serial.print("Callback called with topic ");
    Serial.println(topic);
    if (strcmp(topic, MQTT_RELAY_GET_TOPIC) == 0 ||
        strcmp(topic, MQTT_LOG_TOPIC) == 0 ||
        strcmp(topic, MQTT_SENSOR_TOPIC) == 0)
        return;

    Serial.print("CB: Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    for (int i = 0; i < length; i++)
        command[i] = tolower((char)payload[i]);
    command[length] = '\0';

    Serial.println(command);

    if (strcmp(MQTT_REALY_POWER_SET_TOPIC, topic) == 0)
    {
        if (strcmp(COMM_OFF, command) == 0)
        {
            Serial.println("Turn Relay Thermostat off ...");
            relay_mode = COMM_OFF;
            relay_closed = false;
        }
        else
        { // if (strcmp(COMM_ON, command) == 0) {
            Serial.println("Turn Relay Thermostat on ...");
            relay_mode = COMM_ON;
            relay_closed = true;
        }
    }
    else if (strcmp(MQTT_RELAY_MODE_SET_TOPIC, topic) == 0)
    {

        if (strcmp(COMM_OFF, command) == 0)
        {
            Serial.println("Set Relay Thermostat mode to off ...");
            relay_mode = COMM_OFF;
            relay_closed = false;
        }
        else if (strcmp(COMM_AUTO, command) == 0)
        {
            Serial.println("Set Relay Thermostat mode to auto ...");
            relay_mode = COMM_AUTO;
        }
    }
    else if (strcmp(MQTT_RELAY_TEMP_SET_TOPIC, topic) == 0)
    {

        target_temperature = round(String(command).toFloat());
        Serial.print("Set temperature to ");
        Serial.println(target_temperature);
    }

    relay_temp();
    publish_relay_thermostat_state();
    serial_print_current_sensor();

    delay(500);
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
    if (!wifi.begin())
    {
        Serial.println("ERROR: max_attempt reached to WiFi connect");
        Serial.print("Waiting and Restaring");
        wifi.disconnect();
        delay(1000);
        ESP.restart();
    }

    Serial.println(String("IP: ") + wifi.localIP().toString());

    // init the MQTT connection
    if (!mqtt.begin() && mqtt.attempt() > mqtt_max_attempt)
    {
        Serial.println("MQTT error");
        Serial.print("Waiting and Restaring");
        wifi.disconnect();
        delay(1000);
        ESP.restart();
    }

    ht_sensor.begin();
    if (ht_sensor.error())
        logger_warn("ERROR: sensor read.");
    else
    {

        publish_data_sensor(ht_sensor.getTemp(), ht_sensor.getHumid());
        relay_temp();
        publish_relay_thermostat_state();

        serial_print_current_sensor();
    }

    server.on("/", handleRoot);            // Which routine to handle at root location
    server.on("/action_page", handleForm); // form action is handled here

    server.begin(); // Start server
    Serial.println("HTTP server started");
}

// the loop function runs over and over again forever
void loop()
{

    Serial.println("-");
    if (mqtt.beginLoop())
    {
        if (ht_sensor.beginLoop())
        {
            relay_temp();
            publish_data_sensor(ht_sensor.getTemp(), ht_sensor.getHumid());
        }

        if (ht_sensor.error())
            logger_warn("Failed to read from DHT sensor!");
    }
    else if (mqtt.attempt() > mqtt_max_attempt)
    {
        Serial.println("Cannot connect to mqtt");
        Serial.print("Waiting and Restaring");
        mqtt.reset();
        wifi.disconnect();
        delay(1000);
        ESP.restart();
    }

    serial_print_relay_state();

    server.handleClient(); // Handle client requests

    delay(SLEEPING_TIME_IN_MSECONDS);
}
