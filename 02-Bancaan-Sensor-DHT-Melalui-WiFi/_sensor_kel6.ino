#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h> // Library for the Web Server
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

const char* ssid     = "HOTSPOT@UPNJATIM.AC.ID";
const char* password = "belanegara";

// Hardware Pins
#define DHTPIN 0      // D3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Initialize the web server on port 80
ESP8266WebServer server(80);

void handleRoot() {
  digitalWrite(LED_BUILTIN, LOW); // Blink LED when someone visits the page
  
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Create a simple HTML page
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name='viewport' content='width=device-width, initial-scale=1'>
<meta http-equiv='refresh' content='5'>
<title>Kelompok 6 - Monitoring</title>

<style>
    body {
        font-family: Arial;
        text-align: center;
        background: linear-gradient(to right, #4facfe, #00f2fe);
        color: white;
        margin: 0;
        padding: 0;
    }
    .container {
        margin-top: 50px;
        background: rgba(0,0,0,0.3);
        padding: 20px;
        border-radius: 15px;
        width: 300px;
        margin-left: auto;
        margin-right: auto;
        box-shadow: 0px 4px 15px rgba(0,0,0,0.3);
    }
    h1 {
        margin-bottom: 5px;
    }
    h3 {
        margin-top: 0;
        font-weight: normal;
    }
    .data {
        font-size: 24px;
        margin: 15px 0;
    }
    .footer {
        margin-top: 20px;
        font-size: 12px;
        opacity: 0.8;
    }
</style>

</head>

<body>

<div class="container">
    <h1>🌡️ Monitoring Sensor</h1>
    <h3>Kelompok 6</h3>

    <div class="data">
        Suhu: <b>%TEMP% &deg;C</b>
    </div>

    <div class="data">
        Kelembapan: <b>%HUM% %</b>
    </div>

    <div class="footer">
        IoT Project - ESP8266<br>
        Update setiap 5 detik
    </div>
</div>

</body>
</html>
)rawliteral";

html.replace("%TEMP%", String(t));
html.replace("%HUM%", String(h));

  server.send(200, "text/html", html);
  
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

void handleRootJSON() {
  // Blink LED to indicate data transmission/request
  digitalWrite(LED_BUILTIN, LOW); 
  
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Create a JSON string (Machine-friendly)
  // Format: {"temperature": 25.5, "humidity": 60.0}
  String json = "{";
  json += "\"temperature\": " + String(t) + ",";
  json += "\"humidity\": " + String(h);
  json += "}";

  // Send with "application/json" header instead of "text/html"
  server.send(200, "application/json", json);
  
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}


void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  dht.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  // Define the "Home" route of the web server
  server.on("/", handleRoot);
  server.begin();

  // Show the IP address on the OLED (You need this to access the data!)
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("IP Address:");
  display.setTextSize(2);
  display.setCursor(0,20);
  display.println(WiFi.localIP());
  display.display();
}

void loop() {
  server.handleClient(); // Listen for web browsers
}