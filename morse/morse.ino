#include <WiFiS3.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "arduino_secrets.h"
#include <map>

// WiFi credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* raspberryPiIP = SECRET_IP; 
const int raspberryPiPort = 8080;

// Pin definitions for TFT display and LED
const int TFT_CS = 10;
const int TFT_DC = 9;
const int TFT_RST = 8;
const int LED_PIN = 3;
const int BUTTON_PIN = 2;

#define DOT_DURATION 200
#define DASH_DURATION 600
#define CHAR_GAP 200
#define LETTER_GAP 600
#define WORD_GAP 1400

WiFiServer server(80);
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Morse Code Dictionary
std::map<String, char> morseCodeDict = {
    {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'}, {"..-.", 'F'},
    {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'}, {"-.-", 'K'}, {".-..", 'L'},
    {"--", 'M'}, {"-.", 'N'}, {"---", 'O'}, {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'},
    {"...", 'S'}, {"-", 'T'}, {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'},
    {"-.--", 'Y'}, {"--..", 'Z'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
    {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'},
    {"----.", '9'}, {"-----", '0'}, {"/", ' '}
};

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; } // Wait for serial port to connect

  // Initialize TFT display
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Connect to WiFi
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    delay(10000);
  }

  Serial.println("Connected to WiFi");  
  tft.println("Ready");
  printWifiData();
  server.begin();
}

void loop() {
  // Flags for space and slash characters
  static bool letterGapAdded = false; 
  static bool wordGapAdded = false;

  // Receiving messages from the Raspberry Pi
  WiFiClient client = server.available();
  if (client) {
    String message = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        message += c;
        if (c == '\n') {
          Serial.print("Received: ");
          Serial.println(message);
          String decodedMessage = decodeMorse(message);
          Serial.print("Decoded: ");
          Serial.println(decodedMessage);

          // Clear the display and show both Morse code and decoded message
          tft.fillScreen(ST77XX_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(ST77XX_WHITE);
          tft.setTextSize(2);
          tft.println("Morse:");
          tft.setTextSize(1); // Smaller text size for Morse code
          tft.println(message);
          tft.setTextSize(2);
          tft.println("Decoded:");
          tft.setTextSize(1); // Smaller text size for decoded message
          tft.println(decodedMessage);
          blinkMorse(message);
          message = "";
        }
      }
    }
    client.stop();
  }

  // Transmit Morse code with button
  static unsigned long pressStartTime = 0;
  static unsigned long lastReleaseTime = 0;
  static bool buttonPressed = false;
  static String morseMessage = "";

  if (digitalRead(BUTTON_PIN) == HIGH) { 
    if (!buttonPressed) { // executes once per button press
      pressStartTime = millis();
      buttonPressed = true;
      digitalWrite(LED_PIN, HIGH);
      letterGapAdded = false;  // Reset letterGapAdded when starting a new letter
      wordGapAdded = false;    // Reset wordGapAdded when starting a new letter
    }
  } else { // case where button reads low (released)
    if (buttonPressed) { // Button release logic
      unsigned long pressDuration = millis() - pressStartTime;
      if (pressDuration < 500) { // Dot
        morseMessage += ".";
      } else { // Dash
        morseMessage += "-";
      }
      digitalWrite(LED_PIN, LOW); 
      buttonPressed = false;
      lastReleaseTime = millis();
    } else { // Check for gaps between characters and words
      unsigned long gapDuration = millis() - lastReleaseTime;
      // if the gap is greater than LETTER_GAP and no letter gap yet
      if (gapDuration > LETTER_GAP && morseMessage.length() > 0 && !letterGapAdded) { 
        morseMessage += " "; // append space between letters
        letterGapAdded = true;
      }
      // if gap duration greater than WORD_GAP
      if (gapDuration > WORD_GAP && morseMessage.length() > 0 && !wordGapAdded) {
        morseMessage += "/ "; // append slash and space to separate word
        wordGapAdded = true;

        // Send the Morse message to the Raspberry Pi        
        WiFiClient client;
        if (client.connect(raspberryPiIP, raspberryPiPort)) {
          client.println(morseMessage);
          client.stop();
          Serial.println("Sent to Raspberry Pi: " + morseMessage);
          morseMessage = "";
          letterGapAdded = false;
          wordGapAdded = false;
        } else {
          Serial.println("Failed to connect to the Raspberry Pi");
        }
      }
    }
  }
}

// print board's IP address
void printWifiData() {
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

// Function to handle and decode morse message
String decodeMorse(String message) {
  String decodedMessage = "";
  String runningSeq = "";
  for (char c : message) {
    if (c != ' ' && c != '\n' && c != '/') {
      runningSeq += c;
    } else {
      if (morseCodeDict.count(runningSeq)) {
        decodedMessage += morseCodeDict[runningSeq];
      } else if (runningSeq.length() > 0) {
        Serial.print("Unrecognized sequence: ");
        Serial.println(runningSeq);
      }
      if (c == '/') {
        decodedMessage += ' ';
      }
      runningSeq = "";
    }
  }
  if (morseCodeDict.count(runningSeq)) {
    decodedMessage += morseCodeDict[runningSeq];
  } else if (runningSeq.length() > 0) {
    Serial.print("Unrecognized sequence at end: ");
    Serial.println(runningSeq);
  }
  return decodedMessage;
}

// Function to blink morse message to LED
void blinkMorse(String message) {
  for(char c : message) {
    if(c == '.') {
      digitalWrite(LED_PIN, HIGH);
      delay(DOT_DURATION);
      digitalWrite(LED_PIN, LOW);
      delay(CHAR_GAP); 
    } else if(c == '-') {
      digitalWrite(LED_PIN, HIGH);
      delay(DASH_DURATION);
      digitalWrite(LED_PIN, LOW);
      delay(CHAR_GAP); 
    } else if(c == ' ') {
      delay(LETTER_GAP);
    } else if (c == '/') {
      delay(WORD_GAP);
    }
  }
}
