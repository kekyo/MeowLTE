#include <WioLTEforArduino.h>

bool InitializeDac();
bool InitializeTimer();

class Pcm16Reader
{
public:
  virtual uint32_t SampleRate() const = 0;
  virtual uint32_t Size() const = 0;
  virtual int ReadFragment(int16_t* pBuffer, const uint16_t size) = 0;
};

void OutputPcm16Audio(Pcm16Reader& reader);
