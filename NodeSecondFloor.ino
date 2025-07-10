#include <WiFi.h>
#include <WebSocketsClient.h>

const char* ssid = "GlobeAtHome_17400";
const char* password = "3ML454H26Q5";
const char* host = "192.168.254.110";  // Central ESP32 IP
const int port = 81;

WebSocketsClient webSocket;

// Sensor pins for Slot3 and Slot4 (change pins if needed)
#define TRIG3 26
#define ECHO3 27
#define TRIG4 32
#define ECHO4 33

// LED pins for Sensor 3
#define RED_LED3 5
#define GREEN_LED3 18

// LED pins for Sensor 4
#define RED_LED4 19
#define GREEN_LED4 21


unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG3, OUTPUT);
  pinMode(ECHO3, INPUT);
  pinMode(TRIG4, OUTPUT);
  pinMode(ECHO4, INPUT);

  pinMode(RED_LED3, OUTPUT);
  pinMode(GREEN_LED3, OUTPUT);
  pinMode(RED_LED4, OUTPUT);
  pinMode(GREEN_LED4, OUTPUT);

  connectWiFi();

  // Setup WebSocket connection
  webSocket.begin(host, port, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // Auto reconnect every 5 seconds if disconnected
}

void loop() {
  // Reconnect WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    connectWiFi();
  }

  // Handle WebSocket connection and events
  webSocket.loop();

  // If not connected, just wait for reconnect interval (do NOT call begin() again here)
  if (!webSocket.isConnected()) {
    Serial.println("WebSocket not connected, waiting for reconnect...");
  }

  // Send sensor data every 'sendInterval' milliseconds (non-blocking)
  if (millis() - lastSendTime > sendInterval) {
    lastSendTime = millis();

    long dist3 = readDistanceCM(TRIG3, ECHO3);
    long dist4 = readDistanceCM(TRIG4, ECHO4);

    Serial.print("Distance Slot3: ");
    Serial.print(dist3);
    Serial.print(" cm, Slot4: ");
    Serial.println(dist4);

    // LED Control for Slot3
    if (dist3 < 12) {
      digitalWrite(RED_LED3, HIGH);
      digitalWrite(GREEN_LED3, LOW);
    } else {
      digitalWrite(RED_LED3, LOW);
      digitalWrite(GREEN_LED3, HIGH);
    }

    // LED Control for Slot4
    if (dist4 < 12) {
      digitalWrite(RED_LED4, HIGH);
      digitalWrite(GREEN_LED4, LOW);
    } else {
      digitalWrite(RED_LED4, LOW);
      digitalWrite(GREEN_LED4, HIGH);
    }

    String msg = "Slot3:" + String(dist3 < 12 ? "Occupied" : "Available") +
                 " | Slot4:" + String(dist4 < 12 ? "Occupied" : "Available");

    Serial.println("Sending: " + msg);
    webSocket.sendTXT(msg);
  }
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

long readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 999; // timeout means no echo detected
  return duration * 0.034 / 2;
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.println("Connected to central WebSocket server");
      break;
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from server");
      break;
    case WStype_TEXT:
      Serial.print("Message from server: ");
      Serial.println((char*)payload);
      break;
    case WStype_ERROR:
      Serial.println("WebSocket error");
      break;
    default:
      break;
  }
}