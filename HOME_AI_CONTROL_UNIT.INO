#include <WiFi.h>

#define KITCHEN_LIGHTS 12 
#define BEDROOM 13
#define TV 14
#define HEATER 27
#define DOOR 26

const char* ssid = "******";         
const char* password = "******";

WiFiServer server(80);  // Create a server that listens on port 80

void setup() {
  Serial.begin(115200);

  // Set up the GPIO pins for the actuations
  pinMode(KITCHEN_LIGHTS, OUTPUT);
  pinMode(BEDROOM, OUTPUT);
  pinMode(TV ,OUTPUT);
  pinMode(HEATER ,OUTPUT);
  pinMode(DOOR ,OUTPUT);

  // Initialize all pins to OFF
  digitalWrite(KITCHEN_LIGHTS, LOW);
  digitalWrite(BEDROOM, LOW);
  digitalWrite(TV, LOW);
  digitalWrite(HEATER, LOW);
  digitalWrite(DOOR, LOW);

  // Connect to the WiFi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to wifi ... ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Device Connected.");
  Serial.print("The IP: ");
    Serial.println(WiFi.localIP());
  
  // Start the server
  server.begin();
  Serial.println("Server started, waiting for commands...");
}

void loop() {
  // Check if a client is connected
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client connected");
    
    
    // Wait for the client to send data
    String command = "";
    while (client.connected() && client.available()) {
      char c = client.read();
      command += c;
    }

    command.toLowerCase();  // Ensure the command is in lowercase
    Serial.println("Received command: " + command);

    // Actuation logic based on the received command
    if (command.indexOf("kitchen lights on") != -1) {
      digitalWrite(KITCHEN_LIGHTS, HIGH);  // Turn on kitchen lights
      Serial.println("Kitchen lights turned ON.");
    } 
    else if (command.indexOf("kitchen lights off") != -1) {
      digitalWrite(KITCHEN_LIGHTS, LOW);  // Turn off kitchen lights
      Serial.println("Kitchen lights turned OFF.");
    }

    if (command.indexOf("bedroom lights on") != -1) {
      digitalWrite(BEDROOM, HIGH);  // Turn on bedroom lights
      Serial.println("Bedroom lights turned ON.");
    } 
    else if (command.indexOf("bedroom lights off") != -1) {
      digitalWrite(BEDROOM, LOW);  // Turn off bedroom lights
      Serial.println("Bedroom lights turned OFF.");
    }

    if (command.indexOf("tv on") != -1) {
      digitalWrite(TV, HIGH);  // Turn on TV
      Serial.println("TV turned ON.");
    } 
    else if (command.indexOf("tv off") != -1) {
      digitalWrite(TV, LOW);  // Turn off TV
      Serial.println("TV turned OFF.");
    }

    if (command.indexOf("fan on") != -1) {
      digitalWrite(HEATER, HIGH);  // Turn on heater
      Serial.println("Heater turned ON.");
    } 
    else if (command.indexOf("fan off") != -1) {
      digitalWrite(HEATER, LOW);  // Turn off heater
      Serial.println("Heater turned OFF.");
    }

    if (command.indexOf("lock the door") != -1) {
      digitalWrite(DOOR, HIGH);  // Lock the door
      Serial.println("Door Locked.");
    } 
    else if (command.indexOf("unlock the door") != -1) {
      digitalWrite(DOOR, LOW);  // Unlock the door
      Serial.println("Door Unlocked.");
    }

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
  }
}
