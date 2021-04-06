/*
    BLE_write based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleWrite.cpp
    Ported to Arduino ESP32 by Evandro Copercini

    Zapper based on https://github.com/CrashOverride85/collar
*/
#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <collar.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.print("Bluetooth Receive: "); 
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
        Serial.println();
      }
    }
};

// Set this to whichever pin has the 433MHz transmitter connected to it
const uint8_t  tx_pin = 26;

// Either set this to the id of the original transmitter,
// or make a value up and re-pair
const uint16_t transmitter_id = 0x0ED6;

CollarTx *_tx;

void setup() {

  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen( BLACK );
  M5.Lcd.setCursor(5, 5);
  M5.Lcd.setTextColor(YELLOW);  
  M5.Lcd.setTextSize(3);
  M5.Lcd.println("BLE ZAPPER");
  
  Serial.begin(115200);

  Serial.println();
  Serial.print("Using ID: ");
  Serial.println(transmitter_id, HEX);
  _tx =  new CollarTx(tx_pin, transmitter_id);
  print_help();
}

void print_help()
{
  Serial.println("\n Download and install a BLE scanner app in your phone");
  Serial.println(" Scan for BLE devices in the app");
  Serial.println(" Connect to BLE ZAPPER");
  Serial.println(" Go to CUSTOM CHARACTERISTIC in CUSTOM SERVICE and write a command");
  Serial.println("\nExpected command format:");
  Serial.println("  <channel><command><power>");
  Serial.println("where:");
  Serial.println("   channel = 1-3");
  Serial.println("   command = B, S, V, L (Beep, Shock, Vibrate, Light)");
  Serial.println("   power   = 00-99");
  Serial.println();
  Serial.println("e.g.:");
  Serial.println("  1V05 - Vibrate channel 1 at power 5");
  Serial.println("  2S10 - Shock channel 2 at power 10");
  Serial.println();  

  BLEDevice::init("BLE Zapper");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  pCharacteristic->setValue("Hello World");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop() {
  // put your main code here, to run repeatedly:
  static char serial_buffer[8];
  static uint8_t serial_pos=0;

  while (Serial.available() > 0)
  {
    char c = Serial.read();

    if ((c == '\n') || (c == '\r'))
    {
      serial_buffer[serial_pos] = '\0';

      if (serial_pos > 0)
      {
        process_message(serial_buffer);
        memset(serial_buffer, 0, sizeof(serial_buffer));
        serial_pos = 0;
      }
    }
    else
    {
      if (serial_pos < (sizeof(serial_buffer)-1))
      {
        serial_buffer[serial_pos++] = c;
      }
    }
  }
}

void process_message(const char *input_message)
{
  collar_channel channel;
  collar_mode mode;
  uint8_t power;

  if (strlen(input_message) != 4)
  {
    Serial.println("Message not expected 4 characters. Ignorning.");
    return;
  }

  // Channel
  switch (input_message[0])
  {
    case '1':
      channel = CH1;
      break;

    case '2':
      channel = CH2;
      break;

    case '3':
      channel = CH3;
      break;

    default:
      Serial.println("Unexpected channel");
      return;
  }

  // Mode
  switch (input_message[1])
  {
    case 'B':
    case 'b':
      mode = BEEP;
      break;

    case 'S':
    case 's':
      mode = SHOCK;
      break;

    case 'V':
    case 'v':
      mode = VIBE;
      break;

    case 'L':
    case 'l':
      mode = LIGHT;
      break;
      
    default:
      Serial.println("Unexpected mode");
      return;
  }

  power = atoi(&input_message[2]);

  Serial.print("433 MHz Transmit");
  
  _tx->transmit(channel, mode, power);
}
