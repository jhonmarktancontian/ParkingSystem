#include <WiFi.h>
#include <WebSocketsClient.h>

const char* ssid = "GlobeAtHome_17400";
const char* password = "3ML454H26Q5";
const char* host = "192.168.254.110";  // Central ESP32 IP
const int port = 81;

WebSocketsClient webSocket;

// Sensor pins for Slot5 and Slot6
#define TRIG5 26
#define ECHO5 27
#define TRIG6 32
#define ECHO6 33

// LED pins for Sensor 5
#define RED_LED5 5
#define GREEN_LED5 18

// LED pins for Sensor 6
#define RED_LED6 19
#define GREEN_LED6 21


unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG5, OUTPUT);
  pinMode(ECHO5, INPUT);
  pinMode(TRIG6, OUTPUT);
  pinMode(ECHO6, INPUT);

  pinMode(RED_LED5, OUTPUT);
  pinMode(GREEN_LED5, OUTPUT);
  pinMode(RED_LED6, OUTPUT);
  pinMode(GREEN_LED6, OUTPUT);

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

    long dist5 = readDistanceCM(TRIG5, ECHO5);
    long dist6 = readDistanceCM(TRIG6, ECHO6);

    Serial.print("Distance Slot5: ");
    Serial.print(dist5);
    Serial.print(" cm, Slot6: ");
    Serial.println(dist6);

    // LED Control for Slot5
    if (dist5 < 12) {
      digitalWrite(RED_LED5, HIGH);
      digitalWrite(GREEN_LED5, LOW);
    } else {
      digitalWrite(RED_LED5, LOW);
      digitalWrite(GREEN_LED5, HIGH);
    }

    // LED Control for Slot6
    if (dist6 < 12) {
      digitalWrite(RED_LED6, HIGH);
      digitalWrite(GREEN_LED6, LOW);
    } else {
      digitalWrite(RED_LED6, LOW);
      digitalWrite(GREEN_LED6, HIGH);
    }

    String msg = "Slot5:" + String(dist5 < 12 ? "Occupied" : "Available") +
                 " | Slot6:" + String(dist6 < 12 ? "Occupied" : "Available");

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