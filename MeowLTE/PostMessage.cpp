#include "PostMessage.h"

#include <stdio.h>
#include <assert.h>

#include "MeowLTEConfig.h"

#define IFTTT_MAKER_WEBHOOKS_URL "https://maker.ifttt.com/trigger/" IFTTT_MAKER_WEBHOOKS_EVENT "/with/key/" IFTTT_MAKER_WEBHOOKS_KEY

extern WioLTE Wio;

bool InitializeLteModem()
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

void Post(const int count)
{
  assert(count >= 1);

  SerialUSB.println("### Post.");

  char body[100];
  if (count == 1)
  {
    strcpy(body, "{\"value1\":\"" POST "\"}");
  }
  else
  {
    sprintf(body, "{\"value1\":\"%d" POST "\"}", count);
  }

  int status = 0;
  if (!Wio.HttpPost(IFTTT_MAKER_WEBHOOKS_URL, body, &status))
  {
    SerialUSB.println("### ERROR 11 ###");
    return;
  }

  SerialUSB.print("### Post status:");
  SerialUSB.println(status);
}
