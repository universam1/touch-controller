#include "Arduino.h"
#include "FadeLed.h"

unsigned int FadeLed::_interval = 50;
unsigned int FadeLed::_millisLast = 0;
// byte FadeLed::_ledCount = 0;
// FadeLed* FadeLed::_ledList[FADE_LED_MAX_LED];
//----------------------------------------------------------------------------------------------------
double findMaxForPow(uint16_t maxInputValue)
{
  double exp = 1;
  double expLast = 0;
  double expadj = 1;
  double v = 0;

  while (exp != expLast)
  {
    expLast = exp;
    v = pow(maxInputValue, exp);
    if (v > FADE_LED_RESOLUTION)
    {
      exp -= expadj;
      expadj = expadj / 10;
    }
    exp += expadj;
  }
  Serial.print(F("For a maximum input value of: "));
  Serial.print(maxInputValue);
  Serial.print(F("  Use: "));
  Serial.print(exp, 10);
  Serial.println(F("  In pow(..) function "));
  return exp;
}

FadeLed::FadeLed(byte pin) : FadeLed(pin, FADE_LED_RESOLUTION) {}

FadeLed::FadeLed(byte pin, flvar_t biggestStep) : _pin(pin),
                                                  _count(0),
                                                  _countMax(40),
                                                  //_countMax(2000 / _interval),
                                                  _constTime(false),
                                                  // _gammaLookup(gammaLookup),
                                                  _biggestStep(biggestStep)
{
  _power = findMaxForPow(biggestStep);
  pinMode(_pin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  cli();        // Disable all interrupts
  TCCR1A = 0;   // Clear all flags in control register A
  TCCR1B = 0;   // Clear all flags in control register B
  TCNT1 = 0;    // Zero timer 1 count
  TIFR1 = 0xFF; // Clear all flags in interrupt flag register

  // Set fast PWM Mode 14
  TCCR1A |= (1 << WGM11);
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << WGM13);

  // Set prescaler to 64 and starts PWM
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS11);

  TIMSK1 |= (1 << OCIE1A); // Enable timer compare interrupt
  TIMSK1 |= (1 << TOIE1);  // Enable timer overflow interrupt

  // Set PWM frequency/top value
  ICR1 = FADE_LED_RESOLUTION;
  OCR1A = 0;
  sei(); // Enable all interrupts
}

ISR(TIMER1_OVF_vect){                   // Timer1 overflow interrupt service routine
  PORTB |= _BV(PORTB5);                 // Turn LED (pin 13) on
}

ISR(TIMER1_COMPA_vect){                 // Timer1 compare interrupt service routine
  PORTB &= ~_BV(PORTB5);                // Turn LED off
}

FadeLed::~FadeLed()
{
}

void FadeLed::begin(flvar_t val)
{
  //set to both so no fading happens
  _setVal = val;
  _curVal = val;
  analogWrite(this->_pin, pow(val, this->_power));
}

void FadeLed::set(flvar_t val)
{

  /** edit 2016-11-17
   *  Fix so you can set it to a new value while 
   *  fading in constant speed. And impossible to set 
   *  a new value in constant time.
   */
  if (_setVal != val)
  {
    /** edit 2016-11-30
     *  Fixed out of range possibility
     */
    //check for out of range
    if (val > _biggestStep)
    {
      //if bigger then allowed, use biggest value
      val = _biggestStep;
    }

    //if it's now fading we have to check how to change it
    if (!done())
    {
      //setting new val while fading in constant time not possible
      if (_constTime)
      {
        return;
      }
      //if in constant speed the new val is in same direction and not passed yet
      else if (((_startVal < _setVal) && (_curVal < val)) || //up
               ((_startVal > _setVal) && (_curVal > val)))
      { //down
        //just set a new val
        _setVal = val;
        return;
      }
    }

    //if we make it here it's or finished fading
    //or constant speed in other direction
    //save and reset
    _setVal = val;
    _count = 1;

    //and start fading from current position
    _startVal = _curVal;
  }
}

flvar_t FadeLed::get()
{
  return _setVal;
}

flvar_t FadeLed::getCurrent()
{
  return _curVal;
}

bool FadeLed::done()
{
  return _curVal == _setVal;
}

void FadeLed::on()
{
  this->set(_biggestStep);
}

void FadeLed::off()
{
  this->set(0);
}

void FadeLed::beginOn()
{
  this->begin(_biggestStep);
}

void FadeLed::setTime(unsigned long time, bool constTime)
{
  //Calculate how many times interval need to pass in a fade
  this->_countMax = time / _interval;
  this->_constTime = constTime;
}

bool FadeLed::rising()
{
  return (_curVal < _setVal);
}

bool FadeLed::falling()
{
  return (_curVal > _setVal);
}

void FadeLed::stop()
{
  _setVal = _curVal;
}

flvar_t FadeLed::getBiggestStep()
{
  return _biggestStep;
}

void FadeLed::updateThis()
{
  //need to fade up
  if (_curVal < _setVal)
  {
    flvar_t newVal;

    //we always start at the current level saved in _startVal
    if (_constTime)
    {
      //for constant fade time we add the difference over countMax steps
      newVal = _startVal + _count * (_setVal - _startVal) / _countMax;
    }
    else
    {
      //for constant fade speed we add the full resolution over countMax steps
      newVal = _startVal + _count * _biggestStep / _countMax;
    }

    //check if new
    if (newVal != _curVal)
    {
      //check for overflow
      if (newVal < _curVal)
      {
        _curVal = _biggestStep;
      }
      //Check for overshoot
      else if (newVal > _setVal)
      {
        _curVal = _setVal;
      }
      //Only if the new value is good we use that
      else
      {
        _curVal = newVal;
      }

      analogWrite(this->_pin, pow(_curVal, this->_power));
    }
    _count++;
  }
  //need to fade down
  else if (_curVal > _setVal)
  {
    flvar_t newVal;

    //we always start at the current level saved in _startVal
    if (_constTime)
    {
      //for constant fade time we subtract the difference over countMax steps
      newVal = _startVal - _count * (_startVal - _setVal) / _countMax;
    }
    else
    {
      //for constant fade speed we subtract the full resolution over countMax steps
      newVal = _startVal - _count * _biggestStep / _countMax;
    }

    //check if new
    if (newVal != _curVal)
    {
      //check for overflow
      if (newVal > _curVal)
      {
        _curVal = 0;
      }
      //Check for overshoot
      else if (newVal < _setVal)
      {
        _curVal = _setVal;
      }
      //Only if the new value is good we use that
      else
      {
        _curVal = newVal;
      }
      analogWrite(this->_pin, pow(_curVal, this->_power));
    }
    _count++;
  }
}

void FadeLed::setInterval(unsigned int interval)
{
  _interval = interval;
}

void FadeLed::update()
{
  unsigned int millisNow = millis();

  if (millisNow - _millisLast > _interval)
  {
    /**
     *  Fix issue #13
     *  Weird fade when not calling update() while not fading     
     */
    if (millisNow - _millisLast > (_interval << 1))
    {
      _millisLast = millisNow;
    }
    else
    {
      _millisLast += _interval;
    }

    //update every object
    // for(byte i = 0; i < _ledCount; i++){
    _ledList->updateThis();
    // }
  }
}
