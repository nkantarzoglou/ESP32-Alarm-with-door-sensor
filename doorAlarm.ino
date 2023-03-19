#include <Preferences.h>
#include <WiFi.h>
#include "BluetoothSerial.h"
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <nvs_flash.h>
#define maxUsers 10

#define DOOR_SENSOR_PIN 13
#define BUTTON_PIN 15
#define PIN_RED 23
#define PIN_GREEN 22
#define PIN_BLUE 21


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

struct user {
  char phone[20];
  char apiKey[10];
};


struct wifiData {
  char ssid[32];
  char password[63];
  int local_IP[4];
  int gateway[4];
  int subnet[4];
  int primaryDNS[4];
  int secondaryDNS[4];
};

struct info {
  int doorState;
  char deviceName[20];
  user users[maxUsers];
};

struct settings {
  long gmtOffset;
  int daylightOffset;
};

Preferences preferences;
wifiData connectionData;
info information;
settings deviceSettings;
BluetoothSerial SerialBT;

const char* ntpServer = "pool.ntp.org";
int previousEvent = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  initDataFromNVS();

  if (digitalRead(BUTTON_PIN) == 0) {
    // initialize BT when a button is pressed
    if (!SerialBT.begin(information.deviceName)) {
      ESP.restart();
    }
    ledFlash(0, 0, 255, 500, 1);
    SerialBT.register_callback(btCallback);

  } else {
    information.doorState = digitalRead(DOOR_SENSOR_PIN);
    if (wifiConnect() == 2) {
      doAction();
      sleep();
    }
  }
}
void loop() {
}
int wifiConnect() {
  if (strlen(connectionData.ssid) == 0 || strlen(connectionData.password) == 0) {
    return 0;
  }
  if (compareIpItemsWithDefault()) {
    if (normalWifiConnect()) {
      changeIpItems(WiFi.localIP(), WiFi.subnetMask(), WiFi.gatewayIP(), WiFi.dnsIP(0), WiFi.dnsIP(1));
      return 2;
    }
    return 1;
  } else {
    if (quickWifiConnect()) {
      return 2;
    } else {
      int resetArray[4] = { 0, 0, 0, 0 };
      changeIpItems(convertAddress(resetArray), convertAddress(resetArray), convertAddress(resetArray), convertAddress(resetArray), convertAddress(resetArray));
      if (normalWifiConnect()) {
        changeIpItems(WiFi.localIP(), WiFi.subnetMask(), WiFi.gatewayIP(), WiFi.dnsIP(0), WiFi.dnsIP(1));
        return 2;
      }
      return 1;
    }
  }
}
boolean quickWifiConnect() {
  int counter = 10;  //10 will be 5 seconds
  WiFi.setHostname(information.deviceName);
  WiFi.config(convertAddress(connectionData.local_IP), convertAddress(connectionData.gateway), convertAddress(connectionData.subnet), convertAddress(connectionData.primaryDNS), convertAddress(connectionData.secondaryDNS));
  WiFi.mode(WIFI_STA);
  WiFi.begin(connectionData.ssid, connectionData.password);
  while (WiFi.status() != WL_CONNECTED && counter > 0) {
    counter--;
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  return false;
}
boolean normalWifiConnect() {
  int counter = 30;  //30 will be 15 seconds
  WiFi.setHostname(information.deviceName);
  WiFi.config(convertAddress(connectionData.local_IP), convertAddress(connectionData.gateway), convertAddress(connectionData.subnet), convertAddress(connectionData.primaryDNS), convertAddress(connectionData.secondaryDNS));
  WiFi.mode(WIFI_STA);
  WiFi.begin(connectionData.ssid, connectionData.password);
  while (WiFi.status() != WL_CONNECTED && counter > 0) {
    counter--;
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  return false;
}
void changeIpItems(IPAddress lip, IPAddress sm, IPAddress gip, IPAddress dsnp, IPAddress dnss) {
  for (int i = 0; i < 4; i++) {
    connectionData.local_IP[i] = lip[i];
    connectionData.gateway[i] = gip[i];
    connectionData.subnet[i] = sm[i];
    connectionData.primaryDNS[i] = dsnp[i];
    connectionData.secondaryDNS[i] = dnss[i];
  }
  preferences.begin("wifiData", false);
  preferences.putBytes("local_IP", connectionData.local_IP, sizeof(int[4]));
  preferences.putBytes("gateway", connectionData.gateway, sizeof(int[4]));
  preferences.putBytes("subnet", connectionData.subnet, sizeof(int[4]));
  preferences.putBytes("primaryDNS", connectionData.primaryDNS, sizeof(int[4]));
  preferences.putBytes("secondaryDNS", connectionData.secondaryDNS, sizeof(int[4]));
  preferences.end();
}
boolean compareIpItemsWithDefault() {
  for (int i = 0; i < 4; i++) {
    if (connectionData.local_IP[i] != 0 || connectionData.gateway[i] != 0 || connectionData.subnet[i] != 0 || connectionData.primaryDNS[i] != 0 || connectionData.secondaryDNS[i] != 0) {
      return false;
    }
    return true;
  }
}
IPAddress convertAddress(int array[]) {
  IPAddress address(array[0], array[1], array[2], array[3]);
  return address;
}
void initDataFromNVS() {
  preferences.begin("wifiData", true);
  preferences.getBytes("ssid", connectionData.ssid, sizeof(char[32]));
  preferences.getBytes("password", connectionData.password, sizeof(char[63]));
  preferences.getBytes("local_IP", connectionData.local_IP, sizeof(int[4]));
  preferences.getBytes("gateway", connectionData.gateway, sizeof(int[4]));
  preferences.getBytes("subnet", connectionData.subnet, sizeof(int[4]));
  preferences.getBytes("primaryDNS", connectionData.primaryDNS, sizeof(int[4]));
  preferences.getBytes("secondaryDNS", connectionData.secondaryDNS, sizeof(int[4]));
  preferences.end();

  preferences.begin("info", false);
  preferences.getBytes("users", information.users, sizeof(user) * maxUsers);
  information.doorState = preferences.getInt("doorState", digitalRead(DOOR_SENSOR_PIN));
  preferences.getBytes("deviceName", information.deviceName, sizeof(char[20]));
  if (strlen(information.deviceName) == 0) {
    memset(information.deviceName, 0, sizeof(information.deviceName));
    preferences.putBytes("deviceName", "Door sensor", sizeof(char[20]));
    preferences.getBytes("deviceName", information.deviceName, sizeof(char[20]));
  }
  preferences.end();

  preferences.begin("settings", true);
  deviceSettings.daylightOffset = preferences.getInt("daylightOffset", 1);
  deviceSettings.gmtOffset = preferences.getLong("gmtOffset", 2);
  preferences.end();
}
void sleep() {
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, !information.doorState);  //1 = High, 0 = Low
  delay(100);
  esp_deep_sleep_start();
}
String printLocalTime() {
  struct tm timeinfo;
  configTime(deviceSettings.gmtOffset * 3600, deviceSettings.daylightOffset * 3600, ntpServer);
  if (!getLocalTime(&timeinfo)) {
    return "ERROR";
  }

  char timeHour[3];
  char timeMinute[3];
  char timeSecond[3];

  char timeWeekDay[10];
  char timeDay[3];
  char timeMonth[10];
  char timeYear[5];

  strftime(timeHour, 3, "%H", &timeinfo);
  strftime(timeMinute, 3, "%M", &timeinfo);
  strftime(timeSecond, 3, "%S", &timeinfo);

  strftime(timeWeekDay, 10, "%A", &timeinfo);
  strftime(timeDay, 3, "%d", &timeinfo);
  strftime(timeMonth, 10, "%B", &timeinfo);
  strftime(timeYear, 5, "%Y", &timeinfo);

  return String(timeDay) + " " + String(timeMonth) + " " + String(timeYear) + " - " + String(timeHour) + ":" + String(timeMinute) + ":" + String(timeSecond);
}
void doAction() {
  String url;
  String date = printLocalTime();
  for (int i = 0; i < maxUsers; i++) {
    if (strlen(information.users[i].phone) == 0 || strlen(information.users[i].apiKey) == 0) {
      break;
    } else {
      if (information.doorState == HIGH) {
        url = "https://api.callmebot.com/whatsapp.php?phone=" + String(information.users[i].phone) + "&apikey=" + String(information.users[i].apiKey) + "&text=" + urlEncode("Device: " + String(information.deviceName) + "\nStatus: Open\nDate: " + date);
      } else {
        url = "https://api.callmebot.com/whatsapp.php?phone=" + String(information.users[i].phone) + "&apikey=" + String(information.users[i].apiKey) + "&text=" + urlEncode("Device: " + String(information.deviceName) + "\nStatus: Closed\nDate: " + date);
      }
      HTTPClient http;
      http.begin(url);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      http.POST(url);
      http.end();
    }
  }
}
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    printMainMenu();
    ledFlash(0, 255, 0, 500, 1);
  } else if (event == ESP_SPP_DATA_IND_EVT) {
    String x;
    while (SerialBT.available()) {
      char incomingChar = SerialBT.read();
      x = x + String(incomingChar);
    }
    x = x.substring(0, x.length() - 2);

    if (previousEvent == 0) {
      ledFlash(255, 255, 255, 200, 1);
      if (x.toInt() == 1) {
        SerialBT.println("Please enter the desired ssid.");
        previousEvent = 1;
      } else if (x.toInt() == 2) {
        SerialBT.println("Please enter the desired password.");
        previousEvent = 2;
      } else if (x.toInt() == 3) {
        SerialBT.println("Please enter the desired device name.");
        previousEvent = 3;
      } else if (x.toInt() == 4) {
        SerialBT.println("Please enter the desired GMT offset(-12..+14).");
        previousEvent = 4;
      } else if (x.toInt() == 5) {
        SerialBT.println("Please enter the desired daylight offset(-12..+14).");
        previousEvent = 5;
      } else if (x.toInt() == 6) {
        if (getTotalUsers() == maxUsers) {
          SerialBT.println("Cannot store more users.");
          ledFlash(255, 0, 0, 500, 2);
          previousEvent = 0;
          printMainMenu();
          return;
        }
        SerialBT.println("Please enter data in format: phone-apikey.");
        previousEvent = 6;
      } else if (x.toInt() == 7) {
        if (getTotalUsers() == 0) {
          SerialBT.println("No users stored.");
          ledFlash(255, 0, 0, 500, 2);
          previousEvent = 0;
          printMainMenu();
          return;
        }
        SerialBT.println("Please enter phone number or api key to remove.");
        previousEvent = 7;
      } else if (x.toInt() == 8) {
        if (wifiConnect() == 2) {
          SerialBT.println("Successful connection.");
          ledFlash(0, 255, 0, 500, 2);
        } else {
          SerialBT.println("Unsuccessful connection.");
          ledFlash(255, 0, 0, 500, 2);
        }
        printMainMenu();
      } else if (x.toInt() == 9) {
        SerialBT.println("Device name: " + String(information.deviceName) + "\nSSID: " + connectionData.ssid + "\nPass: " + connectionData.password + "\nGMT offset: " + deviceSettings.gmtOffset + "\nDaylight offset: " + deviceSettings.daylightOffset + "\nUsers: " + getTotalUsers());
        printMainMenu();
      } else if (x.toInt() == 10) {
        if (getTotalUsers() == 0) {
          SerialBT.println("No users to show.");
        } else {
          String x = "Phone : Key";
          for (int i = 0; i < maxUsers; i++) {
            if (strlen(information.users[i].phone) > 0 && strlen(information.users[i].apiKey) > 0) {
              x = x + "\n" + String(information.users[i].phone) + " : " + String(information.users[i].apiKey);
            }
          }
          SerialBT.println(x);
        }
        printMainMenu();
      } else if (x.toInt() == 11) {
        SerialBT.println("Performing reset and restarting.");
        nvs_flash_erase();
        nvs_flash_init();
        ESP.restart();
      } else if (x.toInt() == 12) {
        SerialBT.println("Exiting menu.");
        ESP.restart();
      } else {
        previousEvent = 0;
        SerialBT.println("Please ensure you made a valid selection.");
        printMainMenu();
      }
    } else {
      if (previousEvent == 1) {
        if (x.length() > 32) {
          SerialBT.println("SSID is too long.");
          ledFlash(255, 0, 0, 500, 2);
        } else {
          memset(connectionData.ssid, 0, sizeof(connectionData.ssid));
          for (int i = 0; i < x.length(); i++) {
            connectionData.ssid[i] = x.charAt(i);
          }
          preferences.begin("wifiData", false);
          preferences.putBytes("ssid", connectionData.ssid, sizeof(char[32]));
          preferences.end();
          SerialBT.println("SSID changed successfully.");
          ledFlash(0, 255, 0, 500, 2);
        }
      } else if (previousEvent == 2) {
        if (x.length() > 63) {
          SerialBT.println("Password is too long.");
          ledFlash(255, 0, 0, 500, 2);
        } else {
          memset(connectionData.password, 0, sizeof(connectionData.password));
          for (int i = 0; i < x.length(); i++) {
            connectionData.password[i] = x.charAt(i);
          }
          preferences.begin("wifiData", false);
          preferences.putBytes("password", connectionData.password, sizeof(char[63]));
          preferences.end();
          SerialBT.println("Password changed successfully.");
          ledFlash(0, 255, 0, 500, 2);
        }
      } else if (previousEvent == 3) {
        if (x.length() > 20) {
          SerialBT.println("Device name is too long.");
          ledFlash(255, 0, 0, 500, 2);
        } else {
          memset(information.deviceName, 0, sizeof(information.deviceName));
          for (int i = 0; i < x.length(); i++) {
            information.deviceName[i] = x.charAt(i);
          }
          preferences.begin("info", false);
          preferences.putBytes("deviceName", information.deviceName, sizeof(char[20]));
          preferences.end();
          SerialBT.println("Device name changed successfully.");
          ledFlash(0, 255, 0, 500, 2);
        }
      } else if (previousEvent == 4) {
        if (x.toInt() < -12 || x.toInt() > 14) {
          SerialBT.println("GMT offset not valid.");
          ledFlash(255, 0, 0, 500, 2);
        } else {
          deviceSettings.gmtOffset = x.toInt();
          preferences.begin("settings", false);
          preferences.putInt("gmtOffset", deviceSettings.gmtOffset);
          preferences.end();
          SerialBT.println("GMT offset changed.");
          ledFlash(0, 255, 0, 500, 2);
        }
      } else if (previousEvent == 5) {
        if (x.toInt() < -12 || x.toInt() > 14) {
          SerialBT.println("Daylight offset not valid.");
          ledFlash(255, 0, 0, 500, 2);
        } else {
          deviceSettings.daylightOffset = x.toInt();
          preferences.begin("settings", false);
          preferences.putInt("daylightOffset", deviceSettings.daylightOffset);
          preferences.end();
          SerialBT.println("Daylight offset changed.");
          ledFlash(0, 255, 0, 500, 2);
        }
      } else if (previousEvent == 6) {
        int index = x.indexOf('-');
        if (index == -1) {
          SerialBT.println("Invalid data given.");
          ledFlash(255, 0, 0, 500, 2);
          previousEvent = 0;
          printMainMenu();
          return;
        }
        if (checkUserExistence(x, true) != -400) {
          SerialBT.println("User already exists.");
          ledFlash(255, 0, 0, 500, 2);
          previousEvent = 0;
          printMainMenu();
          return;
        }
        //if i have 2 users, then they are stored at positions 0 and 1
        //so i need to store the new user at position 2
        //which is the number of users
        int positionToAdd = getTotalUsers();

        memset(information.users[positionToAdd].phone, 0, sizeof(information.users[positionToAdd].phone));
        memset(information.users[positionToAdd].apiKey, 0, sizeof(information.users[positionToAdd].apiKey));

        for (int i = 0; i < index; i++) {
          information.users[positionToAdd].phone[i] = x.charAt(i);
        }
        int j = 0;
        for (int i = (index + 1); i < x.length(); i++) {
          information.users[positionToAdd].apiKey[j] = x.charAt(i);
          j++;
        }
        preferences.begin("info", false);
        preferences.putBytes("users", information.users, sizeof(user) * maxUsers);
        preferences.end();
        SerialBT.println("Phone pair added.");
        ledFlash(0, 255, 0, 500, 2);
      } else if (previousEvent == 7) {
        int position = checkUserExistence(x, false);
        if (position != -400) {
          shiftAndUpdateRecords(position);
          SerialBT.println("Item deleted.");
          ledFlash(0, 255, 0, 500, 2);
        } else {
          SerialBT.println("No items found.");
          ledFlash(255, 0, 0, 500, 2);
        }
      }
      previousEvent = 0;
      printMainMenu();
    }
  }
}
void printMainMenu() {
  SerialBT.println("Please select a number:\n1)Enter SSID\n2)Enter password\n3)Change device name\n4)Change GMT offset\n5)Change Daylight offset\n6)Add phone and key\n7)Remove phone and key\n8)Test connection\n9)View current stats\n10)Show users\n11)Reset\n12)Exit");
}
void shiftAndUpdateRecords(int positionFound) {
  int lastValidDataPosition = positionFound;

  for (int i = positionFound; i < maxUsers; i++) {
    if (strlen(information.users[i].phone) == 0 || strlen(information.users[i].apiKey) == 0) break;
    lastValidDataPosition = i;
    if (lastValidDataPosition == maxUsers) {
      lastValidDataPosition = lastValidDataPosition - 1;
    }
  }

  memset(information.users[positionFound].phone, 0, sizeof(information.users[positionFound].phone));
  memset(information.users[positionFound].apiKey, 0, sizeof(information.users[positionFound].apiKey));

  for (int i = 0; i < strlen(information.users[lastValidDataPosition].phone); i++) {
    information.users[positionFound].phone[i] = information.users[lastValidDataPosition].phone[i];
  }

  for (int i = 0; i < strlen(information.users[lastValidDataPosition].apiKey); i++) {
    information.users[positionFound].apiKey[i] = information.users[lastValidDataPosition].apiKey[i];
  }
  memset(information.users[lastValidDataPosition].phone, 0, sizeof(information.users[lastValidDataPosition].phone));
  memset(information.users[lastValidDataPosition].apiKey, 0, sizeof(information.users[lastValidDataPosition].apiKey));

  preferences.begin("info", false);
  preferences.putBytes("users", information.users, sizeof(user) * maxUsers);
  preferences.end();
}
int getTotalUsers() {
  for (int i = 0; i < maxUsers; i++) {
    if (strlen(information.users[i].phone) == 0 || strlen(information.users[i].apiKey) == 0) {
      return (i);
    }
  }
  return maxUsers;
}
int checkUserExistence(String x, bool toAdd) {
  if (toAdd) {
    String phone, apiKey;
    int index = x.indexOf('-');
    for (int i = 0; i < index; i++) {
      phone = phone + x.charAt(i);
    }
    for (int i = (index + 1); i < x.length(); i++) {
      apiKey = apiKey + x.charAt(i);
    }
    //from add
    for (int i = 0; i < maxUsers; i++) {
      if (String(information.users[i].phone) == phone && String(information.users[i].apiKey) == apiKey) {
        return (i);
      }
    }
    return -400;
  } else {
    //from delete
    for (int i = 0; i < maxUsers; i++) {
      if (String(information.users[i].phone) == x || String(information.users[i].apiKey) == x) {
        return (i);
      }
    }
    return -400;
  }
}
void ledFlash(int R, int G, int B, int delayBetweenFlash, int times) {
  if (times == 1) {
    analogWrite(PIN_RED, R);
    analogWrite(PIN_GREEN, G);
    analogWrite(PIN_BLUE, B);
    delay(delayBetweenFlash);
    analogWrite(PIN_RED, 0);
    analogWrite(PIN_GREEN, 0);
    analogWrite(PIN_BLUE, 0);
  } else {
    analogWrite(PIN_RED, R);
    analogWrite(PIN_GREEN, G);
    analogWrite(PIN_BLUE, B);
    delay(delayBetweenFlash);
    analogWrite(PIN_RED, 0);
    analogWrite(PIN_GREEN, 0);
    analogWrite(PIN_BLUE, 0);
    delay(delayBetweenFlash);
    analogWrite(PIN_RED, R);
    analogWrite(PIN_GREEN, G);
    analogWrite(PIN_BLUE, B);
    delay(delayBetweenFlash);
    analogWrite(PIN_RED, 0);
    analogWrite(PIN_GREEN, 0);
    analogWrite(PIN_BLUE, 0);
  }
}