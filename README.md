# dbLoRa-X1
Arduino compatible battery operated LoRaWAN End Node using the RFM95 with the ATMega328P microcontroller

# Requirements:
1. Install the Arduino IDE
2. Install MCCI LMIC library https://github.com/mcci-catena/arduino-lmic
3. Inside the Arduino Libraries folder, Go to Arduino\libraries\MCCI_LoRaWAN_LMIC_library\project_config
4. Choose your region and chip accordingly
5. Example: For India 865 MH branch, select #define CFG_in866 1 & #define CFG_sx1276_radio 1
6. Change the OTAA Keys
7. Upload the program onto the board using a USB-UART converter

# Snapshots:
PCB Front
![PCB Front](/dbLoRa-3D-F.png)

PCB Back
![PCB Back](/dbLoRa-3D-B.png)

Final soldered product
![PCB Back](/dbLoRa-X1.JPG)
