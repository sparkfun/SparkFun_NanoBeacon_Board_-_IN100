#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>

int scanTime = 1;
BLEScan *pBLEScan;

// This class is used for the onResult() callback function
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  // This function is called each time a new BLE device is found during a scan
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    // Some BLE devices don't have names, so we'll ignore the ones that don't
    if (advertisedDevice.haveName())
    {
      // Check if the name of this device matches our beacon
      const char* deviceName = advertisedDevice.getName().c_str();
      if(strncmp(deviceName, "BMA400_Data", strlen(deviceName)) == 0)
      {
        // This is our beacon! Print out the received signal strength
        Serial.printf("RSSI: %i\n", advertisedDevice.getRSSI());

        // Copy the manufacturer data into a buffer
        uint8_t manufacturerData[32];
        std::string manufacturerDataString = advertisedDevice.getManufacturerData();
        manufacturerDataString.copy((char*) manufacturerData, manufacturerDataString.length(), 0);

        // The first 2 bytes are the manufaturer ID, print those out
        uint16_t manufacturerID = (manufacturerData[0] << 8 ) | manufacturerData[1];
        Serial.printf("Manufacturer ID: %04X\n", manufacturerID);

        // The next byte is the battery voltage, units are 31.25mV/LSB
        uint16_t voltageRaw = manufacturerData[2];
        float voltage = voltageRaw * 0.03125;
        Serial.printf("Voltage: %.2f\n", voltage);
        
        // Last 6 bytes are the raw acceleration data for the x/y/z axes, as specified in the BMA400 datasheet
        int16_t xRaw = ((uint16_t) manufacturerData[4] << 8) | manufacturerData[3];
        int16_t yRaw = ((uint16_t) manufacturerData[6] << 8) | manufacturerData[5];
        int16_t zRaw = ((uint16_t) manufacturerData[8] << 8) | manufacturerData[7];

        // The raw values are 12-bit signed values, so we need to make sure negative values are correctly signed
        if(xRaw > 2047) xRaw -= 4096;
        if(yRaw > 2047) yRaw -= 4096;
        if(zRaw > 2047) zRaw -= 4096;

        // Convert the raw values to acceleration in g's
        // BMA400 defaults to +/- 4g over the whole 12-bit range
        float xAcc = xRaw * 4.0 / pow(2, 11);
        float yAcc = yRaw * 4.0 / pow(2, 11);
        float zAcc = zRaw * 4.0 / pow(2, 11);

        // Now print out the x/y/z acceleration values
        Serial.printf("X: %.2f Y: %.2f Z: %.2f\n\n", xAcc, yAcc, zAcc);

        // Now that we've found our beacon and logged everything, we can stop the scan
        pBLEScan->stop();
      }
    }
  }
};

void setup()
{
  // Set up serial
  Serial.begin(115200);
  Serial.println("BLE scan sketch begin!");

  // Create new BLE scan object
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();

  // Set callback for when new devices are found in the scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

  // Enable active scanning. It uses more power, but is faster
  pBLEScan->setActiveScan(true);

  // Set the scan interval and window in milliseconds
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop()
{
  // Start new scan. This blocks until finished
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);

  // Scan finished, delete results to release memory
  pBLEScan->clearResults();
}