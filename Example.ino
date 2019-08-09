/*
 * This file is modified by SurferTim's FTP code (http://playground.arduino.cc/Code/FTP)
 * and fryguy128's code (https://github.com/fryguy128/esp8266FTP/blob/master/FTPESP8266.ino)
 * raw FTP commands. A list of such commands can be found at: http://www.nsftools.com/tips/RawFTP.htm
 */
 
#include <ESP8266WiFi.h>
#include <FS.h>

//Wifi parameter
const char* ssid = "ssid-name";
const char* pwd = "password";

//FTP parameter
const char* host = "ftp-name";
const char* userName = "username";
const char* password = "password";

//File Operation
char directory[32] = "/destination-directory";
char fileName[13] = "filename";
char text2upload[255] ="Hello world. This is ESP8266 message"

//Key variable
char outBuf[128];
char outCount;

WiFiClient client;
WiFiClient dclient;

void setup() {
  Serial.begin(115200);
  
  //Wifi Setup
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //SPIFF setup
  SPIFFS.begin();
}

void loop() {
  File f = SPIFFS.open(fileName, "a");
  if (!f) {
    Serial.println("file open failed");
  }
  f.println(text2upload);
  f.close();

//Download data from FTP Server
  if ( ReadFromFTP() )
    Serial.println(F("FTP OK"));
  else
    Serial.println(F("FTP FAIL"));

//Upload data to FTP Server
  if ( WriteToFTP() )
    Serial.println(F("FTP OK"));
  else
    Serial.println(F("FTP FAIL"));

  delay(10000);
}

//Upload Function Call
byte WriteToFTP() {
  File fh = SPIFFS.open(fileName, "r");
  if (!fh) {
    Serial.println("file open failed");
  }
  if (client.connect(host, 21)) {
    Serial.println(F("Command connected"));
  }
  else {
    fh.close();
    Serial.println(F("Command connection failed"));
    return 0;
  }

  if (!eRcv()) return 0;

  client.print("USER ");
  client.println(userName);

  if (!eRcv()) return 0;

  client.print("PASS ");
  client.println(password);

  if (!eRcv()) return 0;

  client.println("SYST");

  if (!eRcv()) return 0;

  client.println("Type I");

  if (!eRcv()) return 0;

  client.println("PASV");

  if (!eRcv()) return 0;

  char *tStr = strtok(outBuf, "(,");
  int array_pasv[6];
  for ( int i = 0; i < 6; i++) {
    tStr = strtok(NULL, "(,");
    array_pasv[i] = atoi(tStr);
    if (tStr == NULL) {
      Serial.println(F("Bad PASV Answer"));
    }
  }

  unsigned int hiPort, loPort;
  hiPort = array_pasv[4] << 8;
  loPort = array_pasv[5] & 255;
  Serial.print(F("Data port: "));
  hiPort = hiPort | loPort;
  Serial.println(hiPort);
  if (dclient.connect(host, hiPort)) {
    Serial.println("Data connected");
  }
  else {
    Serial.println("Data connection failed");
    client.stop();
    fh.close();
  }

  //Change Default FTP Directory
  client.print("CWD ");
  client.println(directory);

  //Start upload file
  client.print("STOR ");
  client.println(fileName);
  if (!eRcv()) {
    dclient.stop();
    return 0;
  }
  Serial.println(F("Writing"));

  byte clientBuf[64];
  int clientCount = 0;

  while (fh.available()) {
    clientBuf[clientCount] = fh.read();
    clientCount++;

    if (clientCount > 63) {
      dclient.write((const uint8_t *)clientBuf, 64);
      clientCount = 0;
    }
  }
  if (clientCount > 0) dclient.write((const uint8_t *)clientBuf, clientCount);

  dclient.stop();
  Serial.println(F("Data disconnected"));
  client.println();
  if (!eRcv()) return 0;

  client.println("QUIT");

  if (!eRcv()) return 0;

  client.stop();
  Serial.println(F("Command disconnected"));

  fh.close();
  Serial.println(F("File closed"));
  return 1;
}

//Download Function Call
byte ReadFromFTP() {
  if (client.connect(host, 21)) {
    Serial.println(F("Command connected"));
  }
  else {
    Serial.println(F("Command connection failed"));
    return 0;
  }

  if (!eRcv()) return 0;

  client.print("USER ");
  client.println(userName);

  if (!eRcv()) return 0;

  client.print("PASS ");
  client.println(password);

  if (!eRcv()) return 0;

  client.println("SYST");

  if (!eRcv()) return 0;

  client.println("Type I");

  if (!eRcv()) return 0;

  client.println("PASV");

  if (!eRcv()) return 0;

  char *tStr = strtok(outBuf, "(,");
  int array_pasv[6];
  for ( int i = 0; i < 6; i++) {
    tStr = strtok(NULL, "(,");
    array_pasv[i] = atoi(tStr);
    if (tStr == NULL) {
      Serial.println(F("Bad PASV Answer"));
    }
  }

  unsigned int hiPort, loPort;
  hiPort = array_pasv[4] << 8;
  loPort = array_pasv[5] & 255;
  Serial.print(F("Data port: "));
  hiPort = hiPort | loPort;
  Serial.println(hiPort);
  if (dclient.connect(host, hiPort)) {
    Serial.println("Data connected");
  }
  else {
    Serial.println("Data connection failed");
    client.stop();
  }
  
  //Change Default FTP Directory
  client.print("CWD ");
  client.println(directory);
  
  //Start Download file
  client.print("RETR ");
  client.println(fileName);

  if (!eRcv()) {
    dclient.stop();
    return 0;
  }
  Serial.println(F("Reading"));

  while (dclient.connected()) {
    while (dclient.available()) {
      char c = dclient.read();
      Serial.write(c);
    }
  }

  dclient.stop();
  Serial.println();
  Serial.println(F("Data disconnected"));
  client.println();
  if (!eRcv()) return 0;

  client.println("QUIT");

  if (!eRcv()) return 0;

  client.stop();
  Serial.println(F("Command disconnected"));
  return 1;
}

byte eRcv() {
  byte respCode;
  byte thisByte;

  while (!client.available()) delay(1);

  respCode = client.peek();

  outCount = 0;

  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);

    if (outCount < 127) {
      outBuf[outCount] = thisByte;
      outCount++;
      outBuf[outCount] = 0;
    }
  }

  if (respCode >= '4') {
    efail();
    return 0;
  }

  return 1;
}

void efail() {
  byte thisByte = 0;

  client.println(F("QUIT"));

  while (!client.available()) delay(1);

  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }

  client.stop();
  Serial.println(F("Command disconnected"));
}
