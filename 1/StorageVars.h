#include "Arduino.h"
#include "WString.h"
class StorageVars {
public:
  float Speed;
  float Course;
  float Altitude;
  uint32_t HDOP;
  double Lat;
  double Long;
  int RPM;
  float Force;
  float Temp;
  float Sats;
  uint32_t LongDate;
  uint32_t LongTime;

  String GPS_Year;
  String GPS_Month;
  String GPS_Day;

  String GPS_Hour;
  String GPS_Minute;
  String GPS_Second;

public:
  String Speed_string() {
    return String(Speed);
  }

  String RPM_string() {
    return String(RPM);
  }

  String Force_string() {
    return String(Force);
  }

  String Temp_string() {
    return String(Temp);
  }

  String Date_string() {
    return String(GPS_Day) + ":" + String(GPS_Month) + ":" + String(GPS_Year);
  }

  String Time_string_HM() {
    // Convert UTC time to your local timezone (for example, UTC+5)
    int localHour = (GPS_Hour.toInt() + 5) % 24;  // Replace 5 with your UTC offset
    if (localHour < 0) localHour += 24;           // Handle negative values
    String amPm = (localHour < 12) ? "AM" : "PM";
    if (localHour > 12) {
      localHour -= 12;
    } else if (localHour == 0) {
      localHour = 12;
    }

    return String(localHour) + ":" + String(GPS_Minute);
  }

  String Time_string_HMS(int TZ) {
    // Convert UTC time to your local timezone (for example, UTC+5)
    int localHour = (GPS_Hour.toInt() + TZ) % 24;  // Replace 5 with your UTC offset
    if (localHour < 0) localHour += 24;            // Handle negative values
    String amPm = (localHour < 12) ? "AM" : "PM";
    if (localHour > 12) {
      localHour -= 12;
    } else if (localHour == 0) {
      localHour = 12;
    }

    return String(localHour) + ":" + String(GPS_Minute) + ":" + String(GPS_Second);
  }

  String Time_string_HMS_edited(int TZ) {
    // Convert UTC time to your local timezone (for example, UTC+5)
    int localHour = (GPS_Hour.toInt() + TZ) % 24;  // Replace 5 with your UTC offset
    if (localHour < 0) localHour += 24;            // Handle negative values
    String amPm = (localHour < 12) ? "AM" : "PM";

    if (localHour > 12) {
      localHour -= 12;
    } else if (localHour == 0) {
      localHour = 12;
    }

    // Convert hour, minute, and second to strings and add leading zeros if necessary
    String hourStr = (localHour < 10) ? "0" + String(localHour) : String(localHour);
    String minuteStr = (GPS_Minute.toInt() < 10) ? "0" + String(GPS_Minute) : String(GPS_Minute);
    String secondStr = (GPS_Second.toInt() < 10) ? "0" + String(GPS_Second) : String(GPS_Second);

    return hourStr + ":" + minuteStr + ":" + secondStr;
  }


  String DateTimeString() {
    return Date_string() + "-" + Time_string_HMS(13);
  }

  /*
  String LogFileHeader() {
    return "DateTime,Lat,Long,Altitude,Course,Speed,Sats,HDOP,RPM,Force,Temp";
  }
  */

  String LogFileString() {
    return DateTimeString() + ',' + String(Lat, 8) + ',' + String(Long, 8) + ',' + Altitude + ',' + Course + ',' + Speed + ',' + Sats + ',' + HDOP + ',' + RPM + ',' + Force + ',' + Temp;
  }



private:
};