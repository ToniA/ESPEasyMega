#include <OneWire.h>
#include <DallasTemperature.h>

//#######################################################################################################
//#################################### Plugin 004: TempSensor Dallas DS18B20  ###########################
//#######################################################################################################

#define PLUGIN_004
#define PLUGIN_ID_004         4
#define PLUGIN_NAME_004       "Temperature - DS18b20"
#define PLUGIN_VALUENAME1_004 "Temperature"

uint8_t Plugin_004_DallasPin;

boolean Plugin_004(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_004;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_004);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_004));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        uint8_t addr[8];

        // Scan the onewire bus and fill dropdown list with devicecount on this GPIO.
        Plugin_004_DallasPin = Settings.TaskDevicePin1[event->TaskIndex];

        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        byte devCount = Plugin_004_DS_scan(choice, addr);
        string += F("<TR><TD>Device Nr:<TD><select name='plugin_004_dev'>");
        for (byte x = 0; x < devCount; x++)
        {
          string += F("<option value='");
          string += x;
          string += "'";
          if (choice == x)
            string += F(" selected");
          string += ">";
          string += x + 1;
          string += F("</option>");
        }
        string += F("</select> ROM: ");
        if (devCount)
        {
          for (byte  i = 0; i < 8; i++)
          {
            string += (addr[i] < 0x10 ? "0" : "") + String(addr[i], HEX);
            if (i < 7) string += "-";
          }
        }
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        uint8_t addr[8];
        String plugin1 = WebServer.arg(F("plugin_004_dev"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();

        // find the address for selected device and store into extra tasksettings
        Plugin_004_DallasPin = Settings.TaskDevicePin1[event->TaskIndex];
        byte devCount = Plugin_004_DS_scan(Settings.TaskDevicePluginConfig[event->TaskIndex][0], addr);
        for (byte x = 0; x < 8; x++)
          ExtraTaskSettings.TaskDevicePluginConfigLong[x] = addr[x];
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SHOW_CONFIG:
      {
        for (byte i = 0; i < 8; i++)
        {
          string += (ExtraTaskSettings.TaskDevicePluginConfigLong[i] < 0x10 ? "0" : "")
                    + String(ExtraTaskSettings.TaskDevicePluginConfigLong[i], HEX);
          if (i < 7) string += "-";
        }
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        uint8_t addr[8];
        // Load ROM address from tasksettings
        LoadTaskSettings(event->TaskIndex);
        for (byte x = 0; x < 8; x++)
          addr[x] = ExtraTaskSettings.TaskDevicePluginConfigLong[x];

        Plugin_004_DallasPin = Settings.TaskDevicePin1[event->TaskIndex];
        float value = 0;
        String log = F("DS   : Temperature: ");
        if (Plugin_004_DS_readTemp(addr, &value))
        {
          UserVar[event->BaseVarIndex] = value;
          log += UserVar[event->BaseVarIndex];
          success = true;
        }
        else
        {
          UserVar[event->BaseVarIndex] = NAN;
          log += F("Error!");
        }
        log += (" (");
        for (byte i = 0; i < 8; i++)
        {
          log += (addr[i] < 0x10 ? "0" : "") + String(addr[i], HEX);
          if (i < 7) log += "-";
        }
        log += ")";
        addLog(LOG_LEVEL_INFO, log);
        break;
      }

  }
  return success;
}


/*********************************************************************************************\
   Dallas Scan bus
  \*********************************************************************************************/
byte Plugin_004_DS_scan(byte choice, uint8_t* ROM)
{
  byte tmpaddr[8];
  byte devCount = 0;

  OneWire ds(Plugin_004_DallasPin);

  while (ds.search(tmpaddr))
  {
    uint8_t model = tmpaddr[0];
    if (!(model == DS18S20MODEL || model == DS18B20MODEL || model == DS1822MODEL || model == DS1825MODEL))
      continue;

    if (choice == devCount)
      memcpy(ROM, tmpaddr, sizeof(tmpaddr));

    devCount++;
  }

  return devCount;
}


/*********************************************************************************************\
   Dallas Read temperature
  \*********************************************************************************************/
boolean Plugin_004_DS_readTemp(uint8_t ROM[8], float *value)
{
  OneWire ds(Plugin_004_DallasPin);
  DallasTemperature sensors(&ds);
  sensors.setWaitForConversion(true);

  sensors.requestTemperatures();
  float temperature = sensors.getTempC(ROM);

  if (temperature != DEVICE_DISCONNECTED_C)
  {
    *value = temperature;
    return true;
  }

  *value = 0;
  return false;
}
