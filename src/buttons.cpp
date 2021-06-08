#include <Arduino.h>
#include "LowPower.h"
#include <FadeLed.h>

const uint32_t standbyDelay = 20U * 1000U;
uint32_t lastLight;
const uint32_t shutdownDelay = 60U * 60U * 1000U;
uint32_t lightOnSince;

bool directionUp;
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

#define UCARCONV 13.3f / 608.0f / ROUNDS
#define UCARMAX 12.5f

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
        delay(300); // let cap discharge
        int res = 0;
        auto prev = analogRead(carALight);
        for (size_t i = 0; i < ROUNDS; i++)
        {
            delay(20);
            auto r = analogRead(carALight);
            res += r - prev;
            Serial.println(res);
        }
        ret = res < 0 ? OPENED : CLOSED;
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
void evalShutdown()
{
    if (led.get() == 0)
    {
        return;
    }
    if (millis() - lightOnSince > shutdownDelay)
    {
        Serial.println("\nshutdown");
        led.off();
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

void scaleToVSup()
{
    float val;
    for (size_t i = 0; i < ROUNDS; i++)
    {
        val += analogRead(carABat);
    }
    float volt = val * UCARCONV;
    Serial.print("Ucar: ");
    Serial.print(volt);
    Serial.print("   raw: ");
    Serial.print(val / ROUNDS);
    float factor = UCARMAX / volt;
    // Serial.print("    factor: ");
    // Serial.print(factor);
    if (factor > 1.0)
        factor = 1.0;

    auto current = led.get();
    uint8_t limit = factor * 255.0f;
    Serial.print("    limit: ");
    Serial.print(limit);
    Serial.print("    gamma: ");
    Serial.print(led.getGammaValue(current));

    Serial.print("    car: ");
    Serial.println(analogRead(carALight));
    // while (led.getGammaValue(current) > limit)
    // {
    //     current--;
    //     Serial.print("r");
    // }
    // if (led.get() != current)
    // {
    //     Serial.print("scaled V:");
    //     Serial.print(led.get());
    //     Serial.print(":");
    //     Serial.println(current);
    // }
    if (led.getGammaValue(current) > limit)
    {
        led.set(--current);
    }
}

void ledOn()
{
    led.on();
    scaleToVSup();
    lightOnSince = millis();
    directionUp = true;
}

void loop()
{
    auto carTrigger = isCarTriggered();
    if (carTrigger == OPENED)
    {
        Serial.println("opened");
        ledOn();
    }
    else if (carTrigger == CLOSED)
    {
        Serial.println("closed");
        led.off();
        directionUp = false;
    }
    else if (isTouchTriggered())
    {
        flash();
        if (!led.done())
        {
            Serial.println("s");
            led.stop();
        }
        else
        {
            directionUp = !directionUp;
            if (directionUp)
            {
                Serial.println("u");
                ledOn();
            }
            else
            {
                Serial.println("d");
                led.off();
            }
        }
    }
    static uint32_t lastScale;
    if (millis() - lastScale > 1000)
    {
        lastScale = millis();
        scaleToVSup();
    }
    evalShutdown();
    FadeLed::update();
    evalStandby();
}