#include <WioLTEforArduino.h>

#include "AudioOutput.h"
#include "AudioPlay.h"
#include "PostMessage.h"

////////////////////////////////////////////////////////////

WioLTE Wio;

////////////////////////////////////////////////////////////

static volatile int count = 0;

static void OnClicked()
{
  count++;

  PlayMeow();
}

////////////////////////////////////////////////////////////

void setup()
{
  SerialUSB.println("============== Start MeowLTE");

  Wio.Init();

  Wio.PowerSupplyGrove(true);
  delay(200);

  ////////////////////////////////////////////////
  // Initialize Audio output.

  if (InitializeAudio() == false)
  {
    return;
  }

  ////////////////////////////////////////////////
  // SD card.

  if (InitializeSDCard() == false)
  {
    return;
  }

  ////////////////////////////////////////////////
  // Meow

  count = 0;
  attachInterrupt(WIOLTE_D38, OnClicked, RISING);

  PlayMeow();

  ////////////////////////////////////////////////
  // LTE modem

  Wio.PowerSupplyLTE(true);
  delay(1000);

  if (InitializeLTEModem() == false)
  {
    return;
  }
}

////////////////////////////////////////////////////////////

void loop()
{
  while (1)
  {
    const int currentCount = count;
    count = 0;
    if (currentCount == 0)
    {
      break;
    }

    Post(currentCount);
  }
}
