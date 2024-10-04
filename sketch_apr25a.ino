#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <math.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD I2C address
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust the address 0x27 if needed and set the correct column and row count

// GPS Module connections
SoftwareSerial GPS_SoftSerial(4, 3); // (Rx, Tx)
TinyGPSPlus gps;

// Define the sensor threshold values
// Define the raw value threshold limits for each axis
const int xThreshold = 300; // Example threshold value for the X-axis
const int yThreshold = 300; // Example threshold value for the Y-axis
const int zThreshold = 300; // Example threshold value for the Z-axis
const int eyeBlinkThreshold = 1; // Assuming HIGH signal indicates a blink
const int alcoholThreshold = 400; // Threshold value for alcohol detection


// Accelerometer connections
const int x_out = A1;
const int y_out = A2;
const int z_out = A3;
const int buzzerPin = 9; // Connect the positive pin of the buzzer to digital pin 9

// Eye Blink Sensor connection
const int eyeBlinkPin = 7;

// MQ-3 Alcohol Sensor connections
const int mq3AnalogPin = A0; // MQ-3 analog output connected to A1
//const int mq3DigitalPin = 2; // MQ-3 digital output connected to D2

// GSM Module connections
SoftwareSerial GSM_SoftSerial(5, 6); // (Rx, Tx)

// GPS variables
volatile int degree, mins, secs;
double lat_val, lng_val; // Declare lat_val and lng_val globally

// Function to convert decimal degrees to degrees, minutes, seconds
void DegMinSec(double tot_val) {
  degree = (int)tot_val;
  double minutes = (tot_val - degree) * 60;
  mins = (int)minutes;
  double seconds = (minutes - mins) * 60;
  secs = (int)seconds;
}

// Function to process GPS data with a delay
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (GPS_SoftSerial.available())
      gps.encode(GPS_SoftSerial.read());
  } while (millis() - start < ms);
}

// Function to send AT commands to the GSM module
void sendATCommand(const char* command, const char* response, unsigned long timeout) {
  GSM_SoftSerial.println(command); // Send the AT command
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    if (GSM_SoftSerial.find((char*)response)) { // Cast response to char* to match find method signature
      Serial.println("Response received");
      return;
    }
  }
  Serial.println("Timeout waiting for response");
}

// Function to setup the GSM module
void setupGSM() {
  sendATCommand("AT", "OK", 2000); // Check communication
  sendATCommand("ATE0", "OK", 2000); // Turn off echo
  sendATCommand("AT+CPIN?", "READY", 2000); // Check if the SIM card is unlocked
  sendATCommand("AT+CMGF=1", "OK", 2000); // Set SMS text mode
  sendATCommand("AT+CNMI=2,2,0,0,0", "OK", 2000); // Set module to send SMS data to serial out upon receipt
}

void setup() {
    // Initialize the LCD
  lcd.init();
  lcd.backlight();

  Serial.begin(9600);
  pinMode(buzzerPin, OUTPUT);
//  pinMode(mq3DigitalPin, INPUT);
  pinMode(eyeBlinkPin, INPUT);
  GPS_SoftSerial.begin(9600);
  GSM_SoftSerial.begin(9600);
  setupGSM(); // Setup GSM module
}

void loop() {
  // Read the MQ-3 sensor values
  int alcoholValueAnalog = analogRead(mq3AnalogPin); // Read the analog value

  // Print the MQ-3 sensor values
  Serial.print("Alcohol Analog Value: ");
  Serial.println(alcoholValueAnalog);

  // Read the eye blink sensor value
  int blinkValue = digitalRead(eyeBlinkPin);

  // GPS data processing
  smartDelay(1000);
  if (gps.location.isValid()) {
    lat_val = gps.location.lat();
    lng_val = gps.location.lng();
    DegMinSec(lat_val);
    DegMinSec(lng_val);
    // Print GPS data
    Serial.print("Latitude in Decimal Degrees: ");
    Serial.println(lat_val, 6);
    Serial.print("Longitude in Decimal Degrees: ");
    Serial.println(lng_val, 6);
  } else {
    // Print invalid GPS data message
    Serial.println("Invalid GPS data");
  }

  // Accelerometer data processing
  int xRaw = analogRead(x_out);
  int yRaw = analogRead(y_out);
  int zRaw = analogRead(z_out);
  
  // Print raw accelerometer values
  Serial.print("X Raw: ");
  Serial.print(xRaw);
  Serial.print(" Y Raw: ");
  Serial.print(yRaw);
  Serial.print(" Z Raw: ");
  Serial.println(zRaw);

  // Check if any axis exceeds its threshold and activate the buzzer
  if (xRaw > xThreshold || yRaw > yThreshold || zRaw > zThreshold) {
    lcd.clear();
    lcd.print("Accident");
    lcd.setCursor(0, 1);
    lcd.print("Detected!");
    Serial.println("**Movement detected - possible accident!**");
    digitalWrite(buzzerPin, HIGH); // Turn on the buzzer
    delay(1000); // Buzzer on for 1 second
    digitalWrite(buzzerPin, LOW); // Turn off the buzzer
  }

  // Check for alcohol intoxication
  if (alcoholValueAnalog > alcoholThreshold) {
    lcd.clear();
    lcd.print("Intoxication");
    lcd.setCursor(0, 1); // Move to the start of the second line
    lcd.print("Detected!");
    Serial.println("**Alcohol intoxication detected!**");
    digitalWrite(buzzerPin, HIGH); // Turn on the buzzer
    delay(1000); // Buzzer on for 1 second
    digitalWrite(buzzerPin, LOW); // Turn off the buzzer
  } else {
    Serial.println("No alcohol detected.");
  }

  // Check for driver drowsiness
  if (blinkValue == eyeBlinkThreshold) {
    lcd.clear();
    lcd.print("Driver Drowsiness");
    lcd.setCursor(0, 1);
    lcd.print("Detected!");
    Serial.println("**Driver drowsiness detected!**");
    digitalWrite(buzzerPin, HIGH); // Turn on the buzzer
    delay(1000); // Buzzer on for 1 second
    digitalWrite(buzzerPin, LOW); // Turn off the buzzer
  }

  // The following GSM code should be inside a conditional block if you want to send an SMS for a specific event
  // For example, if an accident is detected:
  if (xRaw > xThreshold || yRaw > yThreshold || zRaw > zThreshold) {
    // Send SMS or make a call using GSM module
    GSM_SoftSerial.println("AT+CMGF=1"); // Set GSM module to SMS mode
    delay(1000);
    GSM_SoftSerial.println("AT+CMGS=\"+918815088910\""); // Replace with the phone number you want to text
    delay(1000);
    GSM_SoftSerial.print("Accident detected! Latitude: ");
    GSM_SoftSerial.print(lat_val, 6);
    GSM_SoftSerial.print(", Longitude: ");
    GSM_SoftSerial.println(lng_val, 6); // SMS content
    delay(1000);
    GSM_SoftSerial.write(26); // ASCII code for CTRL+Z to send SMS
  }

  delay(1000); // Delay to make the output readable
}
