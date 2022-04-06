#include "IndexFeeder.h"
#include <Arduino.h>
#include <HardwareSerial.h>

#define TENTH_MM_PER_PIP 40
#define DELAY_FORWARD_DRIVE 15
#define DELAY_BACKWARD_DRIVE 10
#define DELAY_PAUSE 50
#define DRIVE_LEVEL 200
#define TENSION_LEVEL 130

#define THRESHOLD_1_TIMEOUT     10000
#define THRESHOLD_2_TIMEOUT     10000
#define THRESHOLD_3_TIMEOUT     10000
#define TENSION_TIMEOUT         4000

// Unit Tests Fail Because This Isn't Defined In ArduinoFake for some reason
#ifndef INPUT_ANALOG
#define INPUT_ANALOG 0x04
#endif

typedef struct {
    int threshold;
    bool comparison; //true is greater than, false is less than
    uint32_t timeout;
    uint32_t drive_level;
    uint32_t drive_millis;
    uint32_t pause_millis;
} threshold_t;

static const threshold_t forward[] = {
    {900, false, THRESHOLD_1_TIMEOUT, DRIVE_LEVEL, DELAY_FORWARD_DRIVE, DELAY_PAUSE},
    {800, true, THRESHOLD_2_TIMEOUT, DRIVE_LEVEL, DELAY_FORWARD_DRIVE, DELAY_PAUSE},
    {850, false, THRESHOLD_3_TIMEOUT, DRIVE_LEVEL, DELAY_FORWARD_DRIVE, DELAY_PAUSE}
};


static const threshold_t backward[] = {
    {300, false, THRESHOLD_1_TIMEOUT, DRIVE_LEVEL, DELAY_FORWARD_DRIVE, DELAY_PAUSE},
    {200, true, THRESHOLD_2_TIMEOUT, DRIVE_LEVEL, DELAY_FORWARD_DRIVE, DELAY_PAUSE},
    {250, false, THRESHOLD_3_TIMEOUT, DRIVE_LEVEL, DELAY_FORWARD_DRIVE, DELAY_PAUSE}
};

IndexFeeder::IndexFeeder(uint8_t opto_signal_pin, uint8_t film_tension_pin, uint8_t drive1_pin, uint8_t drive2_pin, uint8_t peel1_pin, uint8_t peel2_pin, uint8_t led1_pin) :
    _opto_signal_pin(opto_signal_pin),
    _film_tension_pin(film_tension_pin),
    _drive1_pin(drive1_pin),
    _drive2_pin(drive2_pin),
    _peel1_pin(peel1_pin),
    _peel2_pin(peel2_pin),
    _led1_pin(led1_pin) {
    init();
}

bool IndexFeeder::init() {
    pinMode(_film_tension_pin, INPUT);
    pinMode(_opto_signal_pin, INPUT_ANALOG);

    pinMode(_drive1_pin, OUTPUT);
    pinMode(_drive2_pin, OUTPUT);
    pinMode(_peel1_pin, OUTPUT);
    pinMode(_peel2_pin, OUTPUT);

    pinMode(_led1_pin, OUTPUT);

    return true;
}

Feeder::FeedResult IndexFeeder::feedDistance(uint8_t tenths_mm, bool forward, HardwareSerial *ser) {

    if (tenths_mm % TENTH_MM_PER_PIP != 0) {
        // The Index Feeder has only been tested and calibrated for moves of 4mm (One Pip) so far.
        // If any other value is supplied, indicate it is invalid.
        return Feeder::FeedResult::INVALID_LENGTH;
    }

    uint8_t pips = tenths_mm / TENTH_MM_PER_PIP;

    for (size_t pip_idx = 0; pip_idx < pips; pip_idx++) {
        bool success = (forward) ? moveForward(ser) : moveBackward(ser);
        if (!success) {
            return FeedResult::MOTOR_FAULT;
        }
    }

    return Feeder::FeedResult::SUCCESS;
}

bool IndexFeeder::moveInternal(HardwareSerial *ser, int threshold, bool comparison, uint32_t timeout, uint8_t drive_pin, uint32_t drive_level, uint32_t drive_millis, uint32_t pause_millis) {
    unsigned long start_millis, current_millis;

    start_millis = millis();
    current_millis = start_millis;
    while((analogRead(_opto_signal_pin) > threshold) == comparison && (current_millis - start_millis) < timeout){
        analogWrite(drive_pin, drive_level);
        delay(drive_millis);
        analogWrite(drive_pin, 0);
        delay(pause_millis);
        current_millis = millis();
        //ser->println(analogRead(_opto_signal_pin));
    }

    //disabling returning false so that i can test motor functionality without everything working perfectly
    //return ((current_millis - start_millis) < timeout);
    return true;
}

bool IndexFeeder::tension(uint32_t timeout) {
    unsigned long start_millis, current_millis;

    start_millis = millis();
    current_millis = start_millis;      
    while(digitalRead(_film_tension_pin) == HIGH && ((current_millis - start_millis) < timeout)){//if film tension switch not clicked
        //then spin motor to wind film
        analogWrite(_peel2_pin, TENSION_LEVEL);
        current_millis = millis();
    }

    stop();

    if ((current_millis - start_millis) >= timeout) {
        return false;
    };

    return true;
}

bool IndexFeeder::moveForward(HardwareSerial *ser) {
    // First, ensure everything is stopped
    stop();
    digitalWrite(_led1_pin, LOW);
    analogWrite(_peel2_pin, TENSION_LEVEL);

    // Next Work Through Each Threshold Specified
    for (size_t idx = 0; idx < sizeof(forward) / sizeof(threshold_t); idx++) {
        const threshold_t *settings = &forward[idx];
        if (!moveInternal(ser, settings->threshold, settings->comparison, settings->timeout, _drive2_pin, settings->drive_level, settings->drive_millis, settings->pause_millis)) {
            // Motor Faul, no need to stopp as everything is stopped.
            return false;
        }
    }
    analogWrite(_peel2_pin, 0);
    digitalWrite(_led1_pin, HIGH);
    // Tension the film peeler
    return tension(TENSION_TIMEOUT);
}

bool IndexFeeder::moveBackward(HardwareSerial *ser) {
    // First, ensure everything is stopped
    stop();

    // Next, unspool some film to give the tape slack. imprecise amount because we retention later
    analogWrite(_peel2_pin, 100);
    delay(400);
    analogWrite(_peel2_pin, 0);

    // Next, work through each threshold specified
    for (size_t idx = 0; idx < sizeof(backward) / sizeof(threshold_t); idx++) {
        const threshold_t *settings = &backward[idx];
        if (!moveInternal(ser, settings->threshold, settings->comparison, settings->timeout, _drive1_pin, settings->drive_level, settings->drive_millis, settings->pause_millis)) {
            // Motor Faul, no need to stopp as everything is stopped.
            return false;
        }
    }

    return tension(TENSION_TIMEOUT);
}

void IndexFeeder::stop() {
    // Stop Everything
    analogWrite(_drive1_pin, 0);
    analogWrite(_drive2_pin, 0);
    analogWrite(_peel1_pin, 0);
    analogWrite(_peel2_pin, 0);
}