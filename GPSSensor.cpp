#include "GPSSensor.h"
#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

GPSSensor::GPSSensor(int uartNum, int rx, int tx, EventHandler* parentHandler)
    : Sensor(uartNum, parentHandler),
      gpsSerial(uartNum),
      sentenceIndex(0),
      latitude(0.0),
      longitude(0.0),
      rxPin(rx),
      txPin(tx),
      locationFix(false) {
    gpsSerial.begin(9600, SERIAL_8N1, rxPin, txPin);
    Serial.println("GPS initialized!");
}

void GPSSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        event.identifier = -1;
        return;
    }

    bool hadFix = locationFix;
    double previousLatitude = latitude;
    double previousLongitude = longitude;

    scanLocation();

    if (locationFix && (!hadFix || latitude != previousLatitude || longitude != previousLongitude)) {
        event = Event(DATA_READ_EVENT_IDENTIFIER, pin);
        return;
    }

    event.identifier = -1;
}

void GPSSensor::scanLocation() {
    while (gpsSerial.available()) {
        char incoming = static_cast<char>(gpsSerial.read());
        if (incoming == '\n') {
            sentenceBuffer[sentenceIndex] = '\0';
            parseGgaSentence(sentenceBuffer);
            sentenceIndex = 0;
        } else if (incoming != '\r' && sentenceIndex < sizeof(sentenceBuffer) - 1) {
            sentenceBuffer[sentenceIndex++] = incoming;
        }
    }
}

bool GPSSensor::parseGgaSentence(const char* sentence) {
    if (sentence == nullptr || strncmp(sentence, "$GPGGA,", 7) != 0) {
        return false;
    }

    char mutableSentence[96];
    strncpy(mutableSentence, sentence, sizeof(mutableSentence));
    mutableSentence[sizeof(mutableSentence) - 1] = '\0';

    char* fields[8] = { nullptr };
    uint8_t fieldCount = 0;
    char* token = strtok(mutableSentence, ",");
    while (token != nullptr && fieldCount < 8) {
        fields[fieldCount++] = token;
        token = strtok(nullptr, ",");
    }

    if (fieldCount < 7 || fields[2] == nullptr || fields[3] == nullptr ||
        fields[4] == nullptr || fields[5] == nullptr || fields[6] == nullptr) {
        return false;
    }

    int fixQuality = atoi(fields[6]);
    if (fixQuality <= 0) {
        return false;
    }

    latitude = convertNmeaCoordinate(fields[2], fields[3]);
    longitude = convertNmeaCoordinate(fields[4], fields[5]);
    locationFix = true;
    return true;
}

double GPSSensor::convertNmeaCoordinate(const char* rawCoordinate, const char* hemisphere) const {
    if (rawCoordinate == nullptr || hemisphere == nullptr) {
        return 0.0;
    }

    double coordinate = atof(rawCoordinate);
    int degrees = static_cast<int>(coordinate / 100.0);
    double minutes = coordinate - (degrees * 100.0);
    double decimalDegrees = degrees + (minutes / 60.0);

    if (hemisphere[0] == 'S' || hemisphere[0] == 'W') {
        decimalDegrees *= -1.0;
    }
    return decimalDegrees;
}

double GPSSensor::getLatitude() const {
    return latitude;
}

double GPSSensor::getLongitude() const {
    return longitude;
}

bool GPSSensor::hasFix() const {
    return locationFix;
}
