#include <WiFiEsp.h>
//#include "MacEEPROM.h"
#include "SoftwareSerial.h"

SoftwareSerial esp(6, 7); // RX, TX


//MacEEPROM macEEPROM;
int STATUS_LED = 4;
int BULB_PIN = 5;
int TIMED_RELAY = 9;
int SOLENOID_RELAY = 10;
int TV_RELAY = 11;
int FAN_RELAY = 12;
int AC_RELAY = 13;


int status = WL_IDLE_STATUS;     // the Wifi radio's status
unsigned long connectionLastChecked;                // time when wifi connection was last checked
unsigned long connectionCheckInterval = 120000;      // interval to check wifi connection (ms)
unsigned long lastLocked; // time when solenoid was last locked
unsigned long acLastOff;
unsigned long acCooldown = 180000;
unsigned long lockOpenDuration = 4000; // duration to keep the lock open (ms)
unsigned long timedOutletDuration;
unsigned long timedOutletStart;
int reqCount = 0;                // number of requests received
int clientTimeout = 2000;


int lampBrightness = 0;
String doorLockPin = "3573";

char ssid[] = "IncognitoWiFi";            // your network SSID (name)
char pass[] = "incognitoSecretPassword";        // your network password

IPAddress ip(192, 168, 1, 223);

WiFiEspServer server(333);


void setup()
{
  // initialize serial for debugging
  Serial.begin(9600);
  // initialize serial for ESP module
  esp.begin(9600);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(SOLENOID_RELAY, OUTPUT);
  pinMode(FAN_RELAY, OUTPUT);
  pinMode(BULB_PIN, OUTPUT);
  pinMode(AC_RELAY, OUTPUT);
  pinMode(TIMED_RELAY, OUTPUT);
  pinMode(TV_RELAY, OUTPUT);
  // relays activate when INPUT is set to LOW, so deactivate relays initially
  digitalWrite(FAN_RELAY, HIGH);
  digitalWrite(TIMED_RELAY, HIGH);
  digitalWrite(TV_RELAY, HIGH);
  digitalWrite(SOLENOID_RELAY, HIGH);

  digitalWrite(AC_RELAY, LOW);
  digitalWrite(BULB_PIN, LOW);

  // initialize ESP module
  WiFi.init(&esp);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true) {
      digitalWrite(STATUS_LED, HIGH);
      delay(500);
      digitalWrite(STATUS_LED, LOW);
      delay(500);
    }
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    WiFi.config(ip);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");

  printWifiStatus();
  lastLocked = 0;
  acLastOff = 0;
  timedOutletDuration = 0;
  timedOutletStart = 0;
  connectionLastChecked = millis();
  // start the web server on port 333
  server.begin();
  // turn on status led
  digitalWrite(STATUS_LED, HIGH);
}

