# Summary
An application that can be used to detect when a door or window has been opened.  
Once such an event is detected, a notification is sent through WhatsApp.  
The idea of this project is to be battery powered, allowing for use with batteries.  
This is achieved using deep sleep functionality and wake up using the door sensor as an ext0 source.  

## Connectivity
**WIFI**  
The user can set the WIFI credentials and use that connection.  
The project stores the local_IP, gateway, subnet, primaryDNS, secondaryDNS once the first successful connection achieved.  
This allows the connection to happen much quicker and save battery.  
Also, there is a mechanism implemented to return to the traditional WIFI connection in case of an error.  

**Bluetooth**  
The user can change attributes of the device using Bluetooth connection.  
This is achieved by entering a "secret" menu of the device.  
To do this, the user needs to keep the button pressed and change the sensors position.  
For example, open the door while having the button pressed.  
This should also trigger the led to flash in blue color.  
The user can connect to the device using any Bluetooth terminal application.  
Example: [Serial Bluetooth Terminal](https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal&hl=en)  
Note: During this, the device is constantly enabled resulting in greater battery consumption.  

## Data storage
Data is stored in the nvs memory of the device.  
This allows the data to be kept even if the device turns of completely.  
The data can be changed through Bluetooth connection.  
The data can also be erased also by utilizing the Bluetooth menu.  

## User Interaction
The project allows the device to work fully autonomously.  
The user will interact with the device once connected using Bluetooth.  
The user can understand the process of the options he/she selects through the menu by seeing the led color.  
The led flashed at a blue color once the user enters the menu.  
For each option the user enters, the led flashes in a white color.  
In case of a success, the led flashes in green color and a message is also displayed in the Bluetooth terminal application.  
In case of an error, the led flashes in red color and a message is also displayed in the Bluetooth terminal application.  

## WhatsApp Integration
The project contains a function called doAction.  
In this project, the function sends a message to the devices declared using WhatsApp.  
With this implementation, each phone number is registered with a unique api key provided by [CallmeBot WhatsApp](https://www.callmebot.com/blog/free-api-whatsapp-messages/)  
This information needs to be set in the device for each desired mobile device.  

## Options Provided
Enter SSID &rarr; Change the WIFI SSID.  
Enter password &rarr; Change the WIFI password.  
Change device name &rarr; Change the name of the device (this appears in the WIFI connection and also in the Bluetooth connection).  
Change GMT offset &rarr; Change GMT offset used in the time calculation.  
Change Daylight offset &rarr; Change Daylight used in the time calculation.  
Add phone and key &rarr; Add a new phone-key pair.  
Remove phone and key &rarr; Remove a phone-key pair.  
Test connection &rarr; Test the WIFI connection.  
View current stats &rarr; View some stats for the device.  
Show users &rarr; Show current phone-key pairs.  
Reset &rarr; Reset all values to default.  
Exit &rarr; Exit menu.  


## Hardware used
ESP32 Devkit V1  
MC-38 Wired Door Window Sensor  
Common Cathode RGB led  
Push Button  
Dupont wires  
