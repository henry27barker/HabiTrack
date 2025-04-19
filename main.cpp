#include <WiFi.h>
#include <HttpClient.h>
#include <TFT_eSPI.h> // TTGO T-Display uses this library
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Initialize TFT

char ssid[] = "Salty Crew";    
char pass[] = "JoeTedHen379"; 

const char kHostname[] = "3.141.23.250";
const char kPath[] = ":5000/?var=5";

const int kNetworkTimeout = 30 * 1000;
const int kNetworkDelay = 1000;

unsigned long buttonPressTime = 0;
unsigned long buttonReleaseTime = 0;
const unsigned long longPressDuration = 800;

int lastButtonState = LOW;
bool buttonPressed = false;

// Timer state
bool timerRunning = false;
bool timerPaused = false;
unsigned long startTime = 0;
unsigned long pauseStartTime = 0;
unsigned long pausedTimeTotal = 0;

const int voltagePin = 2;  // Output pin providing 3.3V
const int buttonPin = 37;   // Input pin detecting button press

void displayTime(unsigned long ms);
void handleLongPress();
void handleShortPress();
void sendElapsedTimeToServer(unsigned long elapsedTime);

void setup() {
  Serial.begin(115200);

  // Initialize TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Connecting WiFi...");

  // Setup pins
  pinMode(voltagePin, OUTPUT);
  digitalWrite(voltagePin, HIGH);  // Set voltage output HIGH

  pinMode(buttonPin, INPUT);  // Button input

  delay(1000);
  Serial.println();
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("WiFi connected");
  tft.println(WiFi.localIP());
}

void loop() {
  int currentButtonState = digitalRead(buttonPin);
  unsigned long currentTime = millis();

  if (currentButtonState != lastButtonState) {
    delay(50); // debounce delay
    currentButtonState = digitalRead(buttonPin); // check again
    if (currentButtonState != lastButtonState) {
      lastButtonState = currentButtonState;

      if (currentButtonState == HIGH) {
        // Button pressed
        buttonPressTime = currentTime;
      } else {
        // Button released
        buttonReleaseTime = currentTime;
        unsigned long pressDuration = buttonReleaseTime - buttonPressTime;

        if (pressDuration >= longPressDuration) {
          handleLongPress();
        } else {
          handleShortPress();
        }
      }
    }
  }

  if (timerRunning && !timerPaused) {
    unsigned long currentElapsed = millis() - startTime - pausedTimeTotal;
    displayTime(currentElapsed);
  } else if (timerRunning && timerPaused) {
    unsigned long currentElapsed = pauseStartTime - startTime - pausedTimeTotal;
    displayTime(currentElapsed);
  }
  else {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(50, 50);
    tft.println("Ready");
  }

  delay(10); // small delay to prevent loop spamming
}

void displayTime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  seconds = seconds % 60;
  minutes = minutes % 60;

  char buffer[16];
  sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);

  // Display on TTGO screen
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(50, 50);
  tft.printf(buffer);
}

void handleLongPress() {
  if (!timerRunning) {
    // Start timer
    timerRunning = true;
    timerPaused = false;
    pausedTimeTotal = 0;
    startTime = millis();
    Serial.println("Timer Started");
  } else {
    // Stop timer and send data
    unsigned long endTime = millis();

    if (timerPaused) {
      // Add final pause duration before stopping
      pausedTimeTotal += endTime - pauseStartTime;
    }

    unsigned long elapsedTime = endTime - startTime - pausedTimeTotal;
    Serial.print("Timer Stopped. Elapsed Time: ");
    Serial.println(elapsedTime);

    sendElapsedTimeToServer(elapsedTime);

    // Reset
    timerRunning = false;
    timerPaused = false;
    pausedTimeTotal = 0;
  }
}

void handleShortPress() {
  if (timerRunning) {
    if (!timerPaused) {
      pauseStartTime = millis();
      timerPaused = true;
      Serial.println("Timer Paused");
    } else {
      unsigned long pauseDuration = millis() - pauseStartTime;
      pausedTimeTotal += pauseDuration;
      timerPaused = false;
      Serial.println("Timer Unpaused");
    }
  }
}

void sendElapsedTimeToServer(unsigned long elapsedTime) {
  int err = 0;
  WiFiClient c;
  HttpClient http(c);
  IPAddress ip(3, 141, 23, 250);
  uint16_t port = 5000;
  String url = "http://3.141.23.250:5000/data?elapsedTime=" + String(elapsedTime);

  err = http.get(ip, "", port, url.c_str());
  if (err == 0) {
    Serial.println("startedRequest ok");
    err = http.responseStatusCode();
    if (err >= 0) {
      Serial.print("Got status code: ");
      Serial.println(err);
      err = http.skipResponseHeaders();
      if (err >= 0) {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println("Body returned follows:");

        unsigned long timeoutStart = millis();
        char c;
        while ((http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout)) {
          if (http.available()) {
            c = http.read();
            Serial.print(c);
            bodyLen--;
            timeoutStart = millis();
          } else {
            delay(kNetworkDelay);
          }
        }
      } else {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    } else {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  } else {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
}