int FAN_STATUS = LOW;
int TV_STATUS = LOW;
int AC_STATUS = LOW;
int LOCK_STATUS = LOW;
int TIMED_STATUS = LOW;
void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - connectionLastChecked >= connectionCheckInterval) {
    status = WiFi.status();
    while ( status != WL_CONNECTED) {
      currentMillis = millis();
      // doorlock timer
      checkDoorLockTimer(currentMillis);
      Serial.println("test");
      // timed outlet timer
      checkTimedOutletTimer(currentMillis);
      digitalWrite(STATUS_LED, LOW);
      Serial.print("Attempting to connect to WPA SSID: ");
      Serial.println(ssid);
      WiFi.config(ip);
      // Connect to WPA/WPA2 network
      status = WiFi.begin(ssid, pass);
    }
    digitalWrite(STATUS_LED, HIGH);

    Serial.println(status);
    Serial.println(millis());
    Serial.println(connectionLastChecked);
    Serial.println(millis() - connectionLastChecked);
    Serial.println("checking connection");
    connectionLastChecked = currentMillis;
  }
  // doorlock timer
  checkDoorLockTimer(currentMillis);

  // timed outlet timer
  checkTimedOutletTimer(currentMillis);

  unsigned long acCooldownRemaining = acLastOff == 0 || acCooldown < currentMillis - acLastOff ? 0 : acCooldown - (currentMillis - acLastOff);
  unsigned long timedTimeRemaining = timedOutletStart == 0 ? 0 : timedOutletDuration - (currentMillis - timedOutletStart);

  String request;
  // listen for incoming clients
  WiFiEspClient client = server.available();
  if (client) {


    unsigned long timeSinceLastRequest = millis();
    Serial.println("New client");

    boolean closeConnection = true;


    while (client.connected()) {
      request = "";
      //Serial.println(client.c  onnected());
      //Serial.println(client.available());
      while (client.available()) {
        char c = client.read();
        if (c == 'Z') {
          Serial.println("more words");
          while (client.available() > 0) {
            char t = client.read();
            Serial.print(t);
          }
          break;
        }
        request += c;

      }

      // check when client hasnt sent a request in a while
      if (millis() - timeSinceLastRequest > clientTimeout) {
        client.stop();
        Serial.println("Client timeout");
      }


      if (request[0] == 'X') {
        Serial.println(request);
        //Serial.println("printed");
        timeSinceLastRequest = millis();
        // Handshake
        if (request.substring(0, 10) == "XHANDSHAKE") {

          //String address = request.substring(11);
          //unsigned char hex[12];
          //mac clientMac;
          //for (int i = 0; i < 12; i++) {
          //  hex[i] = address[i];
          //}
          //hexStringToHex(hex, clientMac.mac);
          //if (arrayIncludeElement(authenticatedDevices, address)) {
          //if (macEEPROM.macExists(clientMac)){
            client.print("FAN:");
            client.print(FAN_STATUS);
            client.print("::");
            client.print("LAMP:");
            client.print(lampBrightness);
            client.print("::");
            client.print("TV:");
            client.print(TV_STATUS);
            client.print("::");
            client.print("AC:");
            client.print(AC_STATUS);
            client.print("::");
            client.print("ACCL:");
            client.print(acCooldownRemaining);
            client.print("::");
            client.print("ACCD:");
            client.print(acCooldown);
            client.print("::");
            client.print("TIMED:");
            client.print(timedTimeRemaining);
            client.print("::");
            client.print("TIMEDP:");
            client.println(TIMED_STATUS);
          //}
          //else
            //client.println("400");

          Serial.println(request);
          break;
        }
        //        else if(request.substring(0, 7) == "XADDMAC") {
        //          String address = request.substring(8);
        //          unsigned char hexString[12];
        //
        //          for(int i = 0; i < 12; i++) {
        //            hexString[i] = address[i];
        //
        //          }
        //          for(int i = 0; i < 12; i++) {
        //            Serial.print(hexString[i]);
        //            Serial.print(' ');
        //          }
        //          Serial.println();
        //          unsigned char mac[6];
        //          hexStringToHex(hexString, mac);
        //          for(int i = 0; i < 6; i++) {
        //            Serial.print(mac[i],BIN);
        //            Serial.print(' ');
        //          }
        //          Serial.println();
        //        }
        // Fan
        else if (request == "XFANTOGGLE") {
          if (FAN_STATUS == LOW) {
            digitalWrite(FAN_RELAY, LOW);
            FAN_STATUS = HIGH;
          }
          else {
            digitalWrite(FAN_RELAY, HIGH);
            FAN_STATUS = LOW;
          }
          Serial.println(FAN_STATUS);
          // Send fan status to client
          client.println(FAN_STATUS);
          break;
        }
        // TV
        else if (request == "XTVTOGGLE") {
          if (TV_STATUS == LOW) {
            digitalWrite(TV_RELAY, LOW);
            TV_STATUS = HIGH;
          }
          else {
            digitalWrite(TV_RELAY, HIGH);
            TV_STATUS = LOW;
          }
          Serial.println(TV_STATUS);
          // Send TV status to client
          client.println(TV_STATUS);
          break;
        }
        // AC
        else if (request == "XACTOGGLE") {
          if (acLastOff == 0 || currentMillis - acLastOff >= acCooldown) {

            if (AC_STATUS == LOW) {
              digitalWrite(AC_RELAY, HIGH);
              AC_STATUS = HIGH;
              acLastOff = 0;
              Serial.println(AC_STATUS);
              client.println(AC_STATUS);
            }
            else {
              digitalWrite(AC_RELAY, LOW);
              AC_STATUS = LOW;
              acLastOff = millis();
              Serial.println(AC_STATUS);
              client.println(AC_STATUS);
            }
          }
          else {
            Serial.println(acCooldownRemaining);
            client.println(acCooldownRemaining);
          }

          break;
        }
        // Solenoid lock
        else if (request.substring(0, 9) == "XDOORLOCK") {
          String pin = request.substring(10);
          if (pin == doorLockPin) {
            if (LOCK_STATUS == LOW) {
              digitalWrite(SOLENOID_RELAY, LOW);
              LOCK_STATUS = HIGH;
              lastLocked = millis();
              Serial.println("unlocked");
              client.println(LOCK_STATUS);
            }

          }
          else {
            Serial.println("Wrong PIN");
            client.println("Wrong");
          }

          break;
        }

        // Dimming
        else if (request.substring(0, 4) == "XDIM") {
          //request.replace("\r\n","");
          if (request.substring(4).length() > 0) {
            int value = request.substring(4).toInt();
            Serial.println(value);
            lampBrightness = value;
            analogWrite(BULB_PIN, value);
            client.println("Success");
            break;

            //Serial.println(request);

          }
          else {
            break;
          }
        }

        //Timed
        else if (request.substring(0, 6) == "XTIMED") {
          if (request.substring(7).length() > 0) {
            unsigned int mins = request.substring(7).toInt();
            if (TIMED_STATUS == LOW) {
              digitalWrite(TIMED_RELAY, LOW);
              TIMED_STATUS = HIGH;
            }
            timedOutletStart = millis();
            timedOutletDuration = mins * 60UL * 1000UL;
            Serial.println(mins);
            Serial.println("timed set");
            Serial.println(timedOutletDuration);
          }
          else {
            Serial.println("test");
            if (TIMED_STATUS == LOW) {
              digitalWrite(TIMED_RELAY, LOW);
              TIMED_STATUS = HIGH;
            }
            else {
              timedOutletStart = 0;
              timedOutletDuration = 0;
              digitalWrite(TIMED_RELAY, HIGH);
              TIMED_STATUS = LOW;
            }
          }

          Serial.println(TIMED_STATUS);
          client.println(TIMED_STATUS);
          break;
        }
      }
      else if (request.endsWith("\r\n\r\n")) {
        Serial.println("Sending response");

        // send a standard http response header
        // use \r\n instead of many println statements to speedup data send
        client.print(
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "Connection: close\r\n"  // the connection will be closed after completion of the response
          "Refresh: 20\r\n"        // refresh the page automatically every 20 sec
          "\r\n");
        client.print("<!DOCTYPE HTML>\r\n");
        client.print("<html>\r\n");
        client.print("<h1>Hello World!</h1>\r\n");
        client.print("Requests received: ");
        client.print(++reqCount);
        client.print("<br>\r\n");
        client.print("Analog input A0: ");
        client.print(analogRead(0));
        client.print("<br>\r\n");
        client.print("</html>\r\n");
        break;

      }

    }
    // give the web browser time to receive the data and close the connection
    delay(10);
    client.stop();
    Serial.println("Client disconnected");



  }
}

