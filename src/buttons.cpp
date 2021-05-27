#include <Arduino.h>
#include "LowPower.h"
// #include <SoftPWM.h>
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
volatile int8_t _carTrigger = 0;
#define OPENED 1
#define CLOSED -1
#define FADETIME 5000

FadeLed led(FETPort);

// void PWMSet(uint8_t duty, bool hardset = false)
// {
//     SoftPWMSet(LED_BUILTIN, duty, hardset);
//     SoftPWMSet(FETPort, duty, hardset);
// }

// uint8_t PWMGet(bool raw = false)
// {
//     if (raw)
//     {
//         return SoftPWMGetRaw(LED_BUILTIN);
//     }
//     return SoftPWMGet(LED_BUILTIN);
// }
// bool PWMisFading()
// {
//     return SoftPWMisFading(LED_BUILTIN);
// }

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

    // if (!_touched)
    // {
    //     return false;
    // }

    if (millis() - lastTouch < 200)
        _touched = false;
    else if (_touched)
        lastTouch = millis();
    return _touched;
}

int8_t isCarTriggered()
{
    static uint32_t lastCarTrigger;

    // if (_carTrigger == 0)
    // {
    //     return 0;
    // }
    // auto t = _carTrigger;

    if (millis() - lastCarTrigger < 100)
        _carTrigger = 0;
    else if (_carTrigger)
        lastCarTrigger = millis();

    return _carTrigger;
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
volatile uint16_t res;
void ISR1()
{
#define ROUNDS 10
    res = 0;
    for (size_t i = 0; i < ROUNDS; i++)
    {
        res += analogRead(carALight);
    }
    _carTrigger = res < 250 * ROUNDS ? OPENED : CLOSED;
}

void setup()
{
    pinMode(touchPin, INPUT);
    pinMode(carPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(touchPin), ISR0, RISING);
    attachInterrupt(digitalPinToInterrupt(carPin), ISR1, CHANGE);

    // SoftPWMBegin();
    // PWMSet(0);
    // SoftPWMSetFadeTime(ALL, 4000, 4000);

    //Set update interval to 10ms
    FadeLed::setInterval(10);

    //set fade time to 4 seconds
    led.setTime(FADETIME);

    Serial.begin(115200);
}

void loop()
{
    FadeLed::update();

    auto carTrigger = isCarTriggered();
    if (carTrigger == OPENED)
    {

        Serial.print(res);
        Serial.println(" door-opened");
        led.on();
    }
    else if (carTrigger == CLOSED)
    {
        Serial.print(res);
        Serial.println(" door-closed");
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

        // flash(LED_BUILTIN);
        // auto value = led.getCurrent();
        // switch (value)
        // {
        // case 0:
        //     Serial.print("9");
        //     direction = HIGH;
        //     led.on();
        //     break;
        // case 100:
        //     Serial.print("0");
        //     direction = LOW;
        //     led.off();
        //     break;

        // default:
        //     if (!led.done())
        //     {
        //         Serial.print("s");
        //         PWMSet(PWMGet(true));
        //     }
        //     else
        //     {
        //         if (direction == HIGH)
        //         {
        //             Serial.print("d");
        //             led.off();
        //         }
        //         else
        //         {
        //             Serial.print("u");
        //             led.on();
        //         }
        //         direction = !direction;
        //     }
        //     break;
        // }
    }

    evalStandby();
}