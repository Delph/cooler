#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// change this to 0 to turn off serial logging etc.
// for when you are happy
#define DEBUG 1

// pin assignments
const uint8_t PIN_COOLERS[] = {2, 3, 4, 5, 6, 7, 8, 9};
const size_t NUM_COOLERS = sizeof(PIN_COOLERS) / sizeof(uint8_t);
const uint8_t PIN_SENSOR = 10;
const uint8_t PIN_ALARM = 11;

// used to tell the watchdog arduino we're still running
const uint8_t PIN_WATCH = 13;


// settings
// this is how often PIN_WATCH is toggled
const uint32_t PULSE_RATE = 3000;

// how often system logic runs to check temperatures etc
// independant of PULSE RATE
const uint32_t TICK_RATE = 500;

// purge time settings
// interval is how long between purging a specific module
// time is how long to purge for
const uint32_t PURGE_INTERVAL = 5 * 60 * 1000;
const uint32_t PURGE_TIME = 10 * 1000;

// the minimum temperature the system will cool to
// in degrees celsius
const float TEMP_MIN = 18.0f;

// how much the temperature goes up for each level
const float TEMP_INCREMENT = 0.25f;

// hysteresis in temperature
// prevents rapid toggling of cooler pads
const float TEMP_HYSTERESIS = 0.125f;

// the temperature at which to trigger the alarm
const float TEMP_ALARM = 24.0f;

// the temperature at which we arm the alarm
const float TEMP_ARM = 19.0f;

// variables
// cooling level -- a la number of coolers on
size_t level = 0;

// time when cooler was last on -- for purge
uint32_t lastOn[NUM_COOLERS] = {0};

// current cooler on purge, if == NUM_COOLERS then none
size_t purge = NUM_COOLERS;

// if true, the temperature has decreased enough for us to arm
bool armed = false;

// temperature sensor
OneWire oneWire(PIN_SENSOR);
DallasTemperature sensor(&oneWire);

void setup()
{
  #if DEBUG
  Serial.begin(9600);
  Serial.println("Initialization");
  #endif
  pinMode(PIN_WATCH, OUTPUT);

  for (size_t i = 0; i < NUM_COOLERS; ++i)
    pinMode(PIN_COOLERS[i], OUTPUT);
  pinMode(PIN_SENSOR, INPUT);
  pinMode(PIN_ALARM, OUTPUT);

  // start all off
  for (size_t i = 0; i < NUM_COOLERS; ++i)
    digitalWrite(PIN_COOLERS[i], 1);

  sensor.begin();
}

void loop()
{
  const uint32_t now = millis();
  static uint32_t next = 0;

  // toggle the watch pin
  digitalWrite(PIN_WATCH, (now / 1000) % 2);

  // check if it's time
  if (next < now)
    return;
  next = now + TICK_RATE;

  // do logic
  sensor.requestTemperaturesByIndex(0);
  float temp = sensor.getTempCByIndex(0);

  // if we're not armed and under arming temperature
  // arm the alarm
  if (!armed && temp < TEMP_ARM)
  {
    armed = true;
    #if DEBUG
    Serial.print("ARMED");
    #endif
  }

  // over temperature
  if (armed && temp >= TEMP_ALARM)
  {
    digitalWrite(PIN_ALARM, (now / TICK_RATE) % 2);
    #if DEBUG
    Serial.print("ALARM");
    #endif
  }

  // calculate how many coolers should be on
  float levelTemp = TEMP_MIN + level * TEMP_INCREMENT;
  if (temp > levelTemp + TEMP_HYSTERESIS)
    ++level;
  if (temp < levelTemp)
    --level;

  // turn them on (or off)
  size_t i = 0;
  for (; i < level; ++i)
  {
    // don't modify purging coolers
    if (purge == i)
      continue;
    digitalWrite(PIN_COOLERS[i], 0); // active LOW
    lastOn[i] = now;
  }
  for (; i < NUM_COOLERS; ++i)
  {
    // don't modify purging coolers
    if (purge == i)
      continue;
    digitalWrite(PIN_COOLERS[i], 1);
  }

  if (purge == NUM_COOLERS)
  {
    size_t oldest = 0;
    for (size_t i = 1; i < NUM_COOLERS; ++i)
    {
      if (lastOn[i] < lastOn[oldest])
        oldest = i;
    }

    if (lastOn[oldest] > PURGE_INTERVAL)
    {
      purge = oldest;
      lastOn[purge] = now;
      digitalWrite(PIN_COOLERS[purge], 0);
    }
  }
  else
  {
    if (lastOn[purge] + PURGE_TIME > now)
    {
      lastOn[purge] = now;
      digitalWrite(PIN_COOLERS[purge], 1);
      purge = NUM_COOLERS;
    }
  }
}