void checkDoorLockTimer(unsigned long currentMillis) {
  if (lastLocked) {
    if (currentMillis - lastLocked >= lockOpenDuration) {
      digitalWrite(SOLENOID_RELAY, HIGH);
      LOCK_STATUS = LOW;
      lastLocked = 0;
      Serial.println("locked");
    }
  }
}

void checkTimedOutletTimer(unsigned long currentMillis) {
  if (timedOutletStart) {
    if (currentMillis - timedOutletStart >= timedOutletDuration) {
      digitalWrite(TIMED_RELAY, HIGH);
      TIMED_STATUS = LOW;
      timedOutletStart = 0;
      timedOutletDuration = 0;
      Serial.println("timed expired");
      Serial.println(TIMED_STATUS);
    }
  }
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in the browser
  Serial.println();
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  Serial.println();
}

//boolean arrayIncludeElement(String array[], String element) {
//  for (int i = 0; i < authCount; i++) {
//    if (array[i] == element) {
//      return true;
//    }
//  }
//  return false;
//}
unsigned char hexCharToHex(unsigned char hex) {
  unsigned char val;
  if (hex >= '0' && hex <= '9') {
    val = hex - '0';
  }
  else if (hex >= 'a' && hex <= 'f') {
    val = hex - 'a' + 10;
  }

  return val;

}
unsigned char hexToHexChar(unsigned char hexChar) {
  unsigned char val;
  if (hexChar >= 0 && hexChar <= 9) {
    val = hexChar + '0';
  }
  else if (hexChar >= 0xA && hexChar <= 0xF) {
    val = hexChar + 'a' - 10;
  }

  return val;

}

void hexStringToHex(unsigned char* hexString, unsigned char* mac) {

  for (int i = 0; i < 6; i++) {
    mac[i] = hexCharToHex(hexString[i * 2]);
    mac[i] <<= 4;
    mac[i] += hexCharToHex(hexString[i * 2 + 1]);

  }


}
void hexToHexString(unsigned char* mac, unsigned char* hexString) {
  for (int i = 0; i < 6; i++) {
    hexString[i * 2] = hexToHexChar(mac[i] >> 4);

    hexString[i * 2 + 1] = hexToHexChar(mac[i] & 0x0F);

  }
}

