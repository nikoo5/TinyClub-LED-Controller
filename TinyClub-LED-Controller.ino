#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

#define LEDS_DATA_PIN 13
#define ONOFF_BUTTON_PIN 12
#define FX_BUTTON_PIN 14
#define AP_BUTTON_PIN 27
#define BILTIN_LED_PIN 2
#define BUTTON_DEBOUNCE_MS 40

// WIFI configuration
const char *AP_SSID = "TinyClub-LED-Controller";
const char *AP_PASSWORD = "12345678";
const char *SSID = "";
const char *PASSWORD = "";
bool apEnabled = false;

// Web server configuration
WebServer server(80);
Preferences prefs;

// LEDs configuration
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, LEDS_DATA_PIN, NEO_GRB + NEO_KHZ800);
struct Config
{
  int ledCount;
  int baseR;
  int baseG;
  int baseB;
  int effectR;
  int effectG;
  int effectB;
  int effectLength;
  int effectCount;
  int effectSpeed;
  bool reverse;
};
Config cfg;
uint32_t baseColor = 0;
uint16_t effectPointer = 0;
uint8_t currentEffect = 0;
bool ledsState = true;

// Button state tracking configuration
int lastAPButtonReading = HIGH;
int stableAPButtonState = HIGH;
int lastFXButtonReading = HIGH;
int stableFXButtonState = HIGH;
int lastOnOffButtonReading = HIGH;
int stableOnOffButtonState = HIGH;
unsigned long lastDebounceTime = 0;

// Support functions
int clampInt(int value, int minValue, int maxValue)
{
  if (value < minValue)
    return minValue;
  if (value > maxValue)
    return maxValue;
  return value;
}
String configAsJson()
{
  String json = "{";
  json += "\"ledCount\":" + String(cfg.ledCount) + ",";
  json += "\"baseColor\":{";
  json += "\"r\":" + String(cfg.baseR) + ",";
  json += "\"g\":" + String(cfg.baseG) + ",";
  json += "\"b\":" + String(cfg.baseB) + "},";
  json += "\"effectColor\":{";
  json += "\"r\":" + String(cfg.effectR) + ",";
  json += "\"g\":" + String(cfg.effectG) + ",";
  json += "\"b\":" + String(cfg.effectB) + "},";
  json += "\"effectLength\":" + String(cfg.effectLength) + ",";
  json += "\"effectCount\":" + String(cfg.effectCount) + ",";
  json += "\"effectSpeed\":" + String(1001 - cfg.effectSpeed) + ",";
  json += "\"direction\":\"" + String(cfg.reverse ? "reverse" : "forward") + "\"";
  json += "}";
  return json;
}

// LEDs functions
void ledsClear()
{
  pixels.clear();
  pixels.show();
}
void ledsDrawBackground()
{
  baseColor = pixels.Color(cfg.baseR, cfg.baseG, cfg.baseB);

  for (int i = 0; i < cfg.ledCount; i++)
  {
    pixels.setPixelColor(i, baseColor);
  }

  pixels.show();
}
void ledsDrawEffect0()
{
  if (currentEffect != 0)
  {
    return;
  }

  uint16_t effectSpace = cfg.ledCount / cfg.effectCount;
  uint16_t pointer = effectPointer;

  for (uint8_t i = 0; i < cfg.effectCount; i++)
  {
    uint16_t start = (i * effectSpace + pointer) % cfg.ledCount;

    if (!cfg.reverse)
    {
      start = start == 0 ? cfg.ledCount - 1 : start - 1;
    }

    uint8_t stepR = cfg.effectR / (cfg.effectLength - 1);
    uint8_t stepG = cfg.effectG / (cfg.effectLength - 1);
    uint8_t stepB = cfg.effectB / (cfg.effectLength - 1);

    for (uint8_t j = 0; j <= cfg.effectLength; j++)
    {
      int ledIndex = (start + j) % cfg.ledCount;

      if ((!cfg.reverse && j == 0) || (cfg.reverse && j == cfg.effectLength))
      {
        pixels.setPixelColor(ledIndex, pixels.Color(cfg.baseR, cfg.baseG, cfg.baseB));
      }
      else
      {
        uint8_t R = !cfg.reverse ? (stepR * (j - 1)) : (stepR * (cfg.effectLength - (j + 1)));
        uint8_t G = !cfg.reverse ? (stepG * (j - 1)) : (stepG * (cfg.effectLength - (j + 1)));
        uint8_t B = !cfg.reverse ? (stepB * (j - 1)) : (stepB * (cfg.effectLength - (j + 1)));
        pixels.setPixelColor(ledIndex, pixels.Color(R, G, B));
      }
    }
  }

  effectPointer = cfg.reverse ? (effectPointer == 0 ? cfg.ledCount - 1 : effectPointer - 1) : (effectPointer + 1) % cfg.ledCount;
  pixels.show();
}
void ledsDrawEffect1()
{
  if (currentEffect != 1)
  {
    return;
  }

  // Implement other effects here
}
void ledsDrawEffect2()
{
  if (currentEffect != 2)
  {
    return;
  }

  // Implement other effects here
}
void ledsSetup()
{
  pixels.updateLength(cfg.ledCount);
  pixels.begin();
  ledsDrawBackground();
}
void ledsLoop()
{
  if (ledsState)
  {
    ledsDrawEffect0();
    ledsDrawEffect1();
    ledsDrawEffect2();
    delay(cfg.effectSpeed);
  }
}

