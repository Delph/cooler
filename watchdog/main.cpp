#include <Arduino.h>

// pin assignments
const uint8_t PIN_ALARM = 0;

// this is configured to run as an interrupt
// so it must be pin 2 or 3 for nano/uno
const uint8_t PIN_WATCH = 2;

const uint32_t TIMEOUT = 10 * 1000; // 10 s

// must be volatile due to interrupt
volatile uint32_t last = 0;

void isr()
{
  last = millis();
}

void setup()
{
  pinMode(PIN_ALARM, OUTPUT);
  attachInterrupt(
    digitalPinToInterrupt(PIN_WATCH),
    isr,
    CHANGE);
}

void loop()
{
  const uint32_t now = millis();

  if (last && last + TIMEOUT > now)
  {
    const bool on = (now / 500) % 2;
    digitalWrite(PIN_ALARM, on);
  }
}
