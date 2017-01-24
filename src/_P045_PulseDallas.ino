#include <OneWire.h>
#include <DS2423.h>

//#######################################################################################################
//#################################### Plugin 045: Counter Dallas DS2423  ###############################
//#######################################################################################################

#define PLUGIN_045
#define PLUGIN_ID_045         045
#define PLUGIN_NAME_045       "Pulse Counter - DS2423"
#define PLUGIN_VALUENAME1_045 "CountDelta"


boolean Plugin_045(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_045;
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
        string = F(PLUGIN_NAME_045);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_045));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        uint8_t addr[8];

        ExtraTaskSettings.TaskDeviceValueDecimals[0] = 0;
        // Scan the onewire bus and fill dropdown list with devicecount on this GPIO.
        byte plugin_045_DallasPin = Settings.TaskDevicePin1[event->TaskIndex];

        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        byte counter = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        byte devCount = Plugin_045_DS_scan(plugin_045_DallasPin, choice, addr);
        string += F("<TR><TD>Device Nr:<TD><select name='plugin_045_dev'>");
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
        string += F("<TR><TD>Counter:<TD><select name='plugin_045_cnt'>");
        string += F("<option value='0'");
        if (counter == 0)
          string += F(" selected");
        string += F(">A<option value='1'");
        if (counter == 1)
          string += F(" selected");
        string += F(">B</option></select> Counter value is incremental");

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        uint8_t addr[8];
        String deviceIndex = WebServer.arg(F("plugin_045_dev"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = deviceIndex.toInt();
        String counter = WebServer.arg(F("plugin_045_cnt"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = counter.toInt();

        // find the address for selected device and store into extra tasksettings
        byte devCount = Plugin_045_DS_scan(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePluginConfig[event->TaskIndex][0], addr);
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

    case PLUGIN_INIT:
      {
        UserVar[event->BaseVarIndex] = 0;
        UserVar[event->BaseVarIndex + 1] = 0;
        UserVar[event->BaseVarIndex + 2] = 0;

        break;
      }

    case PLUGIN_READ:
      {
        uint8_t addr[8];
        // Load ROM address from tasksettings
        LoadTaskSettings(event->TaskIndex);
        for (byte x = 0; x < 8; x++)
          addr[x] = ExtraTaskSettings.TaskDevicePluginConfigLong[x];

        byte plugin_045_DallasPin = Settings.TaskDevicePin1[event->TaskIndex];
        byte counterIndex = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        float value = 0;
        String log = F("DS   : Counter ");
        log += counterIndex == 0 ? F("A") : F("B");
        log += F(": ");
        if (Plugin_045_DS_readCounter(plugin_045_DallasPin, addr, &value, counterIndex))
        {
          UserVar[event->BaseVarIndex] = UserVar[event->BaseVarIndex + 2] != 0
            ? value - UserVar[event->BaseVarIndex + 1]
            : 0;
          UserVar[event->BaseVarIndex + 2] = 1;
          UserVar[event->BaseVarIndex + 1] = value;
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
byte Plugin_045_DS_scan(byte plugin_045_DallasPin, byte choice, uint8_t* ROM)
{
  byte tmpaddr[8];
  byte devCount = 0;

  OneWire ds(plugin_045_DallasPin);

  while (ds.search(tmpaddr))
  {
    uint8_t model = tmpaddr[0];
    if (!(model == 0x1D))
      continue;

    if (choice == devCount)
      memcpy(ROM, tmpaddr, sizeof(tmpaddr));

    devCount++;
  }

  return devCount;
}


/*********************************************************************************************\
   Dallas Read Counter
  \*********************************************************************************************/
boolean Plugin_045_DS_readCounter(byte plugin_045_DallasPin, uint8_t ROM[8], float *value, byte counterIndex)
{
  OneWire ds(plugin_045_DallasPin);
  DS2423 ds2423(&ds, ROM);
  ds2423.begin(DS2423_COUNTER_A|DS2423_COUNTER_B);

  ds2423.update();
  if (ds2423.isError())
    return false;

  uint32_t count = counterIndex == 0
    ? ds2423.getCount(DS2423_COUNTER_A)
    : ds2423.getCount(DS2423_COUNTER_B);

  *value = count;
  return true;
}
