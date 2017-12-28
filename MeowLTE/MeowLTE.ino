#include <config.h>
#include <WioLTEforArduino.h>
#include <SD.h>		// https://github.com/Seeed-Studio/SD

////////////////////////////////////////////////////////////

#define FILE_NAME "meow.wav"
#define BUFFER_LENGTH 128
#define BUFFER_ELEMENTS 3

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

#define BUFFER_SIZE (BUFFER_LENGTH * sizeof(int16_t))

WioLTE Wio;
HardwareTimer timer(2);
File myFile;

int offset = -1;
int size = -1;
int position = 0;

RiffHeader header;
FormatChunk format;

static void timer_handler();

////////////////////////////////////////////////////////////

void setup()
{
  Wio.Init();

  Wio.PowerSupplyGrove(true);
  
  WioLTEDac::Init((WioLTEDac::DACChannel)(WioLTEDac::DAC1 | WioLTEDac::DAC2));
  WioLTEDac::Write(WioLTEDac::DAC1, ((uint16_t)32768) >> 4);
  WioLTEDac::Write(WioLTEDac::DAC2, ((uint16_t)65535) >> 4);

  delay(500);

  SerialUSB.println("");
  SerialUSB.println("--- START ---------------------------------------------------");

  SerialUSB.println("### Initialize SD card.");
  if (!SD.begin())
  {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("### Reading from "FILE_NAME".");

  myFile = SD.open(FILE_NAME);
  if (!myFile)
  {
    SerialUSB.println("### ERROR 1 ###");
    return;
  }

  if (myFile.read(&header, sizeof header) != sizeof header)
  {
    myFile.close();
    SerialUSB.println("### ERROR 2 ###");
    return;
  }

  if ((header.riff != 0x46464952) || (header.type != 0x45564157))
  {
    myFile.close();
    SerialUSB.println("### ERROR 3 ###");
    return;
  }
  
  if (myFile.read(&format, sizeof format) != sizeof format)
  {
    myFile.close();
    SerialUSB.println("### ERROR 4 ###");
    return;
  }

  // DIRTY HACK: fmt chunk.
  if ((format.id != 0x20746d66)
    && (format.format != 0x0001)  // PCM
    && (format.channels != 0x0001) // Mono
    && (format.bitswidth != 16))
  {
    myFile.close();
    SerialUSB.println("### ERROR 5 ###");
    return;
  }

  int ext = format.size + sizeof(format.id) + sizeof(format.size) - sizeof format;
  if (ext != 0)
  {
    myFile.close();
    SerialUSB.println("### ERROR 6 ###");
    return;
  }

  DataChunk data;
  if (myFile.read(&data, sizeof data) != sizeof data)
  {
    myFile.close();
    SerialUSB.println("### ERROR 7 ###");
    return;
  }

  if (data.id != 0x61746164)
  {
    myFile.close();
    SerialUSB.println("### ERROR 8 ###");
    return;
  }

  offset = sizeof header + sizeof format + sizeof data;
  size = data.size;

  SerialUSB.println(FILE_NAME":");

  ////////////////////////////////////////////////
  // Timer

  uint32_t period = (uint32_t)((10000000 / format.samplerate + 5) / 20);
  timer.setPeriod(period);

  timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  timer.setCompare(TIMER_CH1, 1);
  timer.attachCompare1Interrupt(timer_handler); 

  timer.refresh();
  timer.resume();
}

////////////////////////////////////////////////////////////

uint8_t readElement = 0;
uint8_t writeElement = 0;
uint16_t writePosition = 0;

int16_t buffer[BUFFER_ELEMENTS][BUFFER_LENGTH];
uint16_t length[BUFFER_ELEMENTS];

////////////////////////////////////////////////////////////

uint16_t a = 0;

static void timer_handler()
{
  /*
  WioLTEDac::Write(WioLTEDac::DAC1, a);
  a = a ? 0 : 0xfff;
*/
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

////////////////////////////////////////////////////////////

void loop()
{
  if (offset == -1)
  {
    return;
  }

  while (1)
  {
    uint8_t nextElement = readElement + 1;
    if (nextElement >= BUFFER_ELEMENTS)
    {
      nextElement = 0;
    }
  
    if (nextElement == writeElement)
    {
      return;
    }
  
    if (position >= size)
    {
      //WioLTEDac::Write(WioLTEDac::DAC1, ((uint16_t)32768) >> 4);
      myFile.seek(offset);
      position = 0;
    }
        
    int req = size - position;
    req = (req > BUFFER_SIZE) ? BUFFER_SIZE : req;
    
    int read = myFile.read(&buffer[readElement][0], req);
    length[readElement] = req / sizeof(uint16_t);
    position += req;
    
    readElement = nextElement;
  }
}
