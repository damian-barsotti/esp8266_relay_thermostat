/*
  ESP8266 Relay Temperature by Damian Barsotti
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


#include <DHT.h>

#include "config_local.h" // File for testing outside git
# include "config.h"

ESP8266WebServer server(80);

DHT dht(DHTPIN, DHTTYPE);

float target_temperature = 17.0;
float current_temperature, current_humidity;

bool relay_closed;

void publish_data_sensor(float p_temperature, float p_humidity) {
    current_temperature = p_temperature;
    current_humidity = p_humidity;
}

void serial_print_current_sensor(){
    Serial.print("Current temperature: "); Serial.print(current_temperature);
    Serial.print(", Target temperature: "); Serial.print(target_temperature);
    Serial.print(", Current humidity: "); Serial.println(current_humidity);    
}

bool read_sensors(float &t, float &h){
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  h = dht.readHumidity();

  if (isnan(h) || isnan(t)) {
      h = 0.0; t = 0.0; 
      return false;
  } else {
      // adjust DHT11
      h = humid_slope * h + humid_shift;
      t = temp_slope * t + temp_shift; // Read temperature as C
      return true;
  }
}

bool setup_wifi() {

    // init the WiFi connection
    Serial.println();
    Serial.print("INFO: Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
    
    //  Enable light sleep
    wifi_set_sleep_type(LIGHT_SLEEP_T);
    
    if (local_IP != IPAddress(0,0,0,0) && !WiFi.config(local_IP, gateway, subnet)){
        Serial.println("STA Failed to configure fixed IP");
        return false;    
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    static const int max_attempt = 100;
    int attempt = 0;
    while (attempt < max_attempt && WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempt++;
    }
    Serial.println("");

    return (attempt < max_attempt);
}

void relay_temp(){

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

//  <input type="range" id="target_temp" name="target_temp" min="16" max="30" value="17" onchange="updateTextInput(this.value);">
  // <input type="text" id="textInput" value="">
  // <script type="text/javascript">
  //       function updateTextInput(val) {
  //         document.getElementById('textInput').value=val; 
  //       }
  // </script>

const char GO_back[] PROGMEM = "<a href='/'> Go Back </a>";

//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot() {
    char MAIN_page[600];
    char *str_relay_state;

    if (relay_closed)
        str_relay_state = "closed";
    else
        str_relay_state = "open";



    snprintf(MAIN_page, 600, MAIN_page_template, current_temperature, current_humidity, 
             target_temperature, target_temperature, str_relay_state);
    Serial.println(strlen(MAIN_page));
    server.send(200, "text/html", MAIN_page); //Send web page
}

//===============================================================
// This routine is executed when you press submit
//===============================================================
void handleForm() {

    float send_temperature = server.arg("target_temp").toFloat();

    Serial.print("Received temp: "); Serial.println(send_temperature);
    Serial.print("Actual target temp: "); Serial.println(target_temperature);
    
    if (send_temperature != target_temperature){
        target_temperature = send_temperature ; 
        relay_temp();
    }

    Serial.print("Target Temperature: "); Serial.println(target_temperature);
    
    server.send(200, "text/html", GO_back); //Send web page
}

int last_sensor_read_time;
int last_avg_sensor_read_time;

void setup() {
    float temperature, humidity;

    // init the serial
    Serial.begin(115200);
    //Take some time to open up the Serial Monitor
    delay(1000);
    //pinMode(RELAYPIN, OUTPUT);
    
    dht.begin();

    pinMode(RELAYPIN, OUTPUT);

    // Restart ESP if max attempt reached
    if (!setup_wifi()){
        Serial.println("ERROR: max_attempt reached to WiFi connect");
        Serial.print("Waiting and Restaring");
        WiFi.disconnect();
        delay(1000);
        ESP.restart();
    }

    read_sensors(temperature, humidity);
    publish_data_sensor(temperature, humidity);
    serial_print_current_sensor();

    server.on("/", handleRoot);      //Which routine to handle at root location
    server.on("/action_page", handleForm); //form action is handled here

    server.begin();                  //Start server
    Serial.println("HTTP server started");

    last_sensor_read_time = 0;
    last_avg_sensor_read_time = 0;
}

int n_sensor_reads = 0;
float ac_t = 0.0, ac_h = 0.0;

// the loop function runs over and over again forever
void loop() {
    float t, h;

    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    if (last_sensor_read_time >= 2000){

        if (read_sensors(t, h)){
            //Serial.print("read temp: "); Serial.print(t);
            //Serial.print("read humid: "); Serial.println(h);            
            n_sensor_reads++;
            ac_t += t;
            ac_h += h;
        } else {
            Serial.print("Failed read "); Serial.print(n_sensor_reads); Serial.println( " DHT sensor");
        }

        last_sensor_read_time = 0;
    }

    if (last_avg_sensor_read_time > READ_AVG_SENSOR_TIME_IN_SECONDS*1000){

        if (n_sensor_reads != 0){
            // Average reads
            publish_data_sensor(ac_t/n_sensor_reads, ac_h/n_sensor_reads);
            serial_print_current_sensor();
            relay_temp();
        }
        else
            Serial.println("Failed to read from DHT sensor!");

        ac_t=0.0; ac_h=0.0;
        n_sensor_reads = 0;

        //relay_temp();
        last_avg_sensor_read_time = 0;
    } 

    server.handleClient();          //Handle client requests

    last_sensor_read_time += SLEEPING_TIME_IN_MSECONDS; 
    last_avg_sensor_read_time += SLEEPING_TIME_IN_MSECONDS;
    delay(SLEEPING_TIME_IN_MSECONDS);
}
