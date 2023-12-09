/*

"Now thats a spicy pillow" - Jacob Croul

*/

// https://randomnerdtutorials.com/esp8266-nodemcu-client-server-wi-fi/#:~:text=You're%20going%20to%20set,ESP8266%20boards%20using%20Arduino%20IDE.

// ==================================== Libraries
#include <LiquidCrystal_I2C.h>  // LCD
#include <HX711_ADC.h>          // LoadCell
#include "max6675.h"            // Thermocouple
#include <TinyGPS++.h>          // GPS
#include <HardwareSerial.h>     // GPS Communication
//#include "FS.h"                 // SD File System
#include "SD.h"                 // SD Card
#include "SPI.h"                // Serial Peripheral Interface
#include "StorageVars.h"        // Variable Storage
// ==================================== END Libraries

// ==================================== Variables
// Main Editables
const String recordName = "record";   // Name on file (eg. record0, record1, record2)
const int lines_per_file = 2000;         // Default: 2000
bool DSTActive = false;               // Daylight Savings Time
double loadCalibration = 116013.70;   // LoadCell Calabration Number
const int SamplesInUse = 4;           // Refresh rate speed of loadcell (Higher number = slower, Lower number = faster) Allowed values are 1, 2, 4, 8, 16, 32, 64 or 128.

// LCD
const int LCDColumns = 20;
const int LCDRows = 4;
long lcdLastReset = 0;

// LoadCell
const int HX711_dout = 3;
const int HX711_sck = 2;
bool loadCellEnabled = false;
const long loadUpdateInterval = 500;  //how frequent do we want to refresh the Load in Milliseconds.
const long loadRetryInterval = 10000;
long loadLastUpdate = 0;
unsigned long t = 0;

const int LoadcellResetBTN = 39;

//Thermocouple
const int thermoDO = 12;
const int thermoCS = 11;
const int thermoCLK = 10;

// GPS
const int GPSrx = 5;
const int GPStx = 4;
const int GPSbaud = 9600;
long gpsLastUpdate = 0;

// SD
bool cardBad = false;
int fileNumber = 1;
int currentLineNumber = 0;

// RPM
const int RPMpin = 9;
volatile int RPMcounter = 0;
long RPMLastUpdate = 0;
const long RPMUpdateInterval = 500;

// Boat (GREEN WIRE)
const int BoatSwitch = 34;
int BoatID;

// Daylight Savings Time (DST) (BROWN WIRE)
const int DSTSwitch = 35;
const int DSTin = 13;
const int DSTout = 12;
// ==================================== END Variables

// ==================================== Objects
// LCD
LiquidCrystal_I2C lcd(0x27, LCDColumns, LCDRows);

// LoadCell
HX711_ADC loadCell(HX711_dout, HX711_sck);

// Thermocouple
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// GPS
TinyGPSPlus gps;

// SD
File dataFile;

// Storage Vars
StorageVars Storage;

// ==================================== END  Objects

// ==================================== Main Functions
void setup() {

  // === BOAT SWITCH
  pinMode(BoatSwitch, INPUT);
  if (digitalRead(BoatSwitch) == HIGH) {
    loadCalibration = 2112121;  // some random number to test it
    BoatID = 999;
  } else {
    loadCalibration = 116013.70;  // the standard
    BoatID = 1;
  }
  // ===============

  // ==== DST SWITCH
  pinMode(DSTSwitch, INPUT);
  if (digitalRead(DSTSwitch) == LOW) {
    DSTActive = true;
  } else {
    DSTActive = false;
  }
  // ===============

  Serial.begin(115200);
  Serial.println("\n \n");
  LCD_Setup();

  MultiPrint("Boat ID: ", true, false, false, 0, 0, false);
  MultiPrint(String(BoatID), true, true, false, 0, 0, false);

  MultiPrint("DST Active?: ", true, false, false, 0, 0, false);
  MultiPrint(String(DSTActive), true, true, false, 0, 0, false);

  MultiPrint("Initialising...", false, false, true, 1, 1, false);
  Serial1.begin(GPSbaud);
  delay(100);
  MultiPrint("GPS started (hopefully)", true, true, false, 0, 0, false);
  loadCellEnabled = Loadcell_Setup(loadCalibration);  // calibrationValue from Calibration.ino from HX711_ADC
  pinMode(LoadcellResetBTN, INPUT);

  attachInterrupt(9, RPMCount, RISING);  // pin 13 is the hall sensor, attach an interrupt. this is the setup for it

  if (!SD.begin(53)) {
    cardBad = true;
    MultiPrint("Card Mount Failed", true, true, false, 0, 0, false);
    MultiPrint("!card", false, false, true, 15, 2, false);
    return;
  }
  

  fileNumber = countFiles() - 2;
  MultiPrint("Latest file number: " + String(fileNumber), true, true, false, 0, 0 , false);
  if (fileNumber < 0) {
    fileNumber = 0;
    MultiPrint("FileNumber is: " + String(fileNumber), true, true, false, 0, 0 , false);
  } else {
      fileNumber = fileNumber + 2;
  }
  MultiPrint("Starting file number: " + String(fileNumber), true, true, false, 0, 0 , false);


  MultiPrint("Intialisation Complete", true, true, false, 0, 0, false);


  lcd.clear();
  LCD_Reset(true);
}

