/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/
//***SENSOR***
#include <Wire.h> //BH1750 IIC Mode
#include <math.h> 
int BH1750address = 0x23; //setting i2c address
byte buff[2];
//************

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// This EUI must be in little-endian form.
static const u1_t PROGMEM APPEUI[8]={ 0xF7, 0x54, 0x01, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 }; //CAN BE ANYTHING FOR Loraserver.io
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={0xf1, 0x55, 0x50, 0x8e, 0x6b, 0x06, 0xe5, 0xec }; //Should be same in Loraserver.io// Write LSB first from loraserver.io
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format 
static const u1_t PROGMEM APPKEY[16] = { 0x98, 0xf2, 0x30, 0x70, 0x4e, 0x26, 0xd2, 0x10, 0x61, 0x09, 0xb1, 0x24, 0x81, 0x83, 0xda, 0x45 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds
const unsigned TX_INTERVAL = 60;

// Pin mapping
const lmic_pinmap lmic_pins = 
{
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

void onEvent (ev_t ev) 
{
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) 
    {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("artKey: ");
              for (int i=0; i<sizeof(artKey); ++i) {
                Serial.print(artKey[i], HEX);
              }
              Serial.println("");
              Serial.print("nwkKey: ");
              for (int i=0; i<sizeof(nwkKey); ++i) {
                Serial.print(nwkKey[i], HEX);
              }
              Serial.println("");
            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
            // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;

        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) 
            {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));

//**************DEVICE CONTROL CODE LINES******************

              if (LMIC.dataLen == 1) 
              {
                  char result = LMIC.frame[LMIC.dataBeg + 0];
                  if (result == '0')  
                  {
                      Serial.println("Downlink Payload Data: 0");
                      digitalWrite(A1,LOW);
                      Serial.println("LED Turned Off!");
                  }              
                  if (result == '1')  
                  {
                      Serial.println("Downlink Payload Data: 1");
                      digitalWrite(A1,HIGH);                
                      Serial.println("LED Turned On!");
                  } 
              }             
//***********************************************************
            }
            Serial.println();
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;

        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j)
{
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) 
    {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } 
    else 
    {      
//************SENSOR*******************
         int i;
         uint16_t val=0;
         byte payload[2];
         BH1750_Init(BH1750address);
         delay(200);
       
         if(2==BH1750_Read(BH1750address))
         {
           val=((buff[0]<<8)|buff[1])/1.2;
           Serial.print("Light Intensity: ");
           Serial.print(val,DEC);     
           Serial.println(" lx (lux)"); 
         }
         delay(150);
         
         val=val*100;
         
         payload[0]=highByte(val);
         payload[1]=highByte(val);           
//***************************************

        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, (uint8_t*)payload, sizeof(payload), 0);
        
        Serial.print("Channel Frequency: ");
        Serial.print(LMIC.freq);
        Serial.println(" MHz");

        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}


void setup() 
{
    Serial.begin(9600);
    Serial.println("LoRaWAN: End node sending data to Network Server via Gateway.");
    Serial.println("Starting Transmission by OTAA Mode.");
    Serial.println("Device EUI, Application EUI & Application Key obtained from Server side (Own/Tata/Senraco).");
    Serial.println("");


//***SENSOR***
    Wire.begin();
//************

//***DEVICE CONTROL***
    pinMode(A1,OUTPUT); //FOR dbLoRa-X1
//********************

   
    // LMIC init
    os_init();
    
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Let LMIC compensate for +/- 1% clock error
      LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); //1% can be made 2% also
//
//    #if defined(CFG_in866)
//    LMIC_setupChannel(0, 865062500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(1, 865402500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(2, 865985000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(3, 865232500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(4, 865572500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(5, 865815000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(6, 866155000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band
//    LMIC_setupChannel(7, 866325000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // g-band    
//    #endif


    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void loop() 
{
    os_runloop_once();
}


//************SENSOR********************
int BH1750_Read(int address)
{
     int i=0;
     Wire.beginTransmission(address);
     Wire.requestFrom(address, 2);
     while(Wire.available()) 
     {
        buff[i] = Wire.read();  // receive one byte
        i++;
     }
     Wire.endTransmission();  
     return i;
}
 
void BH1750_Init(int address) 
{
     Wire.beginTransmission(address);
     Wire.write(0x10); //1lx reolution 120ms
     Wire.endTransmission();
}
//**************************************
