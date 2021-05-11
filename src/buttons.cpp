#include <Arduino.h>
#include <ADCTouch.h>
#include "LowPower.h"
#include <SoftPWM.h>
int ref0; //reference values to remove offset

const uint8_t adc1 = A3;
// const uint8_t adc2 = A4;
const uint8_t trigger = 10;
const uint32_t calibinterval = 10000;
const uint32_t standbyDelay = 20000;
uint32_t lastLight;
bool direction;

void flash(int8_t pin)
{
    auto current = SoftPWMGet(pin);
    auto currentRaw = SoftPWMGetRaw(pin);
    SoftPWMSet(pin, currentRaw < 128 ? currentRaw + 64 : 0, true);
    delay(10);
    SoftPWMSet(pin, currentRaw, true);
    SoftPWMSet(pin, current, false);
}
void calibrate()
{
    flash(LED_BUILTIN);
    ref0 = ADCTouch.read(adc1, 1000);
}

bool getTouch()
{
    return trigger < (ADCTouch.read(adc1, 500) - ref0);
}

bool isTouchTriggered(bool touch)
{
    static bool active;
    static uint32_t lastTouch;

    if (millis() - lastTouch > calibinterval)
    {
        lastTouch = millis();
        calibrate();
    }

    if (!touch)
    {
        active = false;
        return false;
    }

    if (active || (millis() - lastTouch < 50))
        return false;
    lastTouch = millis();
    active = true;
    return true;
}

void setup()
{
    SoftPWMBegin();
    SoftPWMSet(LED_BUILTIN, 0);
    // SoftPWMSetFadeTime(LED_SIGNAL, 200, 200);
    SoftPWMSetFadeTime(LED_BUILTIN, 4000, 4000);
    // SoftPWMSet(LED_SIGNAL, 255);
    // SoftPWMSet(LED_SIGNAL, 0);

    Serial.begin(115200);

    calibrate();
}

void loop()
{
    if (isTouchTriggered(getTouch()))
    {
        flash(LED_BUILTIN);
        uint8_t value = SoftPWMGetRaw(LED_BUILTIN);
        switch (value)
        {
        case 0:
            Serial.print("0");
            direction = HIGH;
            SoftPWMSet(LED_BUILTIN, 255);
            break;
        case 255:
            Serial.print("5");
            direction = LOW;
            SoftPWMSet(LED_BUILTIN, 0);
            break;

        default:
            if (SoftPWMisFading(LED_BUILTIN))
            {
                Serial.print("s");
                SoftPWMSet(LED_BUILTIN, SoftPWMGetRaw(LED_BUILTIN));
            }
            else
            {
                if (direction == HIGH)
                {
                    Serial.print("d");
                    SoftPWMSet(LED_BUILTIN, 0);
                }
                else
                {
                    Serial.print("u");
                    SoftPWMSet(LED_BUILTIN, 255);
                }
                direction = !direction;
            }

            break;
        }
    }
    if (SoftPWMGet(LED_BUILTIN) > 0)
    {
        lastLight = millis();
    }

    if (millis() - lastLight > standbyDelay)
    {
        Serial.println("stdby");
        LowPower.powerStandby(SLEEP_4S, ADC_OFF, BOD_OFF);
    }

    // delay(10);
}