void loop() {
  if (digitalRead(LoadcellResetBTN) == HIGH) {
    Loadcell_Setup(loadCalibration);
  }

  unsigned long currentMillis = millis();  // Get the current timestamp

  if (loadCellEnabled) {
    Loadcell_Loop();
    MultiPrint("\n", true, true, false, 0, 0, false);
  } else if (currentMillis > loadLastUpdate + loadRetryInterval) {
    Loadcell_Setup(loadCalibration);
  }
  Thermocouple_Loop();
  GPS_Loop();
  RPM_Loop();
  delay(1000);
  if (!LCD_Reset(false)) {  //see if it's time to do a full lcd reset
    LCD_Loop();             //if not just update the data.

    if (Storage.Lat != 0) {
      MultiPrint("fix", false, false, true, 17, 1, false);
    } else {
      MultiPrint("   ", false, false, true, 17, 1, false);
    }
  }
  SD_Loop();
  IncrementFileNumber();

  
}
// ==================================== END Main Functions

// ==================================== Secondary Functions
// Setup code for LCD
void LCD_Setup() {
  lcd.begin(LCDColumns, LCDRows);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.home();
  MultiPrint("LCD Setup done", true, true, false, 0, 0, false);
}

// Setup code for Loadcell
bool Loadcell_Setup(double Calibration) {
  MultiPrint("Loadcell Init...", true, false, false, 0, 0, false);
  loadCell.begin(128);
  loadCell.setSamplesInUse(SamplesInUse);
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 2000;  // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true;                  //set this to false if you don't want tare to be performed in the next step
  loadCell.start(stabilizingtime, _tare);
  if (loadCell.getTareTimeoutFlag()) {
    MultiPrint("Failed!", true, true, false, 0, 0, false);
    return false;
  } else {
    loadCell.setCalFactor(Calibration);  // set calibration value (float)
    MultiPrint("Completed!", true, true, false, 0, 0, false);

    MultiPrint(String(loadCell.getSPS()), true, true, false, 0, 0, false);
    return true;
  }
}

// Setup code for Thermocouple
// (none)

// Setup code for GPS
// (none)

// Setup code for SD
int countFiles() {
  int fileCount = 0;
  
  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }
    fileCount++;
    entry.close();
  }
  return fileCount;
}

// Loop code for LCD
void LCD_Loop() {
  LCD_ClearValues();
  MultiPrint(Storage.RPM_string(), false, false, true, 7, 0, false);
  MultiPrint(Storage.Speed_string(), false, false, true, 7, 1, false);
  MultiPrint(Storage.Force_string(), false, false, true, 7, 2, false);


  if (Storage.Temp > 60.00) {
    String displaytemp = String((char)255) + Storage.Temp_string() + String((char)255);
    MultiPrint(displaytemp, false, false, true, 7, 3, false);

  } else {
    MultiPrint(Storage.Temp_string(), false, false, true, 7, 3, false);
  }

  if (DSTActive == true) {
    MultiPrint(Storage.Time_string_HMS_edited(DSTin), false, false, true, 12, 0, false);
  } else if (DSTActive == false) {
    MultiPrint(Storage.Time_string_HMS_edited(DSTout), false, false, true, 12, 0, false);
  }
}

void LCD_ClearValues() {
  MultiPrint("    ", false, false, true, 7, 0, false);
  MultiPrint("    ", false, false, true, 7, 1, false);
  MultiPrint("     ", false, false, true, 7, 2, false);
  MultiPrint("         ", false, false, true, 7, 3, false);

  MultiPrint("        ", false, false, true, 12, 0, false);
}

