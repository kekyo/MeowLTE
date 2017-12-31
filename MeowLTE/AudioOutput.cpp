#include "AudioOutput.h"

#include <assert.h>

#define BUFFER_LENGTH 128
#define BUFFER_ELEMENTS 3
#define BUFFER_SIZE (BUFFER_LENGTH * sizeof(int16_t))

extern WioLTE Wio;

static HardwareTimer timer(1);

static volatile uint8_t readElement = 0;
static volatile uint8_t writeElement = 0;
static volatile uint16_t writePosition = 0;

static int16_t buffer[BUFFER_ELEMENTS][BUFFER_LENGTH];
static uint16_t length[BUFFER_ELEMENTS];

static void TimerHandler()
{
  if (readElement == writeElement)
  {
    return;
  }

  uint16_t v = (uint16_t)(buffer[writeElement][writePosition] + 32768);
  WioLTEDac::Write(WioLTEDac::DAC1, v >> 4);

  writePosition++;
  if (writePosition < length[writeElement])
  {
    return;
  }

  writePosition = 0;

  uint8_t nextElement = writeElement + 1;
  if (nextElement >= BUFFER_ELEMENTS)
  {
    nextElement = 0;
  }
  writeElement = nextElement;
}

// Initialize Grove and DAC
bool InitializeDac()
{
  SerialUSB.println("### Initialize Grove and DAC.");

  WioLTEDac::Init((WioLTEDac::DACChannel)(WioLTEDac::DAC1 | WioLTEDac::DAC2));
  WioLTEDac::Write(WioLTEDac::DAC1, ((uint16_t)32768) >> 4);
  WioLTEDac::Write(WioLTEDac::DAC2, 0);

  delay(100);

  Wio.PowerSupplyGrove(true);

  delay(300);

  return true;
}

bool InitializeTimer()
{
  SerialUSB.println("### Initialize timer.");

  timer.pause();

  timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  timer.setCompare(TIMER_CH1, 1);
  timer.attachCompare1Interrupt(TimerHandler);

  return true;
}

void OutputPcm16Audio(Pcm16Reader& reader)
{
  timer.pause();

  readElement = 0;
  writeElement = 0;
  writePosition = 0;

  const uint32_t period = (uint32_t)((10000000 / reader.SampleRate() + 5) / 10);
  timer.setPeriod(period);

  timer.refresh();
  timer.resume();

  const uint32_t size = reader.Size();
  uint32_t position = 0;

  while (position < size)
  {
    uint8_t nextElement = readElement + 1;
    if (nextElement >= BUFFER_ELEMENTS)
    {
      nextElement = 0;
    }

    if (nextElement == writeElement)
    {
      continue;
    }

    uint32_t req = size - position;
    req = (req > BUFFER_SIZE) ? BUFFER_SIZE : req;

    const uint16_t read = (uint16_t)reader.ReadFragment(&buffer[readElement][0], (uint16_t)req);

    length[readElement] = read / sizeof(uint16_t);
    position += read;

    readElement = nextElement;
  }
}
