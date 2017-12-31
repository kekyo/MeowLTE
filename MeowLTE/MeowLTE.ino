#include <WioLTEforArduino.h>

#include "AudioOutput.h"
#include "AudioPlay.h"
#include "PostMessage.h"

////////////////////////////////////////////////////////////

WioLTE Wio;

////////////////////////////////////////////////////////////

static volatile int count = 1;

static void OnClicked()
{
  count++;

  digitalWrite(WIOLTE_A5, 1);

  PlayMeow();
}

////////////////////////////////////////////////////////////

void setup()
{
  SerialUSB.println("============== Start MeowLTE");

  pinMode(WIOLTE_A5, OUTPUT);
  digitalWrite(WIOLTE_A5, 0);

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

  count = 1;
  attachInterrupt(WIOLTE_D39, OnClicked, RISING);

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
