#include <Arduino.h>
#include "LowPower.h"
#include <SoftPWM.h>

const uint32_t standbyDelay = 2000;
uint32_t lastLight;
bool direction;
const uint8_t touchPin = 2;
const uint8_t carPin = 3;
const uint8_t FETPort = 5;
volatile bool touched = false;
volatile bool carTriggered = false;

void PWMSet(uint8_t duty, bool hardset = false)
{
    SoftPWMSet(LED_BUILTIN, duty, hardset);
    SoftPWMSet(FETPort, duty, hardset);
}

uint8_t PWMGet(bool raw = false)
{
    if (raw)
    {
        return SoftPWMGetRaw(LED_BUILTIN);
    }
    return SoftPWMGet(LED_BUILTIN);
}
bool PWMisFading()
{
    return SoftPWMisFading(LED_BUILTIN);
}

void flash(int8_t pin)
{
    auto current = PWMGet();
    auto currentRaw = PWMGet(true);
    PWMSet(currentRaw < 128 ? currentRaw + 64 : 0, true);
    delay(10);
    PWMSet(currentRaw, true);
    PWMSet(current, false);
}

bool isTouchTriggered()
{
    static uint32_t lastTouch;

    if (!touched)
    {
        return false;
    }
    touched = false;

    if (millis() - lastTouch < 50)
        return false;
    lastTouch = millis();
    return true;
}

bool isCarTriggered()
{
    static uint32_t lastCarTrigger;

    if (!carTriggered)
    {
        return false;
    }
    carTriggered = false;

    if (millis() - lastCarTrigger < 500)
        return false;
    lastCarTrigger = millis();

    return true;
}

void ISR0()
{
    touched = true;
}
void ISR1()
{
    carTriggered = true;
}

void setup()
{
    pinMode(touchPin, INPUT);
    pinMode(carPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(touchPin), ISR0, RISING);
    attachInterrupt(digitalPinToInterrupt(carPin), ISR1, FALLING);

    SoftPWMBegin();
    PWMSet(0);
    SoftPWMSetFadeTime(ALL, 4000, 4000);

    Serial.begin(115200);
}
void evalStandby()
{
    if (PWMGet(true) > 0)
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

void loop()
{
    if (isCarTriggered())
    {
        Serial.println("door");
        PWMSet(255);
    }
    else if (isTouchTriggered())
    {
        flash(LED_BUILTIN);
        uint8_t value = PWMGet(true);
        switch (value)
        {
        case 0:
            Serial.print("0");
            direction = HIGH;
            PWMSet(255);
            break;
        case 255:
            Serial.print("5");
            direction = LOW;
            PWMSet(0);
            break;

        default:
            if (PWMisFading())
            {
                Serial.print("s");
                PWMSet(PWMGet(true));
            }
            else
            {
                if (direction == HIGH)
                {
                    Serial.print("d");
                    PWMSet(0);
                }
                else
                {
                    Serial.print("u");
                    PWMSet(255);
                }
                direction = !direction;
            }
            break;
        }
    }

    evalStandby();
}