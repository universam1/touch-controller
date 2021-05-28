#include <Arduino.h>
#include "LowPower.h"
#include <FadeLed.h>

const uint32_t standbyDelay = 2000;
uint32_t lastLight;
bool direction;
const uint8_t touchPin = 2;
const uint8_t carPin = 3;
const uint8_t carALight = A1;
const uint8_t carABat = A2;
const uint8_t FETPort = 5;
volatile bool _touched = false;
volatile bool _carTrigger = false;
#define OPENED 1
#define CLOSED -1
#define FADETIME 5000

FadeLed led(FETPort);

void flash()
{
    auto current = led.get();
    auto currentRaw = led.getCurrent();
    // no flash when off
    if (currentRaw == 0)
        return;
    led.begin(currentRaw < 50 ? currentRaw + 25 : currentRaw - 25);
    FadeLed::update();
    delay(10);
    led.begin(currentRaw);
    FadeLed::update();
    led.set(current);
    FadeLed::update();
    delay(50);
}

bool isTouchTriggered()
{
    static uint32_t lastTouch;

    if (millis() - lastTouch < 200)
        _touched = false;
    else if (_touched)
        lastTouch = millis();
    return _touched;
}

int8_t isCarTriggered()
{
#define ROUNDS 10
    static uint32_t lastCarTrigger;
    int8_t ret = 0;

    if (millis() - lastCarTrigger < 100)
        _carTrigger = false;
    else if (_carTrigger)
    {
        delay(50); // let cap discharge
        uint16_t res = 0;
        for (size_t i = 0; i < ROUNDS; i++)
        {
            delay(10);
            auto r = analogRead(carALight);
            Serial.println(r);
            res += r;
        }
        ret = res < 250 * ROUNDS ? OPENED : CLOSED;
        _carTrigger = false;
        lastCarTrigger = millis();
    }

    return ret;
}
void evalStandby()
{
    if (led.getCurrent() > 0)
    {
        lastLight = millis();
    }
    else if (millis() - lastLight > standbyDelay)
    {
        Serial.println("\nstdby");
        delay(50);
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
        Serial.println("woke");
        lastLight = millis();
    }
}
void ISR0()
{
    _touched = true;
}

void ISR1()
{
    _carTrigger = true;
}

void setup()
{
    pinMode(touchPin, INPUT);
    pinMode(carPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(touchPin), ISR0, RISING);
    attachInterrupt(digitalPinToInterrupt(carPin), ISR1, CHANGE);

    FadeLed::setInterval(10);
    led.setTime(FADETIME);

    Serial.begin(115200);
}

void loop()
{
    FadeLed::update();

    auto carTrigger = isCarTriggered();
    if (carTrigger == OPENED)
    {

        Serial.println("opened");
        led.on();
    }
    else if (carTrigger == CLOSED)
    {
        Serial.println("closed");
        led.off();
    }
    else if (isTouchTriggered())
    {
        flash();
        Serial.println(analogRead(carABat));
        if (!led.done())
        {
            Serial.print("s");
            led.stop();
        }
        else
        {
            direction = !direction;
            Serial.print(direction ? "u" : "d");
            led.set(direction ? 100 : 0);
        }
    }

    evalStandby();
}