// Prefs functions
void prefsLoadConfig()
{
  cfg.ledCount = prefs.getInt("ledCount", 60);
  cfg.baseR = prefs.getInt("baseR", 128);
  cfg.baseG = prefs.getInt("baseG", 128);
  cfg.baseB = prefs.getInt("baseB", 128);
  cfg.effectR = prefs.getInt("effectR", 255);
  cfg.effectG = prefs.getInt("effectG", 64);
  cfg.effectB = prefs.getInt("effectB", 64);
  cfg.effectLength = prefs.getInt("effLen", 10);
  cfg.effectCount = prefs.getInt("effCnt", 3);
  cfg.effectSpeed = prefs.getInt("effSpd", 100);
  cfg.reverse = prefs.getBool("reverse", false);

  cfg.ledCount = clampInt(cfg.ledCount, 1, 2000);
  cfg.baseR = clampInt(cfg.baseR, 0, 255);
  cfg.baseG = clampInt(cfg.baseG, 0, 255);
  cfg.baseB = clampInt(cfg.baseB, 0, 255);
  cfg.effectR = clampInt(cfg.effectR, 0, 255);
  cfg.effectG = clampInt(cfg.effectG, 0, 255);
  cfg.effectB = clampInt(cfg.effectB, 0, 255);
  cfg.effectLength = clampInt(cfg.effectLength, 1, 100);
  cfg.effectCount = clampInt(cfg.effectCount, 1, 20);
  cfg.effectSpeed = clampInt(cfg.effectSpeed, 1, 1000);
}
void prefsSaveConfig()
{
  prefs.putInt("ledCount", cfg.ledCount);
  prefs.putInt("baseR", cfg.baseR);
  prefs.putInt("baseG", cfg.baseG);
  prefs.putInt("baseB", cfg.baseB);
  prefs.putInt("effectR", cfg.effectR);
  prefs.putInt("effectG", cfg.effectG);
  prefs.putInt("effectB", cfg.effectB);
  prefs.putInt("effLen", cfg.effectLength);
  prefs.putInt("effCnt", cfg.effectCount);
  prefs.putInt("effSpd", cfg.effectSpeed);
  prefs.putBool("reverse", cfg.reverse);
}
void prefsSetDefaults()
{
  cfg.ledCount = 600;
  cfg.baseR = 128;
  cfg.baseG = 128;
  cfg.baseB = 128;
  cfg.effectR = 0;
  cfg.effectG = 0;
  cfg.effectB = 128;
  cfg.effectLength = 10;
  cfg.effectCount = 3;
  cfg.effectSpeed = 1000;
  cfg.reverse = false;
}
void prefsSetup()
{
  prefs.begin("ledcfg", false);
  prefsLoadConfig();
}

// WiFi functions
void wifiSetAP(bool enabled)
{
  if (enabled == apEnabled)
  {
    return;
  }

  if (SSID[0] != '\0' && PASSWORD[0] != '\0')
  {
    return;
  }

  if (enabled)
  {
    WiFi.mode(WIFI_AP);
    bool started = WiFi.softAP(AP_SSID, AP_PASSWORD);
    if (started)
    {
      IPAddress ip = WiFi.softAPIP();
      Serial.print("AP enabled. IP: ");
      Serial.println(ip);
      apEnabled = true;
      digitalWrite(BILTIN_LED_PIN, HIGH);
    }
    else
    {
      Serial.println("Failed to enable AP");
    }
    return;
  }

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  apEnabled = false;
  Serial.println("AP disabled");
  digitalWrite(BILTIN_LED_PIN, LOW);
}
void wifiInit()
{
  if (SSID[0] == '\0' || PASSWORD[0] == '\0')
  {
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
    if (!apStarted)
    {
      Serial.println("Failed to start AP");
      apEnabled = false;
    }
    else
    {
      apEnabled = true;
      IPAddress ip = WiFi.softAPIP();
      Serial.print("AP enabled. IP: ");
      Serial.println(ip);
    }
  }
  else
  {
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(BILTIN_LED_PIN, HIGH);
  }
}

