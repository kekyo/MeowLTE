#include <config.h>
#include <WioLTEforArduino.h>
#include <SD.h>		// https://github.com/Seeed-Studio/SD

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

#define FILE_NAME "meow.wav"

File myFile;
int offset = -1;
int size = -1;

RiffHeader header;
FormatChunk format;
uint8_t buffer[256];

void setup()
{
  WioLTEDac::Init(WioLTEDac::DAC1);

  delay(200);

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
    && (format.samplerate != 11025)
    && (format.bitswidth != 16))
  {
    myFile.close();
    SerialUSB.println("### ERROR 5 ###");
    return;
  }

  int ext = format.size + sizeof(format.id) + sizeof(format.size) - sizeof format;
  if ((ext < 0) || (ext > sizeof buffer))
  {
    myFile.close();
    SerialUSB.println("### ERROR 6 ###");
    return;
  }
  
  if (myFile.read(buffer, ext) != ext)
  {
    myFile.close();
    SerialUSB.println("### ERROR 7 ###");
    return;
  }

  DataChunk data;
  if (myFile.read(&data, sizeof data) != sizeof data)
  {
    myFile.close();
    SerialUSB.println("### ERROR 8 ###");
    return;
  }

  if (data.id != 0x61746164)
  {
    myFile.close();
    SerialUSB.println("### ERROR 9 ###");
    return;
  }

  offset = sizeof header + sizeof format + ext + sizeof data;
  size = data.size;

  SerialUSB.println(FILE_NAME":");
}

void loop()
{
  if (offset == -1)
  {
    return;
  }

  int position = 0;
  while (position < size)
  {
    int read = myFile.read(buffer, sizeof buffer);
    if (read <= 0)
    {
      break;
    }

    const int16_t* p = (const int16_t*)&buffer[0];
    for (int index = 0; index < (read / sizeof *p); index++)
    {
      uint16_t v = (uint16_t)(*p + 32767);
      WioLTEDac::Write(WioLTEDac::DAC1, v >> 4);
      p++;
      delayMicroseconds(100);
    }

    position += read;
  }

  myFile.seek(offset);
}

