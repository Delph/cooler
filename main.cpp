#include <Arduino.h>


#include <avr/wdt.h>


#include <OneWire.h>
#include <DallasTemperature.h>

// change this to 0 to turn off serial logging etc.
// for when you are happy
#define DEBUG 1

// pin assignments
// the coolers are wired in order, 1-4 down the left, 5-8 down the right
// order them here left-to-right, top-to-bottom for better thermal management
const uint8_t PIN_COOLERS[] = {2, 6, 3, 7, 4, 8, 5, 9};
const uint8_t PIN_TEMPERATURE_SENSOR = 10;
const uint8_t PIN_ALARM = 11;
// const uint8_t PIN_INTERLOCK = 22;

// led inbuilt, just flash it every so often
const uint8_t PIN_WATCH = 13;

// used for seeding the rng to randomise the starting index
const uint8_t PIN_SEED = A0;

const size_t NUM_COOLERS = sizeof(PIN_COOLERS) / sizeof(uint8_t);

// settings
// how often system logic runs to check temperatures etc
const uint32_t TICK_RATE = 500;

// purge time settings
// interval is how long between purging a specific module
// time is how long to purge for
const uint32_t PURGE_INTERVAL = 5 * 60 * 1000;
const uint32_t PURGE_TIME = 10 * 1000;

// the minimum temperature the system will cool to
// in degrees celsius
const float TEMPERATURE_MINIMUM = 16.0f;

// the maximum temperature the system will attempt to maintain
// above this temperature, all cooling elements will be on
const float TEMPERATURE_MAXIMUM = 18.0f;

// the temperature at which to trigger the alarm
const float TEMPERATURE_ALARM_LOW = 14.0f;
const float TEMPERATURE_ALARM_HIGH = 19.0f;

// the temperature at which we arm the alarm
const float TEMPERATURE_ARM = 17.0f;

// how much the temperature goes up for each level
const float TEMPERATURE_INCREMENT = (TEMPERATURE_MAXIMUM - TEMPERATURE_MINIMUM) / static_cast<float>(NUM_COOLERS);

// hysteresis in temperature
// prevents rapid toggling of cooler pads
const float TEMPERATURE_HYSTERESIS = TEMPERATURE_INCREMENT / 2.0f;


// variables
// cooling level -- a la number of coolers on
size_t level = 0;

// this is the next cooler to turn on
size_t head = 0;

// this is the next cooler to turn off
size_t tail = 0;

// time when cooler was last on
uint32_t lastOn[NUM_COOLERS] = {0};
// time when cooler was last off
uint32_t lastOff[NUM_COOLERS] = {0};

// current cooler on purge, if == NUM_COOLERS then none
size_t purge = NUM_COOLERS;

// if true, the temperature has decreased enough for us to arm
bool armed = false;


// temperature sensor
OneWire oneWire(PIN_TEMPERATURE_SENSOR);
DallasTemperature temperatureSensor(&oneWire);


void setup()
{
  #if DEBUG
  Serial.begin(9600);
  Serial.println("Initialization");
  #endif
  pinMode(PIN_WATCH, OUTPUT);

  for (size_t i = 0; i < NUM_COOLERS; ++i)
    pinMode(PIN_COOLERS[i], OUTPUT);
  pinMode(PIN_TEMPERATURE_SENSOR, INPUT);
  pinMode(PIN_ALARM, OUTPUT);
  // pinMode(PIN_INTERLOCK, OUTPUT);

  randomSeed(analogRead(A0));

  // start on a random head/tail
  head = tail = random(0, NUM_COOLERS);

  // turn off the interlock
  // digitalWrite(PIN_INTERLOCK, false);

  // start all off
  for (size_t i = 0; i < NUM_COOLERS; ++i)
    digitalWrite(PIN_COOLERS[i], 1);

  // begin the temperature sensor
  temperatureSensor.begin();

  // enable the watchdog timer at 4s internals, which should be plenty
  wdt_enable(WDTO_4S);
}


void turnCoolerOn(const uint32_t now)
{
  digitalWrite(PIN_COOLERS[head], 0);
  lastOn[head] = now;
  head = (head + 1) % NUM_COOLERS;
}


void turnCoolerOff(const uint32_t now)
{
  digitalWrite(PIN_COOLERS[tail], 0);
  lastOff[tail] = now;
  tail = (tail + 1) % NUM_COOLERS;
}


void loop()
{
  const uint32_t now = millis();
  static uint32_t next = 0;

  // toggle the watch pin
  digitalWrite(PIN_WATCH, (now / 1000) % 2);

  // reset the watchdog timer
  wdt_reset();

  // check if it's time
  if (now < next)
    return;
  next = now + TICK_RATE;

  // do logic
  temperatureSensor.requestTemperaturesByIndex(0);
  float temp = temperatureSensor.getTempCByIndex(0);

  // if we're not armed and under arming temperature
  // arm the alarm
  if (!armed && temp < TEMPERATURE_ARM)
  {
    armed = true;
    // digitalWrite(PIN_INTERLOCK, true);
    #if DEBUG
    Serial.println("ARMED");
    #endif
  }

  // over temperature
  if (armed && temp >= TEMPERATURE_ALARM_HIGH)
  {
    digitalWrite(PIN_ALARM, (now / TICK_RATE) % 2);
    // digitalWrite(PIN_INTERLOCK, false);
    #if DEBUG
    Serial.println("ALARM HIGH");
    #endif
  }
  else if (armed && temp <= TEMPERATURE_ALARM_LOW)
  {
    digitalWrite(PIN_ALARM, (now / (TICK_RATE / 2)) % 2);
    // digitalWrite(PIN_INTERLOCK, false);
    #if DEBUG
    Serial.println("ALARM LOW");
    #endif
  }
  else
  {
    digitalWrite(PIN_ALARM, false);
  }

  // calculate how many coolers should be on
  float levelTemp = TEMPERATURE_MINIMUM + level * TEMPERATURE_INCREMENT;
  if (temp > levelTemp + TEMPERATURE_HYSTERESIS && level < NUM_COOLERS)
  {
    ++level;
    turnCoolerOn(now);
  }
  if (temp < levelTemp && level > 0)
  {
    --level;
    turnCoolerOff(now);
  }

  #if DEBUG
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(", level: ");
  Serial.print(level);
  Serial.print(" (T: ");
  Serial.print(tail);
  Serial.print(", H: ");
  Serial.print(head);
  Serial.println(")");
  #endif

  // see if we should move onto the next cooler as part of purging/limiting use
  // if we're under temperature or flat out then there's nothing to do
  if (level > 0 && level < NUM_COOLERS - 1)
  {
    const auto prev = (head + NUM_COOLERS - 1) % NUM_COOLERS;
    if (lastOn[prev] + PURGE_TIME < now && lastOff[head] + PURGE_INTERVAL < now)
    {
      #if DEBUG
      Serial.print("Purge stepped to ");
      Serial.println(head);
      Serial.print("Tail last on ");
      Serial.print(lastOn[tail]);
      Serial.print(", head last off ");
      Serial.println(lastOff[head]);
      #endif
      turnCoolerOn(now);
      turnCoolerOff(now);
    }
  }
}
