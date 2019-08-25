#include <Arduino.h>

// change to 1 to enable automatic reset
#define ENABLE_RESET 0

// pin assignments
const uint8_t PIN_ALARM = 0;

// this is configured to run as an interrupt
// so it must be pin 2 or 3 for nano/uno
const uint8_t PIN_WATCH = 2;

#if ENABLE_RESET
// connect this to the RST pin of the cooler
const uint8_t PIN_RESET = 4;
#endif

const uint32_t TIMEOUT = 10 * 1000; // 10 s

// must be volatile due to interrupt
volatile uint32_t last = 0;

#if ENABLE_RESET
uint32_t lastReset = 0;
#endif

void isr()
{
  last = millis();
}

void setup()
{
  pinMode(PIN_ALARM, OUTPUT);
  pinMode(PIN_WATCH, INPUT);
  #if ENABLE_RESET
  pinMode(PIN_RESET, OUTPUT);
  #endif
  attachInterrupt(digitalPinToInterrupt(PIN_WATCH), isr, CHANGE);
}

void loop()
{
  const uint32_t now = millis();

  if (last && last + TIMEOUT > now)
  {
    const bool on = (now / 500) % 2;
    digitalWrite(PIN_ALARM, on);

    #if ENABLE_RESET
    if (lastReset + TIMEOUT * 2 > now)
    {
      digitalWrite(PIN_RESET, 0);
      delay(10);
      digitalWrite(PIN_RESET, 1);
      lastReset = now;
    }
    #endif
  }
}
