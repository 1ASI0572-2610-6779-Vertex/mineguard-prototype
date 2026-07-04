#ifndef GPS_SENSOR_H
#define GPS_SENSOR_H

#include <ModestIoT.h>
#include <HardwareSerial.h>

class GPSSensor : public Sensor {
private:
    HardwareSerial gpsSerial;
    char sentenceBuffer[96];
    uint8_t sentenceIndex;
    double latitude;
    double longitude;
    int rxPin;
    int txPin;
    bool locationFix;

    bool parseGgaSentence(const char* sentence);
    double convertNmeaCoordinate(const char* rawCoordinate, const char* hemisphere) const;

protected:
    void processEvent(Event& event) override;

public:
    GPSSensor(int uartNum, int rx, int tx, EventHandler* parentHandler = nullptr);

    void scanLocation();
    double getLatitude() const;
    double getLongitude() const;
    bool hasFix() const;
};

#endif //GPS_SENSOR_H