bool LCD_Reset(bool Force) {
  long currentTime = millis();
  if (currentTime > lcdLastReset + 60000 || Force) {
    lcd.clear();
    MultiPrint("RPM:", false, false, true, 0, 0, false);
    //MultiPrint("Engine:", false, false, true, 0, 0, false);
    MultiPrint("Speed:", false, false, true, 0, 1, false);
    //MultiPrint("Speed:", false, false, true, 0, 0, false);
    MultiPrint("Force:", false, false, true, 0, 2, false);
    //MultiPrint("Force:", false, false, true, 0, 0, false);
    MultiPrint("Temp:", false, false, true, 0, 3, false);
    //MultiPrint("Temp:", false, false, true, 0, 0, false);

    MultiPrint(String(BoatID), false, false, true, 17, 3, false);
    displayCardBad();

    

    lcdLastReset = currentTime;
    return true;
  }
  return false;
}

// Loop code for Loadcell
void Loadcell_Loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0;  //increase value to slow down serial print activity
  //MultiPrint("loadcell loop 1", true, true, false, 0, 0, false);

  // check for new data/start next conversion:
  if (loadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = loadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(i);
      Storage.Force = i;
      newDataReady = 0;
      t = millis();
    }
  }
}

// Loop code for Thermocouple
String Thermocouple_Loop() {
  Storage.Temp = thermocouple.readCelsius();
  return Storage.Temp_string();
}

// Loop code for GPS
void GPS_Loop() {
  while (Serial1.available() > 0) {
    if (gps.encode(Serial1.read())) {
      // Check if there is new valid data
      if (gps.time.isValid() && gps.date.isValid()) {

        Storage.Lat = gps.location.lat();
        Storage.Long = gps.location.lng();
        Storage.Altitude = gps.altitude.meters();
        Storage.LongDate = gps.date.value();
        Storage.LongTime = gps.time.value();
        Storage.Course = gps.course.deg();
        Storage.HDOP = gps.hdop.hdop();
        Storage.Sats = gps.satellites.value();
        Storage.Speed = gps.speed.kmph();

        Storage.GPS_Year = gps.date.year();
        Storage.GPS_Month = gps.date.month();
        Storage.GPS_Day = gps.date.day();

        Storage.GPS_Hour = gps.time.hour();
        Storage.GPS_Minute = gps.time.minute();
        Storage.GPS_Second = gps.time.second();
        gpsLastUpdate = millis();
      }
    }
  }
}

// Loop code for SD
void SD_Loop() {
  MultiPrint(Storage.LogFileString(), false, false, false, 0, 0, true);
}

void displayCardBad() {
  if (cardBad == true) {
    MultiPrint("!card", false, false, true, 15, 2, false);
  }
}

void RPMCount() {  // Interupt process.
  RPMcounter++;
}

void RPM_Loop() {
  long currentTime = millis();
  if (RPMLastUpdate == 0)  // If this is the first time we call this we want to just get the time since we don't actually know how long the fist cycle has taken.@
  {
    RPMLastUpdate = currentTime;
    return;
  }
  if (currentTime > RPMLastUpdate + RPMUpdateInterval) {
    Storage.RPM = (60000 / (currentTime - RPMLastUpdate)) * RPMcounter;
    Storage.RPM = Storage.RPM / 2;
    RPMLastUpdate = currentTime;
    RPMcounter = 0;
  }
}

// Used for reporting to different places
void MultiPrint(String message, bool SerialPrint, bool SerialEndLine, bool LCDPrint, int LCDC, int LCDR, bool SDPrint) {
  if (SerialPrint) {
    if (SerialEndLine) {
      Serial.println(message);
    } else {
      Serial.print(message);
    }
  }

  if (LCDPrint) {
    lcd.setCursor(LCDC, LCDR);
    lcd.print(message);
  }

  if (SDPrint) {
    String Filename = "/" + recordName + String(fileNumber) + ".csv";
    bool existingFile = SD.exists(Filename);


    if (existingFile) {
      MultiPrint("existingFile: True", true, true, false, 0, 0, false);
      MultiPrint("Writing to File", true, true, false, 0, 0, false);
      dataFile = SD.open(Filename, O_APPEND);
      if (dataFile) {
        dataFile.println(message);
        dataFile.close();
        currentLineNumber++;
      }
    }

    if (!existingFile) {
      MultiPrint("existingFile: False\nCreating and writing to file", true, true, false, 0, 0, false);

      dataFile = SD.open(Filename, O_WRITE);
      if (dataFile) {
        dataFile.println("DateTime,Lat,Long,Altitude,Course,Speed,Sats,HDOP,RPM,Force,Temp,BoatID: " + String(BoatID));
        dataFile.println(message);
        dataFile.close();
        currentLineNumber++;
      }
    }
  }
}

void IncrementFileNumber() {
  if (currentLineNumber > lines_per_file - 1) {
    currentLineNumber = 0;
    fileNumber++;
  }
}
// ==================================== END Secondary Functions