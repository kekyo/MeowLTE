#include <WioLTEforArduino.h>

#include "AudioOutput.h"
#include "AudioPlay.h"
#include "PostMessage.h"

#include "MeowLTEConfig.h"

////////////////////////////////////////////////////////////

WioLTE Wio;

////////////////////////////////////////////////////////////

static volatile int count = 1;

static void OnClicked()
{
  count++;

  digitalWrite(WIOLTE_A5, 1);

  PlayAudio(MEOW_FILENAME);
}

////////////////////////////////////////////////////////////

void setup()
{
  SerialUSB.println("============== Start MeowLTE");

  pinMode(WIOLTE_A5, OUTPUT); // LED
  pinMode(WIOLTE_D39, INPUT); // Button

  // LED OFF
  digitalWrite(WIOLTE_A5, 0);

  Wio.Init();

  ////////////////////////////////////////////////
  // Initialize Audio output.

  Wio.PowerSupplyGrove(true);
  delay(100);

  if (InitializeAudio() == false)
  {
    return;
  }

  ////////////////////////////////////////////////
  // SD card.

  Wio.PowerSupplySD(true);
  delay(100);

  if (InitializeSDCard() == false)
  {
    return;
  }

  ////////////////////////////////////////////////
  // Meow

  count = 1;
  attachInterrupt(WIOLTE_D39, OnClicked, RISING);

  PlayAudio(MEOW_FILENAME);

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
