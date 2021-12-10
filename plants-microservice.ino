// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

WiFiClient wifiClient;

// Replace with your network credentials
const char* ssid     = "Vodafone-65256734";
const char* password = "e3f75mptt6pfwvs";

// Set web server port number to 80
WiFiServer server(4000);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "off";

// Assign output variables to GPIO pins
const int output5 = 4;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

unsigned long currentMillis;
unsigned long previousMillis = 0;
const long timeoutAfterResendRegistration = 5*60*1000; // 5 min

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output5, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output5, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  registerToGateway();
}

void loop(){
  currentMillis = millis();
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:application/json");
            client.println("Access-Control-Allow-Origin: *");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /activate") >= 0) {
              Serial.println("GPIO 5 on");
              output5State = "on";
              digitalWrite(output5, HIGH);
            } else if (header.indexOf("GET /deactivate") >= 0) {
              Serial.println("GPIO 5 off");
              output5State = "off";
              digitalWrite(output5, LOW);
            }
            
            // Display the HTML web page
            client.println("{\"status\":\"ok\",\"response\":\""+output5State+"\"}");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  // Ogni 5 min
  if (currentMillis - previousMillis >= timeoutAfterResendRegistration) {
    registerToGateway();
    output5State = "off";
    digitalWrite(output5, LOW);
    previousMillis = currentMillis;
  }
}

void registerToGateway() {
  HTTPClient http;    //Declare object of class HTTPClient
 
  http.begin(wifiClient, "http://192.168.1.100:3000/register");      //Specify request destination
  http.addHeader("Access-Control-Allow-Origin", "*");  //Specify content-type header
  http.addHeader("Content-type", "application/json");

  int httpCode = http.POST("{\"ip\":\""+WiFi.localIP().toString()+"\",\"name\":\"plants-microservice\"}");   //Send the request
  String payload = http.getString();                  //Get the response payload

  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload

  http.end();  //Close connection
}
