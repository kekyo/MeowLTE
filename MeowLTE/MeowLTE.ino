#include <config.h>
#include <WioLTEforArduino.h>
#include <SD.h>		// https://github.com/Seeed-Studio/SD

////////////////////////////////////////////////////////////
// Configuration of MeowLTE

#define FILENAME "meow.wav"
#define POST     "ﾆｬｰﾝ"

#include "MeowLTEConfig.h"

////////////////////////////////////////////////////////////

struct RiffHeader {
  uint32_t riff;
  int32_t  size;
  uint32_t type;
};

struct FormatChunk {
  uint32_t id;
  int32_t size;
  int16_t format;
  uint16_t channels;
  uint32_t samplerate;
  uint32_t bytepersec;
  uint16_t blockalign;
  uint16_t bitswidth;
};

struct FormatChunkEx {
  FormatChunk formatChunk;
  uint16_t extended_size;
  uint8_t extended [];
};

struct DataChunk {
  uint32_t id;
  int32_t size;
  //uint8_t waveformData [];
};

////////////////////////////////////////////////////////////

#define IFTTT_MAKER_WEBHOOKS_URL "https://maker.ifttt.com/trigger/" IFTTT_MAKER_WEBHOOKS_EVENT "/with/key/" IFTTT_MAKER_WEBHOOKS_KEY

#define BUFFER_LENGTH 128
#define BUFFER_ELEMENTS 3
#define BUFFER_SIZE (BUFFER_LENGTH * sizeof(int16_t))

WioLTE Wio;
File myFile;
HardwareTimer timer(1);

int offset = -1;
int size = -1;
int position = 0;

RiffHeader header;
FormatChunk format;

static void TimerHandler();
static void Meow();
static void Post();

////////////////////////////////////////////////////////////

// Initialize Grove and DAC
static bool InitializeDac()
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

// Initialize SD card and wave file
static bool InitializeSD()
{
  SerialUSB.println("### Initialize SD card.");
  
  if (!SD.begin())
  {
    SerialUSB.println("### ERROR 0 ###");
    return false;
  }

  SerialUSB.println("### Reading from "FILENAME);

  myFile = SD.open(FILENAME);
  if (!myFile)
  {
    SerialUSB.println("### ERROR 1 ###");
    return false;
  }

  if (myFile.read(&header, sizeof header) != sizeof header)
  {
    myFile.close();
    SerialUSB.println("### ERROR 2 ###");
    return false;
  }

  if ((header.riff != 0x46464952) || (header.type != 0x45564157))
  {
    myFile.close();
    SerialUSB.println("### ERROR 3 ###");
    return false;
  }
  
  if (myFile.read(&format, sizeof format) != sizeof format)
  {
    myFile.close();
    SerialUSB.println("### ERROR 4 ###");
    return false;
  }

  // DIRTY HACK: fmt chunk.
  if ((format.id != 0x20746d66)
    && (format.format != 0x0001)  // PCM
    && (format.channels != 0x0001) // Mono
    && (format.bitswidth != 16))
  {
    myFile.close();
    SerialUSB.println("### ERROR 5 ###");
    return false;
  }

  int ext = format.size + sizeof(format.id) + sizeof(format.size) - sizeof format;
  if (ext != 0)
  {
    myFile.close();
    SerialUSB.println("### ERROR 6 ###");
    return false;
  }

  DataChunk data;
  if (myFile.read(&data, sizeof data) != sizeof data)
  {
    myFile.close();
    SerialUSB.println("### ERROR 7 ###");
    return false;
  }

  if (data.id != 0x61746164)
  {
    myFile.close();
    SerialUSB.println("### ERROR 8 ###");
    return false;
  }

  offset = sizeof header + sizeof format + sizeof data;
  size = data.size;

  SerialUSB.println("### Wave file ready: "FILENAME);
  
  return true;
}

static bool InitializeTimer()
{
  SerialUSB.println("### Initialize timer.");

  uint32_t period = (uint32_t)((10000000 / format.samplerate + 5) / 10);
  timer.setPeriod(period);

  timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  timer.setCompare(TIMER_CH1, 1);
  timer.attachCompare1Interrupt(TimerHandler); 

  timer.refresh();
  timer.resume();

  return true;
}

static bool InitializeLteModem()
{
  SerialUSB.println("### Initialize LTE modem.");

  Wio.PowerSupplyLTE(true);

  for (uint8_t count1 = 0; count1 < 10; count1++)
  {
    delay(1000);

    if (Wio.TurnOnOrReset())
    {
      delay(500);

      for (uint8_t count2 = 0; count2 < 10; count2++)
      {
        SerialUSB.println("### Connecting to \""APN"\".");

        if (Wio.Activate(APN, USERNAME, PASSWORD))
        {
          SerialUSB.println("### Connected.");
          return true;
        }

        delay(1000);
      }

      SerialUSB.println("### ERROR 10 ###");
      return false;
    }

    SerialUSB.println("### Modem is resetting.");
  }

  SerialUSB.println("### ERROR 9 ###");

  return false;
}

////////////////////////////////////////////////

void setup()
{
  Wio.Init();

  delay(200);

  SerialUSB.println("");
  SerialUSB.println("--- START MeowLTE -------------------------------------------");

  ////////////////////////////////////////////////
  // Initialize Grove and DAC

  if (InitializeDac() == false)
  {
    return;
  }

  ////////////////////////////////////////////////
  // SD card and wave file

  if (InitializeSD() == false)
  {
    return;
  }
  
  ////////////////////////////////////////////////
  // Timer

  if (InitializeTimer() == false)
  {
    return;
  }

  ////////////////////////////////////////////////
  // Meow

  Meow();

  ////////////////////////////////////////////////
  // LTE modem

  if (InitializeLteModem() == false)
  {
    return;
  }

  ////////////////////////////////////////////////
  // Post

  Post();
}

////////////////////////////////////////////////////////////

volatile uint8_t readElement = 0;
volatile uint8_t writeElement = 0;
volatile uint16_t writePosition = 0;

int16_t buffer[BUFFER_ELEMENTS][BUFFER_LENGTH];
uint16_t length[BUFFER_ELEMENTS];

////////////////////////////////////////////////////////////

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

static void Meow()
{
  while (1)
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
  
    if (position >= size)
    {
      myFile.seek(offset);
      position = 0;
      break;
    }
        
    int req = size - position;
    req = (req > BUFFER_SIZE) ? BUFFER_SIZE : req;
    
    int read = myFile.read(&buffer[readElement][0], req);
    length[readElement] = req / sizeof(uint16_t);
    position += req;
    
    readElement = nextElement;
  }
}

static void Post()
{
  SerialUSB.println("### Post.");

  int status = 0;
  if (!Wio.HttpPost(IFTTT_MAKER_WEBHOOKS_URL, "{\"value1\":\"" POST "\"}", &status))
  {
    SerialUSB.println("### ERROR 11 ###");
    return;
  }

  SerialUSB.print("### Post status:");
  SerialUSB.println(status);
}

////////////////////////////////////////////////////////////

void loop()
{
}

