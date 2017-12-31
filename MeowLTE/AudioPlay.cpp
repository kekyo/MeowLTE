#include "AudioPlay.h"

#include "AudioOutput.h"
#include "MeowLTEConfig.h"

#include <assert.h>
#include <SD.h> // https://github.com/Seeed-Studio/SD

struct RiffHeader
{
  uint32_t riff;
  int32_t size;
  uint32_t type;
};

struct FormatChunk
{
  uint32_t id;
  int32_t size;
  int16_t format;
  uint16_t channels;
  uint32_t samplerate;
  uint32_t bytepersec;
  uint16_t blockalign;
  uint16_t bitswidth;
};

struct FormatChunkEx
{
  FormatChunk formatChunk;
  uint16_t extended_size;
  uint8_t extended[];
};

struct DataChunk
{
  uint32_t id;
  int32_t size;
  //uint8_t waveformData [];
};

class WaveReader : public Pcm16Reader
{
  private:
    File file;
    FormatChunk format;
    int offset;
    int size;

  public:
    WaveReader()
      : offset(-1), size(-1)
    {
      memset(&format, 0, sizeof format);
    }

    virtual ~WaveReader()
    {
      file.close();
    }

    bool Open(const char* pFileName)
    {
      file = SD.open(pFileName);

      RiffHeader header;
      if (file.read(&header, sizeof header) != sizeof header)
      {
        file.close();
        return false;
      }

      if ((header.riff != 0x46464952) || (header.type != 0x45564157))
      {
        file.close();
        return false;
      }

      if (file.read(&format, sizeof format) != sizeof format)
      {
        file.close();
        return false;
      }

      // DIRTY HACK: fmt chunk.
      if ((format.id != 0x20746d66)
        && (format.format != 0x0001)    // PCM
        && (format.channels != 0x0001)  // Mono
        && (format.bitswidth != 16))    // 16bits
      {
        file.close();
        return false;
      }

      int ext = format.size + sizeof(format.id) + sizeof(format.size) - sizeof format;
      if (ext != 0)
      {
        file.close();
        return false;
      }

      DataChunk data;
      if (file.read(&data, sizeof data) != sizeof data)
      {
        file.close();
        return false;
      }

      if (data.id != 0x61746164)
      {
        file.close();
        return false;
      }

      offset = sizeof header + sizeof format + sizeof data;
      size = data.size;
    }

    void Rewind()
    {
      file.seek(offset);
    }

    virtual uint32_t Size() const
    {
      return size;
    }

    virtual uint32_t SampleRate() const
    {
      return format.samplerate;
    }

    virtual int ReadFragment(int16_t* pBuffer, const uint16_t size)
    {
      assert(offset >= 0);
      return file.read(pBuffer, size);
    }
};

// Initialize SD card and wave file
bool InitializeSDCard()
{
  SerialUSB.println("### Initialize SD card.");

  if (!SD.begin())
  {
    SerialUSB.println("### ERROR 0 ###");
    return false;
  }

  return true;
}

void PlayMeow()
{
  WaveReader reader;

  reader.Open(MEOW_FILENAME);
  OutputPcm16Audio(reader);
}
