#include <Arduino.h>
#include "LowPower.h"
#include <FadeLed.h>
#include <Bounce2.h>
Bounce bounce = Bounce();

const uint32_t standbyDelay = 5UL * 1000UL;
uint32_t lastLight;
const uint32_t shutdownDelay = 120UL * 60UL * 1000UL;
uint32_t lightOnSince;

bool directionUp;
const uint8_t buttonPin = 2;
const uint8_t carPin = 3;
const uint8_t carALight = A1;
const uint8_t carABat = A2;
const uint8_t FETPort = 5;
volatile bool _touched = false;
volatile bool _carTrigger = false;
#define OPENED 1
#define CLOSED -1
#define FADETIME 2500

#define OFF 0
#define CAR_ON 1
#define BUTTON_ON 2
uint8_t state = OFF;

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
    led.begin(currentRaw < 50 ? (currentRaw + 25) : (currentRaw - 25));
    FadeLed::update();
    delay(20);
    led.begin(currentRaw);
    FadeLed::update();
    led.set(current);
    FadeLed::update();
    delay(50);
}

// bool isTouchTriggered()
// {
//     static uint32_t lastTouch;
//     bool t = false;

//     if (millis() - lastTouch < 300)
//         _touched = false;
//     else if (_touched)
//     {
//         lastTouch = millis();
//         t = true;
//     }
//     return t;
// }

int8_t isCarTriggered()
{
#define ROUNDS 8
    static uint32_t lastCarTrigger;

    if (millis() - lastCarTrigger < 1000)
        _carTrigger = false;
    else if (_carTrigger)
    {
        lastCarTrigger = millis();
        _carTrigger = false;

        delay(300); // let cap discharge

        int diffBat = 0;
        int lastBat = analogRead(carABat);
        int diffLight = 0;
        int lastLight = analogRead(carALight);
        Serial.print("lastBat: ");
        Serial.print(lastBat);
        Serial.print(" lastLight: ");
        Serial.println(lastLight);
        for (uint8_t i = 0; i < ROUNDS; i++)
        {
            delay(20);
            int valBat = analogRead(carABat);
            delay(20);
            int valLight = analogRead(carALight);
            Serial.print(valBat);
            Serial.print(" ");
            Serial.println(valLight);
            diffBat += valBat - lastBat;
            diffLight += valLight - lastLight;
            lastBat = valBat;
            lastLight = valLight;
        }
        Serial.print("diff: ");
        Serial.print(diffBat);
        Serial.print(" ");
        Serial.println(diffLight);
        // Supply voltage check
        if (diffBat < -10 * ROUNDS || lastBat < 400)
        {
            Serial.println("powered off");
            return CLOSED;
        }
        // Ground line check
        return diffLight < -5 * ROUNDS ? OPENED : CLOSED;
    }

    return 0;
}

void evalStandby()
{
    if (led.getCurrent() > 0)
        lastLight = millis();
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
    // _touched = true;
}

void ISR1()
{
    _carTrigger = true;
}

void setup()
{
    Serial.begin(115200);

    // pinMode(buttonPin, INPUT_PULLUP);

    // SELECT ONE OF THE FOLLOWING :
    // 1) IF YOUR INPUT HAS AN INTERNAL PULL-UP
    bounce.attach(buttonPin, INPUT_PULLUP); // USE INTERNAL PULL-UP
    // 2) IF YOUR INPUT USES AN EXTERNAL PULL-UP
    // bounce.attach( BOUNCE_PIN, INPUT ); // USE EXTERNAL PULL-UP

    // DEBOUNCE INTERVAL IN MILLISECONDS
    bounce.interval(40); // interval in ms
    pinMode(carPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(buttonPin), ISR0, FALLING);
    attachInterrupt(digitalPinToInterrupt(carPin), ISR1, CHANGE);

    FadeLed::setInterval(10);
    led.setTime(FADETIME);
}

void scaleToVSup()
{
    float val;
    for (size_t i = 0; i < ROUNDS; i++)
    {
        val += analogRead(carABat);
        delay(10);
    }
    float volt = val * UCARCONV;
    Serial.print("UBat: ");
    Serial.print(volt);
    Serial.print("   raw: ");
    Serial.print(val / ROUNDS);
    float factor = UCARMAX / volt;
    if (factor > 1.0)
        factor = 1.0;

    auto current = led.get();
    uint8_t limit = factor * 255.0f;
    Serial.print("    limit: ");
    Serial.print(limit);
    Serial.print("    gamma: ");
    Serial.print(led.getGammaValue(current));

    Serial.print("    Ulight: ");
    Serial.println(analogRead(carALight));
    if (led.getGammaValue(current) > limit)
        led.set(--current);
}

void ledTo(uint8_t val, bool quick = false)
{
    led.setTime(quick ? FADETIME / 3 : FADETIME, true);
    directionUp = val > 0;
    led.set(val);
    if (val >= 100)
        scaleToVSup();
    if (val != 0)
        lightOnSince = millis();
    else
        state = OFF;
}

void evalTurnOff()
{
    if (led.get() == 0)
        return;
    if (millis() - lightOnSince > shutdownDelay)
    {
        Serial.println("\nshutdown");
        ledTo(0);
    }
}

void loop()
{

    auto carTrigger = isCarTriggered();
    if (carTrigger == OPENED)
    {
        Serial.println("opened");
        if (led.get() == 0 && state == OFF)
        {
            state = CAR_ON;
            ledTo(30, true);
        }
    }
    else if (carTrigger == CLOSED)
    {
        Serial.println("closed");
        if (state == CAR_ON)
        {
            directionUp = false;
            ledTo(0, true);
        }
    }
bounce.update();
    if (bounce.changed())
    {
        // THE STATE OF THE INPUT CHANGED
        // GET THE STATE
        int deboucedInput = bounce.read();
        // IF THE CHANGED VALUE IS HIGH
        if (deboucedInput == LOW)
        {

            // else if (isTouchTriggered())
            // {
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
                    state = BUTTON_ON;
                    if (led.getCurrent() == 0)
                        ledTo(20, false);
                    else
                        ledTo(100, false);
                }
                else
                {
                    Serial.println("d");
                    ledTo(0, false);
                }
            }
        }
    }
    static uint32_t lastScale;
    if (millis() - lastScale > 2000)
    {
        lastScale = millis();
        scaleToVSup();
    }
    evalTurnOff();
    FadeLed::update();
    evalStandby();
}