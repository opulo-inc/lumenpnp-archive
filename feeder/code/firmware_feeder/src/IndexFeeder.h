#ifndef _INDEX_FEEDER_H
#define _INDEX_FEEDER_H

#include "Feeder.h"
#include <functional>
#include <Arduino.h>
#include <HardwareSerial.h>


class IndexFeeder : public Feeder {

    public:
        IndexFeeder(uint8_t opto_signal_pin, uint8_t film_tension_pin, uint8_t drive1_pin, uint8_t drive2_pin, uint8_t peel1_pin, uint8_t peel2_pin, uint8_t led1_pin);
        bool init() override;
        Feeder::FeedResult feedDistance(uint8_t tenths_mm, bool forward, HardwareSerial *ser) override;
        
    private:
        uint8_t _opto_signal_pin;
        uint8_t _film_tension_pin;

        uint8_t _drive1_pin;
        uint8_t _drive2_pin;

        uint8_t _peel1_pin;
        uint8_t _peel2_pin;

        uint8_t _led1_pin;

        bool moveForward(HardwareSerial *ser);
        bool moveBackward(HardwareSerial *ser);
        void stop();
        bool moveInternal(HardwareSerial *ser, int threshold, bool comparison, uint32_t timeout, uint8_t drive_pin, uint32_t drive_level, uint32_t drive_millis, uint32_t pause_millis);
        bool tension(uint32_t timeout);

};

#endif //_INDEX_FEEDER_H