// Web server functions
bool serverReadArgInt(const char *argName, int minValue, int maxValue, int &target)
{
  if (!server.hasArg(argName))
  {
    return false;
  }
  int value = server.arg(argName).toInt();
  target = clampInt(value, minValue, maxValue);
  return true;
}
void serverHandleRoot_GET()
{
  if (!LittleFS.exists("/index.html"))
  {
    server.send(500, "text/plain", "index.html not found in LittleFS");
    return;
  }

  File file = LittleFS.open("/index.html", "r");
  if (!file)
  {
    server.send(500, "text/plain", "Failed to open index.html");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}
void serverHandleConfig_GET()
{
  server.send(200, "application/json", configAsJson());
}
void serverHandleSave_POST()
{
  int oldLedCount = cfg.ledCount;

  if (!serverReadArgInt("ledCount", 1, 2000, cfg.ledCount))
  {
    server.send(400, "text/plain", "Missing ledCount");
    return;
  }

  if (!serverReadArgInt("baseR", 0, 255, cfg.baseR) ||
      !serverReadArgInt("baseG", 0, 255, cfg.baseG) ||
      !serverReadArgInt("baseB", 0, 255, cfg.baseB) ||
      !serverReadArgInt("effectR", 0, 255, cfg.effectR) ||
      !serverReadArgInt("effectG", 0, 255, cfg.effectG) ||
      !serverReadArgInt("effectB", 0, 255, cfg.effectB) ||
      !serverReadArgInt("effectLength", 1, 100, cfg.effectLength) ||
      !serverReadArgInt("effectCount", 1, 20, cfg.effectCount) ||
      !serverReadArgInt("effectSpeed", 1, 1000, cfg.effectSpeed))
  {
    server.send(400, "text/plain", "Missing or invalid fields");
    return;
  }

  cfg.effectSpeed = 1001 - cfg.effectSpeed;

  String direction = server.hasArg("direction") ? server.arg("direction") : "forward";
  direction.toLowerCase();
  cfg.reverse = (direction == "reverse");

  prefsSaveConfig();
  server.send(200, "application/json", "{\"ok\":true}");

  if (oldLedCount != cfg.ledCount)
    ledsSetup();
  ledsDrawBackground();
}
void serverHandleNotFound()
{
  server.send(404, "text/plain", "Not found");
}
void serverSetup()
{
  server.on("/", HTTP_GET, serverHandleRoot_GET);
  server.on("/config", HTTP_GET, serverHandleConfig_GET);
  server.on("/save", HTTP_POST, serverHandleSave_POST);
  server.onNotFound(serverHandleNotFound);
  server.begin();
  Serial.println("Web server started");
}

// Buttons functions
void buttonCheck(uint8_t pin, int &lastReading, int &stableState, void (*onPress)())
{
  int reading = digitalRead(pin);

  if (reading != lastReading)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_MS && reading != stableState)
  {
    stableState = reading;

    // Active-low button: trigger action when the button is pressed.
    if (stableState == LOW)
    {
      onPress();
    }
  }

  lastReading = reading;
}
void buttonCheckAll()
{
  buttonCheck(AP_BUTTON_PIN, lastAPButtonReading, stableAPButtonState, []()
              { wifiSetAP(!apEnabled); });

  buttonCheck(FX_BUTTON_PIN, lastFXButtonReading, stableFXButtonState, []()
              { currentEffect = (currentEffect + 1) % 3; });

  buttonCheck(ONOFF_BUTTON_PIN, lastOnOffButtonReading, stableOnOffButtonState, []()
              {
                ledsState = !ledsState;
                ledsState ? ledsDrawBackground() : ledsClear(); });
}

// Hardware functions
void hardwareSetup()
{
  pinMode(BILTIN_LED_PIN, OUTPUT);
  pinMode(AP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(FX_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ONOFF_BUTTON_PIN, INPUT_PULLUP);
}

// FS functions
void fsSetup()
{
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS mount failed");
  }
}

// Main functions
void setup()
{
  Serial.begin(115200);

  hardwareSetup();
  prefsSetup();
  ledsSetup();
  fsSetup();
  wifiInit();
  serverSetup();

  wifiSetAP(false);

  // prefsSetDefaults(); //Uncomment this line to reset saved config to defaults on next boot
}
void loop()
{
  ledsLoop();

  buttonCheckAll();

  server.handleClient();
}
