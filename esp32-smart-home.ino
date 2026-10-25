/*
  =============================================================================
  ESP32 4-Channel Smart Home - v2
  =============================================================================
  New in v2:
  - Captive-portal Wi-Fi setup: no hard-coded credentials, no re-flashing to
    change networks. If the router is missing, the device falls back to its own
    hotspot and any browser is redirected to the setup page.
  - Open "SmartHome-Setup" hotspot until Wi-Fi is configured; after that the
    fallback hotspot is password protected.
  - Proper app icons/manifest (Add to Home Screen) and a fixed local URL:
    http://smarthome.local (mDNS) plus DHCP hostname "smarthome".
  - Optional static IP from the setup page.
  - Arduino Cloud (free) -> Google Home -> Gemini/Assistant voice control.
    See cloud_config.h and README.md. Local control works without internet.
  - Non-blocking time reads, safe boot with closed wall switches, separate
    manual timers/inching, correct midnight accounting, JSON escaping,
    fixed icon rendering and safer web OTA.

  Pin Assignments (ESP32 Dev Module):
  - Relay 1 (Module 1, IN1) : GPIO 23
  - Relay 2 (Module 1, IN2) : GPIO 22
  - Relay 3 (Module 2, IN1) : GPIO 21
  - Relay 4 (Module 2, IN2) : GPIO 19
  - Switch 1 (to GND)       : GPIO 13
  - Switch 2 (to GND)       : GPIO 12
  - Switch 3 (to GND)       : GPIO 14
  - Switch 4 (to GND)       : GPIO 27
  - Status LED              : GPIO 2
  =============================================================================
*/

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <time.h>

#include "cloud_config.h"

#if ENABLE_ARDUINO_CLOUD
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#endif

#include "web_assets.h"
#include "wifi_setup.h"
#include "app_icons.h"

// ==========================================
// 1. CONFIGURATION
// ==========================================
#define FW_VERSION "2.0.0"

#define NUM_RELAYS 4
const int relayPins[NUM_RELAYS]  = {23, 22, 21, 19};
const int switchPins[NUM_RELAYS] = {13, 12, 14, 27};
#define STATUS_LED 2

#define RELAY_ACTIVE_LOW true
// Set true for momentary push-buttons (toggles on the press edge only).
// Latching wall switches should stay false.
#define SWITCH_MOMENTARY false

const char* mdnsName = "smarthome";
const char* otaHostname = "esp32-smarthome";
const char* apSetupSsid = "SmartHome-Setup";
const char* apFallbackSsid = "SmartHome-Fallback";
const char* ntpServer = "pool.ntp.org";
const char* defaultApPassword = "smarthome1234";

const long defaultGmtOffset = 19800;   // UTC+5:30 (India)
const int daylightOffset = 0;

const unsigned long WIFI_BOOT_TIMEOUT_MS = 10000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 30000;
const unsigned long STA_LOST_GRACE_MS = 30000;
const unsigned long AP_STOP_GRACE_MS = 60000;
const unsigned long DEBOUNCE_DELAY_MS = 50;

IPAddress apIP(192, 168, 4, 1);

// ==========================================
// 2. GLOBAL STATE
// ==========================================
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

enum NetMode { NET_STA, NET_AP_SETUP, NET_AP_FALLBACK };
NetMode netMode = NET_STA;

bool apRunning = false;
bool apSetupMode = false;
bool staWasConnected = false;
bool mdnsRunning = false;
bool mdnsTried = false;
bool wifiProvisioned = false;
bool cloudConfigured = false;
bool cloudStarted = false;
bool cloudConnected = false;

String wifiSsid;
String wifiPass;
String apPassword;
bool useStaticIP = false;
String staticIpStr;
String gatewayStr;
String subnetStr;
String dnsStr;

float currentTz = 5.5;
unsigned long lastStaAttempt = 0;
unsigned long staDownSince = 0;
unsigned long staUpAt = 0;
unsigned long lastMdnsAttempt = 0;
int lastDisconnectReason = 0;

// ==========================================
// 3. DATA STRUCTURES
// ==========================================
struct ScheduleRule {
  bool enabled;
  int onHour;
  int onMin;
  int offHour;
  int offMin;
};

#define MAX_LOG_EVENTS 20
struct LogEvent {
  char timeStr[9];
  int relayIndex;
  char action[12];
  char trigger[16];
  unsigned long durationSecs;
};

LogEvent eventLogs[MAX_LOG_EVENTS];
int logCount = 0;
int logHead  = 0;

// Relay states and user configuration
bool relayStates[NUM_RELAYS] = {false, false, false, false};
String relayNames[NUM_RELAYS];
String relayIcons[NUM_RELAYS];

// Runtime analytics
unsigned long totalRuntimeSecs[NUM_RELAYS] = {0, 0, 0, 0};
unsigned long todayRuntimeSecs[NUM_RELAYS] = {0, 0, 0, 0};
unsigned int  switchCount[NUM_RELAYS] = {0, 0, 0, 0};
unsigned long sessionStartMillis[NUM_RELAYS] = {0, 0, 0, 0};
uint32_t todayDate = 0;
int lastCheckedMinute = -1;

// Timers. countdownSeconds is the live badge; timerMinutes is the
// configured sleep timer shown in the edit dialog.
unsigned long countdownSeconds[NUM_RELAYS] = {0, 0, 0, 0};
bool countdownIsManual[NUM_RELAYS] = {false, false, false, false};
int timerMinutes[NUM_RELAYS] = {0, 0, 0, 0};
int inchingSeconds[NUM_RELAYS] = {0, 0, 0, 0};

// Power-on behaviour: 0 = off, 1 = on, 2 = restore last state
int powerOnMode[NUM_RELAYS] = {2, 2, 2, 2};
ScheduleRule schedules[NUM_RELAYS];

// Physical switches
int lastSwitchState[NUM_RELAYS];
int confirmedSwitchState[NUM_RELAYS];
unsigned long lastDebounceTime[NUM_RELAYS] = {0, 0, 0, 0};

// Periodic work
unsigned long lastTimerTick = 0;
unsigned long lastHistorySave = 0;

// Schedule edge tracking
uint32_t lastSchedOnDate[NUM_RELAYS] = {0, 0, 0, 0};
uint32_t lastSchedOffDate[NUM_RELAYS] = {0, 0, 0, 0};

// ==========================================
// 4. FORWARD DECLARATIONS
// ==========================================
void applyRelayState(int index, bool state, const char* trigger = "Web UI");
bool readLocalTime(struct tm& out);
String jsonEscape(const String& in);
String htmlEscape(const String& in);
bool sameOrigin();
bool requireControl();

#if ENABLE_ARDUINO_CLOUD
CloudSwitch relay1, relay2, relay3, relay4;

class LocalWiFiConnectionHandler : public ConnectionHandler {
  public:
    LocalWiFiConnectionHandler() : ConnectionHandler(true, NetworkAdapter::WIFI) {}

    Client& getClient() override { return client_; }
    UDP& getUDP() override { return udp_; }
    unsigned long getTime() override {
      time_t now = time(nullptr);
      return (now > 1600000000UL) ? (unsigned long)now : 0;
    }
    int ping(IPAddress, uint8_t ttl = 128, uint8_t count = 1) override { (void)ttl; (void)count; return 0; }
    int ping(const String&, uint8_t ttl = 128, uint8_t count = 1) override { (void)ttl; (void)count; return 0; }
    int ping(const char*, uint8_t ttl = 128, uint8_t count = 1) override { (void)ttl; (void)count; return 0; }

  protected:
    NetworkConnectionState update_handleInit() override {
      return (WiFi.status() == WL_CONNECTED) ? NetworkConnectionState::CONNECTING
                                             : NetworkConnectionState::INIT;
    }
    NetworkConnectionState update_handleConnecting() override {
      return (WiFi.status() == WL_CONNECTED) ? NetworkConnectionState::CONNECTED
                                             : NetworkConnectionState::INIT;
    }
    NetworkConnectionState update_handleConnected() override {
      return (WiFi.status() == WL_CONNECTED) ? NetworkConnectionState::CONNECTED
                                             : NetworkConnectionState::DISCONNECTED;
    }
    NetworkConnectionState update_handleDisconnecting() override {
      return NetworkConnectionState::DISCONNECTED;
    }
    NetworkConnectionState update_handleDisconnected() override {
      return NetworkConnectionState::INIT;
    }

  private:
    WiFiClient client_;
    WiFiUDP udp_;
};

LocalWiFiConnectionHandler cloudConnection;
#endif

// ==========================================
// 5. TIME HELPERS (NON-BLOCKING)
// ==========================================
bool timeIsValid() {
  return time(nullptr) > 1600000000UL;
}

bool readLocalTime(struct tm& out) {
  time_t now = time(nullptr);
  if (now <= 1600000000UL) return false;
  localtime_r(&now, &out);
  return true;
}

uint32_t dateStamp(const struct tm& t) {
  return (uint32_t)(t.tm_year + 1900) * 10000UL
       + (uint32_t)(t.tm_mon + 1) * 100UL
       + (uint32_t)t.tm_mday;
}

String getCurrentTimeString() {
  struct tm ti;
  if (!readLocalTime(ti)) return "--:--";
  char str[10];
  strftime(str, sizeof(str), "%H:%M", &ti);
  return String(str);
}

String getFormattedUptime() {
  unsigned long s = millis() / 1000;
  int d = s / 86400;
  int h = (s % 86400) / 3600;
  int m = (s % 3600) / 60;
  if (d > 0) return String(d) + "d " + String(h) + "h";
  if (h > 0) return String(h) + "h " + String(m) + "m";
  return String(m) + "m " + String(s % 60) + "s";
}

int getWiFiQuality() {
  if (WiFi.status() != WL_CONNECTED) return 0;
  int dbm = WiFi.RSSI();
  if (dbm <= -100) return 0;
  if (dbm >= -50) return 100;
  return 2 * (dbm + 100);
}

// ==========================================
// 6. ESCAPING HELPERS
// ==========================================
String jsonEscape(const String& in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if ((uint8_t)c < 0x20) {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", (unsigned int)(uint8_t)c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

String htmlEscape(const String& in) {
  String out;
  out.reserve(in.length() + 16);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c;
    }
  }
  return out;
}

String cleanApplianceName(String name) {
  name.trim();
  String out;
  for (size_t i = 0; i < name.length(); i++) {
    if ((uint8_t)name[i] >= 0x20) out += name[i];
  }
  if (out.length() > 25) out = out.substring(0, 25);
  out.trim();
  return out;
}

bool iconIsAllowed(const String& icon) {
  const char* allowed[] = {"lightbulb", "fan", "lamp", "power", "tv", "ac", "water", "door"};
  for (const char* a : allowed) {
    if (icon == a) return true;
  }
  return false;
}

// ==========================================
// 7. LOGGING
// ==========================================
void addLogEvent(int relayIndex, const char* action, const char* trigger, unsigned long durationSecs = 0) {
  struct tm ti;
  char timeStr[9] = "--:--:--";
  if (readLocalTime(ti)) {
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &ti);
  }

  memset(&eventLogs[logHead], 0, sizeof(LogEvent));
  strncpy(eventLogs[logHead].timeStr, timeStr, sizeof(eventLogs[logHead].timeStr) - 1);
  eventLogs[logHead].relayIndex = relayIndex;
  strncpy(eventLogs[logHead].action, action, sizeof(eventLogs[logHead].action) - 1);
  strncpy(eventLogs[logHead].trigger, trigger, sizeof(eventLogs[logHead].trigger) - 1);
  eventLogs[logHead].durationSecs = durationSecs;

  logHead = (logHead + 1) % MAX_LOG_EVENTS;
  if (logCount < MAX_LOG_EVENTS) logCount++;
}

// ==========================================
// 8. RELAY CONTROL
// ==========================================
void setRelayHardware(int index, bool state) {
  int pinValue = RELAY_ACTIVE_LOW ? (state ? LOW : HIGH) : (state ? HIGH : LOW);
  digitalWrite(relayPins[index], pinValue);
}

void syncCloudState(int index, bool state) {
#if ENABLE_ARDUINO_CLOUD
  switch (index) {
    case 0: relay1 = state; break;
    case 1: relay2 = state; break;
    case 2: relay3 = state; break;
    case 3: relay4 = state; break;
  }
#else
  (void)index; (void)state;
#endif
}

void saveRuntimeCounters(int index) {
  preferences.putUInt(("sw" + String(index)).c_str(), switchCount[index]);
  preferences.putULong(("tot" + String(index)).c_str(), totalRuntimeSecs[index]);
  preferences.putULong(("tdy" + String(index)).c_str(), todayRuntimeSecs[index]);
  preferences.putUInt("today_date", todayDate);
}

void applyRelayState(int index, bool state, const char* trigger) {
  if (index < 0 || index >= NUM_RELAYS) return;

  bool prevState = relayStates[index];
  if (state == prevState) {
    setRelayHardware(index, state);
    return;
  }

  relayStates[index] = state;
  setRelayHardware(index, state);

  unsigned long now = millis();

  if (state) {
    sessionStartMillis[index] = now;
    switchCount[index]++;
    addLogEvent(index, "ON", trigger, 0);

    if (inchingSeconds[index] > 0) {
      countdownSeconds[index] = (unsigned long)inchingSeconds[index];
      countdownIsManual[index] = false;
    } else {
      countdownSeconds[index] = 0;
      countdownIsManual[index] = false;
    }
  } else {
    unsigned long sessionDuration = 0;
    if (sessionStartMillis[index] > 0) {
      sessionDuration = (now - sessionStartMillis[index]) / 1000;
      totalRuntimeSecs[index] += sessionDuration;
      todayRuntimeSecs[index] += sessionDuration;
      sessionStartMillis[index] = 0;
    }
    countdownSeconds[index] = 0;
    countdownIsManual[index] = false;
    timerMinutes[index] = 0;
    addLogEvent(index, "OFF", trigger, sessionDuration);
  }

  syncCloudState(index, state);

  if (powerOnMode[index] == 2) {
    char key[8];
    snprintf(key, sizeof(key), "r%d", index);
    preferences.putBool(key, state);
  }
  saveRuntimeCounters(index);

  Serial.printf("[RELAY] Channel %d -> %s (%s)\n", index + 1, state ? "ON" : "OFF", trigger);
}

#if ENABLE_ARDUINO_CLOUD
// Cloud -> device
void onCloudRelay1Change() { applyRelayState(0, (bool)relay1, "Google Home"); }
void onCloudRelay2Change() { applyRelayState(1, (bool)relay2, "Google Home"); }
void onCloudRelay3Change() { applyRelayState(2, (bool)relay3, "Google Home"); }
void onCloudRelay4Change() { applyRelayState(3, (bool)relay4, "Google Home"); }
#endif

// ==========================================
// 9. ANALYTICS DAY ROLLOVER
// ==========================================
void rollOverDay(uint32_t newDate) {
  unsigned long now = millis();
  todayDate = newDate;
  for (int i = 0; i < NUM_RELAYS; i++) {
    // Bank the pre-midnight part of an active session into all-time runtime,
    // then restart the session so "today" only counts the new day.
    if (relayStates[i] && sessionStartMillis[i] > 0) {
      totalRuntimeSecs[i] += (now - sessionStartMillis[i]) / 1000;
      sessionStartMillis[i] = now;
    }
    todayRuntimeSecs[i] = 0;
    saveRuntimeCounters(i);
  }
  preferences.putUInt("today_date", todayDate);
  Serial.printf("[ANALYTICS] New day %lu; today's runtimes reset.\n", (unsigned long)todayDate);
}

void syncCalendarDay(const struct tm& ti) {
  uint32_t date = dateStamp(ti);
  if (todayDate == 0) {
    // First boot of v2 (or the date was never stored): start today's counters
    // clean instead of showing a stale value from the old firmware.
    unsigned long now = millis();
    todayDate = date;
    for (int i = 0; i < NUM_RELAYS; i++) {
      if (relayStates[i] && sessionStartMillis[i] > 0) {
        totalRuntimeSecs[i] += (now - sessionStartMillis[i]) / 1000;
        sessionStartMillis[i] = now;
      }
      todayRuntimeSecs[i] = 0;
      saveRuntimeCounters(i);
    }
    preferences.putUInt("today_date", todayDate);
    Serial.printf("[ANALYTICS] Initialised day %lu; today's runtimes start at zero.\n",
                  (unsigned long)todayDate);
  } else if (date != todayDate) {
    rollOverDay(date);
  }
}

// ==========================================
// 10. WI-FI SETTINGS
// ==========================================
void loadWifiSettings() {
  wifiSsid = preferences.getString("wifi_ssid", DEFAULT_WIFI_SSID);
  wifiPass = preferences.getString("wifi_pass", DEFAULT_WIFI_PASS);
  apPassword = preferences.getString("ap_pass", defaultApPassword);
  if (apPassword.length() < 8) apPassword = defaultApPassword;

  useStaticIP = preferences.getBool("use_static", false);
  staticIpStr = preferences.getString("ip", "");
  gatewayStr  = preferences.getString("gw", "");
  subnetStr   = preferences.getString("mask", "");
  dnsStr      = preferences.getString("dns", "");

  currentTz = preferences.getFloat("tz", 5.5);
  if (currentTz < -12 || currentTz > 14) currentTz = 5.5;
}

bool hasWifiCredentials() {
  return wifiSsid.length() > 0;
}

bool applyStaticConfig() {
  if (!useStaticIP) return false;
  IPAddress ip, gw, mask, dns;
  if (!ip.fromString(staticIpStr) || !gw.fromString(gatewayStr) || !mask.fromString(subnetStr)) {
    Serial.println("[WIFI] Static IP settings are invalid; using DHCP.");
    return false;
  }
  if (!dns.fromString(dnsStr)) dns = gw;
  WiFi.config(ip, gw, mask, dns);
  Serial.printf("[WIFI] Static IP %s / gateway %s\n", staticIpStr.c_str(), gatewayStr.c_str());
  return true;
}

void beginSta() {
  if (!hasWifiCredentials()) return;
  if (!apRunning) {
    WiFi.mode(WIFI_STA);
  }
  WiFi.setHostname(mdnsName);
  applyStaticConfig();
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  lastStaAttempt = millis();
  Serial.printf("[WIFI] Connecting to '%s'...\n", wifiSsid.c_str());
}

void startAP(bool setupMode) {
  if (apRunning) return;

  apSetupMode = setupMode;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAPsetHostname(mdnsName);

  const char* ssid = setupMode ? apSetupSsid : apFallbackSsid;
  const char* pass = setupMode ? nullptr : apPassword.c_str();
  bool ok = WiFi.softAP(ssid, pass);
  if (!ok) {
    Serial.println("[WIFI] Failed to start the access point!");
    apSetupMode = false;
    return;
  }

  dnsServer.start(53, "*", WiFi.softAPIP());
  apRunning = true;
  netMode = setupMode ? NET_AP_SETUP : NET_AP_FALLBACK;
  mdnsRunning = false;
  MDNS.end();

  Serial.printf("[WIFI AP] '%s' active at http://%s (%s)\n",
                ssid, WiFi.softAPIP().toString().c_str(),
                setupMode ? "open setup" : "password protected");
}

void stopAP() {
  if (!apRunning) return;
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  apRunning = false;
  apSetupMode = false;
  netMode = NET_STA;
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
  }
  Serial.println("[WIFI AP] Hotspot stopped; home Wi-Fi is back.");
}

bool startMdns() {
  MDNS.end();
  if (MDNS.begin(mdnsName)) {
    MDNS.addService("http", "tcp", 80);
    // MDNS.end() above also cleared ArduinoOTA's service; put it back.
    MDNS.enableArduino(3232, apPassword.length() > 0);
    mdnsRunning = true;
    lastMdnsAttempt = millis();
    Serial.printf("[mDNS] http://%s.local\n", mdnsName);
    return true;
  }
  mdnsRunning = false;
  lastMdnsAttempt = millis();
  Serial.println("[mDNS] Start failed; will retry.");
  return false;
}

void markStaConnected() {
  if (!staWasConnected) {
    staWasConnected = true;
    staUpAt = millis();
    Serial.printf("[WIFI] Connected to '%s' at http://%s\n",
                  wifiSsid.c_str(), WiFi.localIP().toString().c_str());
  }
  if (!wifiProvisioned) {
    wifiProvisioned = true;
    preferences.putBool("wifi_ok", true);
    Serial.println("[WIFI] Network provisioned; future fallback hotspots are password protected.");
  }
  configTime((long)(currentTz * 3600.0f), daylightOffset, ntpServer);
}

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      markStaConnected();
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (staWasConnected) {
        staDownSince = millis();
      }
      staWasConnected = false;
      lastDisconnectReason = info.wifi_sta_disconnected.reason;
      Serial.printf("[WIFI] Disconnected (reason %d: %s). Router down or out of range.\n",
                    lastDisconnectReason,
                    WiFi.disconnectReasonName((wifi_err_reason_t)lastDisconnectReason));
      break;
    default:
      break;
  }
}

void handleNetworkState() {
  bool staConnected = (WiFi.status() == WL_CONNECTED);

  if (staConnected) {
    if (!staWasConnected) {
      staWasConnected = true;
      staUpAt = millis();
      Serial.printf("[WIFI] Connected to '%s' at http://%s\n",
                    wifiSsid.c_str(), WiFi.localIP().toString().c_str());
      configTime((long)(currentTz * 3600.0f), daylightOffset, ntpServer);
    }
    if (!wifiProvisioned) {
      wifiProvisioned = true;
      preferences.putBool("wifi_ok", true);
      Serial.println("[WIFI] Network provisioned; future fallback hotspots are password protected.");
    }

    if (!mdnsRunning && (millis() - lastMdnsAttempt > 10000)) {
      startMdns();
    }

    // Close the fallback hotspot after a grace period so AP clients can see
    // the "Wi-Fi is back" state and finish what they are doing.
    if (apRunning && !apSetupMode && (millis() - staUpAt > AP_STOP_GRACE_MS)) {
      stopAP();
    }
    // If a not-yet-provisioned setup hotspot managed to connect, close it
    // quickly so the open AP is not left running.
    if (apRunning && apSetupMode && (millis() - staUpAt > 10000)) {
      stopAP();
    }
    return;
  }

  if (staWasConnected) {
    staDownSince = millis();
    staWasConnected = false;
  }
  if (mdnsRunning) {
    MDNS.end();
    mdnsRunning = false;
  }

  if (!apRunning) {
    if (!hasWifiCredentials() || !wifiProvisioned) {
      // Never connected successfully yet: reopen the open setup page so the
      // user can correct the credentials.
      startAP(true);
    } else if (millis() - staDownSince > STA_LOST_GRACE_MS) {
      startAP(false);
    }
    return;
  }

  // AP is active: keep trying to reach the home router in the background.
  if (hasWifiCredentials() && (millis() - lastStaAttempt > WIFI_RETRY_INTERVAL_MS)) {
    beginSta();
  }
}

// ==========================================
// 11. CLOUD SYNC
// ==========================================
void updateCloud() {
#if ENABLE_ARDUINO_CLOUD
  if (!cloudConfigured) return;

  if (!cloudStarted) {
    if (WiFi.status() == WL_CONNECTED && timeIsValid()) {
      // Seed the cloud with the local relay state so a stale cloud value can
      // never switch a load on at boot.
      relay1 = relayStates[0];
      relay2 = relayStates[1];
      relay3 = relayStates[2];
      relay4 = relayStates[3];
      // CloudBool's default constructor leaves _cloud_value uninitialised;
      // syncing it here avoids comparing against garbage on first update.
      relay1.fromLocalToCloud();
      relay2.fromLocalToCloud();
      relay3.fromLocalToCloud();
      relay4.fromLocalToCloud();

      ArduinoCloud.setBoardId(CLOUD_BOARD_ID);
      ArduinoCloud.setSecretDeviceKey(CLOUD_DEVICE_KEY);

      ArduinoCloud.addProperty(relay1, Permission::ReadWrite)
        .publishOnChange(1).onSync(DEVICE_WINS).onUpdate(onCloudRelay1Change);
      ArduinoCloud.addProperty(relay2, Permission::ReadWrite)
        .publishOnChange(1).onSync(DEVICE_WINS).onUpdate(onCloudRelay2Change);
      ArduinoCloud.addProperty(relay3, Permission::ReadWrite)
        .publishOnChange(1).onSync(DEVICE_WINS).onUpdate(onCloudRelay3Change);
      ArduinoCloud.addProperty(relay4, Permission::ReadWrite)
        .publishOnChange(1).onSync(DEVICE_WINS).onUpdate(onCloudRelay4Change);

      ArduinoCloud.begin(cloudConnection, false);
      setDebugMessageLevel(DBG_ERROR);

      cloudStarted = true;
      Serial.println("[CLOUD] Arduino IoT Cloud started.");
    }
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    ArduinoCloud.update();
    cloudConnected = (ArduinoCloud.connected() != 0);
  } else {
    cloudConnected = false;
  }
#endif
}

// ==========================================
// 12. STATUS / WI-FI INFO JSON
// ==========================================
String currentModeString() {
  switch (netMode) {
    case NET_AP_SETUP: return "AP_SETUP";
    case NET_AP_FALLBACK: return "AP_FALLBACK";
    default: return "STA";
  }
}

String currentIpString() {
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  if (apRunning) return WiFi.softAPIP().toString();
  return "0.0.0.0";
}

void handleStatus() {
  if (netMode == NET_AP_SETUP) {
    server.send(403, "application/json", "{\"error\":\"wifi_setup_required\"}");
    return;
  }

  unsigned long now = millis();
  unsigned long liveToday[NUM_RELAYS];
  unsigned long liveTotal[NUM_RELAYS];
  for (int i = 0; i < NUM_RELAYS; i++) {
    liveToday[i] = todayRuntimeSecs[i];
    liveTotal[i] = totalRuntimeSecs[i];
    if (relayStates[i] && sessionStartMillis[i] > 0) {
      unsigned long activeSessionSecs = (now - sessionStartMillis[i]) / 1000;
      liveToday[i] += activeSessionSecs;
      liveTotal[i] += activeSessionSecs;
    }
  }

  String json;
  json.reserve(4096);
  json += "{";

  json += "\"states\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += relayStates[i] ? "true" : "false";
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"names\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += "\"" + jsonEscape(relayNames[i]) + "\"";
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"icons\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += "\"" + jsonEscape(relayIcons[i]) + "\"";
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"pins\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += String(relayPins[i]);
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"timers\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += String(countdownSeconds[i]);
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"timerMins\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += String(timerMinutes[i]);
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"inchings\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += String(inchingSeconds[i]);
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"powerModes\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += String(powerOnMode[i]);
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"schedules\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += "{\"enabled\":" + String(schedules[i].enabled ? "true" : "false") + ",";
    json += "\"onH\":" + String(schedules[i].onHour) + ",\"onM\":" + String(schedules[i].onMin) + ",";
    json += "\"offH\":" + String(schedules[i].offHour) + ",\"offM\":" + String(schedules[i].offMin) + "}";
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"todaySecs\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += String(liveToday[i]);
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"totalSecs\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += String(liveTotal[i]);
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"switches\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += String(switchCount[i]);
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],";

  json += "\"logs\":[";
  for (int i = 0; i < logCount; i++) {
    int idx = (logHead - 1 - i + MAX_LOG_EVENTS) % MAX_LOG_EVENTS;
    if (i > 0) json += ",";
    json += "{\"time\":\"" + jsonEscape(String(eventLogs[idx].timeStr)) + "\",";
    json += "\"relay\":" + String(eventLogs[idx].relayIndex) + ",";
    json += "\"action\":\"" + jsonEscape(String(eventLogs[idx].action)) + "\",";
    json += "\"trigger\":\"" + jsonEscape(String(eventLogs[idx].trigger)) + "\",";
    json += "\"duration\":" + String(eventLogs[idx].durationSecs) + "}";
  }
  json += "],";

  json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"mode\":\"" + currentModeString() + "\",";
  json += "\"ip\":\"" + jsonEscape(currentIpString()) + "\",";
  json += "\"apIp\":\"" + jsonEscape(WiFi.softAPIP().toString()) + "\",";
  json += "\"ssid\":\"" + jsonEscape(wifiSsid) + "\",";
  json += "\"apSsid\":\"" + jsonEscape(apRunning ? (apSetupMode ? String(apSetupSsid) : String(apFallbackSsid)) : String("")) + "\",";
  json += "\"hostname\":\"" + String(mdnsName) + "\",";
  json += "\"mdns\":" + String(mdnsRunning ? "true" : "false") + ",";
  json += "\"cloud\":" + String(cloudConfigured ? (cloudConnected ? "1" : "2") : "0") + ",";
  json += "\"rssi\":" + String(getWiFiQuality()) + ",";
  json += "\"time\":\"" + getCurrentTimeString() + "\",";
  json += "\"uptime\":\"" + getFormattedUptime() + "\",";
  json += "\"tz\":" + String(currentTz, 2) + ",";
  json += "\"fw\":\"" FW_VERSION "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleWifiInfo() {
  String json = "{";
  json += "\"configured\":" + String(hasWifiCredentials() ? "true" : "false") + ",";
  json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"mode\":\"" + currentModeString() + "\",";
  json += "\"ssid\":\"" + jsonEscape(wifiSsid) + "\",";
  json += "\"ip\":\"" + jsonEscape(currentIpString()) + "\",";
  json += "\"apIp\":\"" + jsonEscape(WiFi.softAPIP().toString()) + "\",";
  json += "\"hostname\":\"" + String(mdnsName) + "\",";
  json += "\"rssi\":" + String(getWiFiQuality()) + ",";
  json += "\"useStatic\":" + String(useStaticIP ? "true" : "false") + ",";
  json += "\"ipcfg\":\"" + jsonEscape(staticIpStr) + "\",";
  json += "\"gw\":\"" + jsonEscape(gatewayStr) + "\",";
  json += "\"mask\":\"" + jsonEscape(subnetStr) + "\",";
  json += "\"dns\":\"" + jsonEscape(dnsStr) + "\",";
  const char* reasonName = WiFi.disconnectReasonName((wifi_err_reason_t)lastDisconnectReason);
  json += "\"lastError\":" + String(lastDisconnectReason) + ",";
  json += "\"lastErrorName\":\"" + jsonEscape(String(reasonName ? reasonName : "UNKNOWN")) + "\"";
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleScan() {
  // Synchronous scan. This takes 2-4 seconds and is only used from the Wi-Fi
  // setup page. The ESP32 sees 2.4 GHz networks only.
  int n = WiFi.scanNetworks();
  if (n < 0) {
    delay(500);
    n = WiFi.scanNetworks();
  }

  String json = "{\"networks\":[";
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      json += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
  }
  json += "],\"error\":";
  json += (n < 0) ? "\"scan_failed\"" : "null";
  json += "}";
  WiFi.scanDelete();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// ==========================================
// 13. ACCESS / CSRF HELPERS
// ==========================================
bool sameOrigin() {
  String origin = server.header("Origin");
  if (origin.length() == 0) return true; // curl / same-origin form without Origin
  String host = server.header("Host");
  if (host.length() == 0) return true;
  return origin.endsWith(host);
}

bool requireControl() {
  if (netMode == NET_AP_SETUP) {
    server.send(403, "text/plain", "Configure Wi-Fi first: open /wifi");
    return false;
  }
  if (!sameOrigin()) {
    server.send(403, "text/plain", "Cross-origin request blocked");
    return false;
  }
  return true;
}

// ==========================================
// 14. WEB HANDLERS
// ==========================================
void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  if (netMode == NET_AP_SETUP) {
    server.send(200, "text/html", WIFI_SETUP_HTML);
    return;
  }
  server.send(200, "text/html", INDEX_HTML);
}

void handleWifiPage() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", WIFI_SETUP_HTML);
}

void handleIconSvg() {
  server.send(200, "image/svg+xml", SVG_ICON);
}

void handleIcon192() {
  server.send_P(200, "image/png", (PGM_P)ICON_192, ICON_192_LEN);
}

void handleIcon512() {
  server.send_P(200, "image/png", (PGM_P)ICON_512, ICON_512_LEN);
}

void handleAppleIcon() {
  server.send_P(200, "image/png", (PGM_P)ICON_180, ICON_180_LEN);
}

void handleManifest() {
  server.send(200, "application/json", MANIFEST_JSON);
}

void handleToggle() {
  if (!requireControl()) return;
  if (!server.hasArg("relay")) {
    server.send(400, "application/json", "{\"error\":\"Missing relay\"}");
    return;
  }
  int idx = server.arg("relay").toInt();
  if (idx < 0 || idx >= NUM_RELAYS) {
    server.send(400, "application/json", "{\"error\":\"Invalid index\"}");
    return;
  }
  bool newState = !relayStates[idx];
  applyRelayState(idx, newState, "Web UI");
  server.send(200, "application/json",
              "{\"relay\":" + String(idx) + ",\"state\":" + (newState ? "true" : "false") + "}");
}

void handleAll() {
  if (!requireControl()) return;
  if (!server.hasArg("state")) {
    server.send(400, "application/json", "{\"error\":\"Missing state\"}");
    return;
  }
  bool targetState = (server.arg("state").toInt() == 1);
  for (int i = 0; i < NUM_RELAYS; i++) {
    applyRelayState(i, targetState, "Master Switch");
  }
  handleStatus();
}

void handleSaveConfig() {
  if (!requireControl()) return;
  if (!server.hasArg("relay")) {
    server.send(400, "application/json", "{\"error\":\"Missing relay\"}");
    return;
  }
  int idx = server.arg("relay").toInt();
  if (idx < 0 || idx >= NUM_RELAYS) {
    server.send(400, "application/json", "{\"error\":\"Invalid index\"}");
    return;
  }

  if (server.hasArg("name")) {
    String name = cleanApplianceName(server.arg("name"));
    if (name.length() == 0) name = "Relay " + String(idx + 1);
    if (name != relayNames[idx]) {
      relayNames[idx] = name;
      preferences.putString(("name" + String(idx)).c_str(), relayNames[idx]);
    }
  }

  if (server.hasArg("icon")) {
    String icon = server.arg("icon");
    if (iconIsAllowed(icon) && icon != relayIcons[idx]) {
      relayIcons[idx] = icon;
      preferences.putString(("icon" + String(idx)).c_str(), relayIcons[idx]);
    }
  }

  if (server.hasArg("power")) {
    int mode = server.arg("power").toInt();
    mode = constrain(mode, 0, 2);
    if (mode != powerOnMode[idx]) {
      powerOnMode[idx] = mode;
      preferences.putInt(("pwm" + String(idx)).c_str(), powerOnMode[idx]);
    }
  }

  if (server.hasArg("inching")) {
    int inch = constrain(server.arg("inching").toInt(), 0, 3600);
    if (inch != inchingSeconds[idx]) {
      inchingSeconds[idx] = inch;
      preferences.putInt(("inch" + String(idx)).c_str(), inchingSeconds[idx]);
    }
  }

  // A manual timer is independent from inching. Setting it while the relay is
  // off also switches the relay on (that is the existing behaviour).
  if (server.hasArg("timer")) {
    int mins = constrain(server.arg("timer").toInt(), 0, 1440);
    timerMinutes[idx] = mins;
    if (mins > 0) {
      if (!relayStates[idx]) applyRelayState(idx, true, "Timer Set");
      countdownSeconds[idx] = (unsigned long)mins * 60UL;
      countdownIsManual[idx] = true;
    } else {
      countdownSeconds[idx] = 0;
      countdownIsManual[idx] = false;
    }
  }

  if (server.hasArg("sched")) {
    schedules[idx].enabled = (server.arg("sched").toInt() == 1);
    schedules[idx].onHour  = constrain(server.arg("onH").toInt(), 0, 23);
    schedules[idx].onMin   = constrain(server.arg("onM").toInt(), 0, 59);
    schedules[idx].offHour = constrain(server.arg("offH").toInt(), 0, 23);
    schedules[idx].offMin  = constrain(server.arg("offM").toInt(), 0, 59);

    preferences.putBool(("sc_e" + String(idx)).c_str(), schedules[idx].enabled);
    preferences.putInt(("sc_onh" + String(idx)).c_str(), schedules[idx].onHour);
    preferences.putInt(("sc_onm" + String(idx)).c_str(), schedules[idx].onMin);
    preferences.putInt(("sc_ofh" + String(idx)).c_str(), schedules[idx].offHour);
    preferences.putInt(("sc_ofm" + String(idx)).c_str(), schedules[idx].offMin);
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleSetTz() {
  if (!requireControl()) return;
  if (!server.hasArg("tz")) {
    server.send(400, "application/json", "{\"error\":\"Missing tz\"}");
    return;
  }
  float tz = server.arg("tz").toFloat();
  if (tz < -12 || tz > 14) {
    server.send(400, "application/json", "{\"error\":\"Timezone must be between -12 and 14\"}");
    return;
  }
  currentTz = tz;
  preferences.putFloat("tz", currentTz);
  configTime((long)(currentTz * 3600.0f), daylightOffset, ntpServer);
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleSetApPassword() {
  if (!requireControl()) return;
  if (!server.hasArg("pass")) {
    server.send(400, "application/json", "{\"error\":\"Missing password\"}");
    return;
  }
  String pass = server.arg("pass");
  if (pass.length() < 8 || pass.length() > 63) {
    server.send(400, "application/json", "{\"error\":\"Password must be 8-63 characters\"}");
    return;
  }
  apPassword = pass;
  preferences.putString("ap_pass", apPassword);
  server.send(200, "application/json",
              "{\"status\":\"ok\",\"note\":\"Applies to the web updater now; restart for ArduinoOTA and the hotspot.\"}");
}

void handleResetHistory() {
  if (!requireControl()) return;
  unsigned long now = millis();
  struct tm ti;
  bool haveTime = readLocalTime(ti);
  for (int i = 0; i < NUM_RELAYS; i++) {
    totalRuntimeSecs[i] = 0;
    todayRuntimeSecs[i] = 0;
    switchCount[i] = 0;
    if (relayStates[i]) sessionStartMillis[i] = now;
    saveRuntimeCounters(i);
  }
  if (haveTime) {
    todayDate = dateStamp(ti);
    preferences.putUInt("today_date", todayDate);
  }
  logCount = 0;
  logHead = 0;
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleReboot() {
  if (!requireControl()) return;
  server.send(200, "text/plain", "Rebooting...");
  delay(700);
  ESP.restart();
}

// ---- Wi-Fi setup form handlers ----
void sendWifiResultPage(const String& title, const String& message, bool reboot) {
  String page;
  page.reserve(1200 + message.length());
  page += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<title>" + title + "</title><style>";
  page += "body{background:#0b0f19;color:#f8fafc;font-family:system-ui,sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;margin:0;padding:20px}";
  page += ".c{background:#151d30;border:1px solid rgba(255,255,255,.08);border-radius:18px;padding:26px;max-width:440px;width:100%}";
  page += "h1{color:#38bdf8;font-size:1.35rem;margin:0 0 10px}p{color:#94a3b8;font-size:.9rem;line-height:1.6;margin:0 0 16px}";
  page += "a{color:#38bdf8;text-decoration:none;font-weight:700}</style></head><body><div class='c'>";
  page += "<h1>" + htmlEscape(title) + "</h1><p>" + htmlEscape(message) + "</p>";
  page += "<a href='/wifi'>&larr; Back to Wi-Fi setup</a>";
  page += "</div></body></html>";
  server.send(200, "text/html", page);
  if (reboot) {
    delay(1500);
    ESP.restart();
  }
}

void handleSaveWifi() {
  if (!sameOrigin()) {
    server.send(403, "text/plain", "Cross-origin request blocked");
    return;
  }
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.length() == 0 || ssid.length() > 32) {
    sendWifiResultPage("Invalid network name", "Enter a network name between 1 and 32 characters.", false);
    return;
  }
  if (pass.length() > 0 && pass.length() < 8) {
    sendWifiResultPage("Invalid password", "WPA2 passwords must be at least 8 characters. Leave the password empty only for an open network.", false);
    return;
  }
  if (pass.length() > 64) {
    sendWifiResultPage("Invalid password", "The password is too long.", false);
    return;
  }

  bool wantStatic = server.hasArg("static");
  String ip = server.arg("ip");
  String gw = server.arg("gw");
  String mask = server.arg("mask");
  String dns = server.arg("dns");
  IPAddress tmp;
  if (wantStatic) {
    if (!tmp.fromString(ip) || !tmp.fromString(gw) || !tmp.fromString(mask)) {
      sendWifiResultPage("Invalid static IP", "Check the IP address, gateway and subnet mask, or use DHCP.", false);
      return;
    }
    if (dns.length() > 0 && !tmp.fromString(dns)) {
      sendWifiResultPage("Invalid DNS", "Check the DNS address, or leave it empty.", false);
      return;
    }
  }

  preferences.putString("wifi_ssid", ssid);
  preferences.putString("wifi_pass", pass);
  preferences.putBool("use_static", wantStatic);
  preferences.putString("ip", ip);
  preferences.putString("gw", gw);
  preferences.putString("mask", mask);
  preferences.putString("dns", dns);
  // Keep the open setup hotspot until this network has actually connected
  // once; if the password is wrong the user can rejoin and fix it.
  preferences.putBool("wifi_ok", false);

  Serial.printf("[WIFI] Saved credentials for '%s' (%s). Rebooting.\n",
                ssid.c_str(), wantStatic ? "static IP" : "DHCP");
  sendWifiResultPage("Saved",
                     "Restarting and connecting to \"" + ssid + "\". The setup hotspot will disappear. "
                     "Reconnect your phone to your home Wi-Fi and open http://smarthome.local or the IP from your router.",
                     true);
}

void handleForgetWifi() {
  if (!sameOrigin()) {
    server.send(403, "text/plain", "Cross-origin request blocked");
    return;
  }
  preferences.remove("wifi_ssid");
  preferences.remove("wifi_pass");
  preferences.remove("wifi_ok");
  preferences.remove("use_static");
  preferences.remove("ip");
  preferences.remove("gw");
  preferences.remove("mask");
  preferences.remove("dns");
  Serial.println("[WIFI] Credentials forgotten; restarting into the setup hotspot.");
  sendWifiResultPage("Wi-Fi forgotten",
                     "The device is restarting. Join the open hotspot \"SmartHome-Setup\" to configure Wi-Fi again.",
                     true);
}

// ==========================================
// 15. CAPTIVE PORTAL
// ==========================================
void handleCaptiveProbe() {
  if (apRunning) {
    String url = "http://" + WiFi.softAPIP().toString() + "/";
    server.sendHeader("Location", url);
    server.send(302, "text/plain", "");
  } else {
    server.send(204, "text/plain", "");
  }
}

void handleNotFound() {
  if (apRunning) {
    String url = "http://" + WiFi.softAPIP().toString() + "/";
    server.sendHeader("Location", url);
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "Not found");
  }
}

// ==========================================
// 16. ARDUINO OTA & WEB OTA
// ==========================================
void setupArduinoOTA() {
  ArduinoOTA.setHostname(otaHostname);
  ArduinoOTA.setPassword(apPassword.c_str());
  ArduinoOTA.begin();
}

bool otaAuthOK = false;

void setupWebRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/wifi", HTTP_GET, handleWifiPage);
  server.on("/wifi_info", HTTP_GET, handleWifiInfo);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/save_wifi", HTTP_POST, handleSaveWifi);
  server.on("/forget_wifi", HTTP_POST, handleForgetWifi);

  server.on("/sw.js", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/javascript", SW_CLEANUP);
  });
  server.on("/icon.svg", HTTP_GET, handleIconSvg);
  server.on("/icon-192.png", HTTP_GET, handleIcon192);
  server.on("/icon-512.png", HTTP_GET, handleIcon512);
  server.on("/apple-touch-icon.png", HTTP_GET, handleAppleIcon);
  server.on("/manifest.json", HTTP_GET, handleManifest);

  server.on("/status", HTTP_GET, handleStatus);
  server.on("/toggle", HTTP_POST, handleToggle);
  server.on("/all", HTTP_POST, handleAll);
  server.on("/save_config", HTTP_POST, handleSaveConfig);
  server.on("/set_tz", HTTP_POST, handleSetTz);
  server.on("/set_ap_pass", HTTP_POST, handleSetApPassword);
  server.on("/reset_history", HTTP_POST, handleResetHistory);
  server.on("/reboot", HTTP_POST, handleReboot);

  // Captive portal detection endpoints
  const char* probes[] = {
    "/generate_204", "/gen_204", "/hotspot-detect.html",
    "/library/test/success.html", "/connecttest.txt", "/ncsi.txt",
    "/redirect", "/success.txt"
  };
  for (const char* path : probes) {
    server.on(path, HTTP_GET, handleCaptiveProbe);
  }

  // ---- Web OTA ----
  server.on("/update", HTTP_GET, []() {
    if (netMode == NET_AP_SETUP) {
      server.send(403, "text/plain", "Configure Wi-Fi first");
      return;
    }
    if (!server.authenticate("admin", apPassword.c_str())) {
      server.requestAuthentication();
      return;
    }
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html", OTA_HTML);
  });

  server.on("/update", HTTP_POST, []() {
    if (!otaAuthOK) {
      server.requestAuthentication();
      return;
    }
    bool ok = !Update.hasError();
    server.sendHeader("Connection", "close");
    server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
    if (ok) {
      delay(500);
      ESP.restart();
    }
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      otaAuthOK = server.authenticate("admin", apPassword.c_str());
      if (!otaAuthOK) return;
      if (netMode == NET_AP_SETUP) { otaAuthOK = false; return; }
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (!otaAuthOK) return;
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (!otaAuthOK) return;
      if (Update.end(true)) {
        Serial.println("[WEB-OTA] Firmware upload complete.");
      } else {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      Update.abort();
      otaAuthOK = false;
    }
  });

  server.onNotFound(handleNotFound);
}

// ==========================================
// 17. PHYSICAL SWITCHES
// ==========================================
void handleSwitches() {
  for (int i = 0; i < NUM_RELAYS; i++) {
    int currentReading = digitalRead(switchPins[i]);

    if (currentReading != lastSwitchState[i]) {
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY_MS) {
      if (currentReading != confirmedSwitchState[i]) {
        confirmedSwitchState[i] = currentReading;
#if SWITCH_MOMENTARY
        if (currentReading == LOW) {
          applyRelayState(i, !relayStates[i], "Physical Switch");
        }
#else
        applyRelayState(i, !relayStates[i], "Physical Switch");
#endif
      }
    }
    lastSwitchState[i] = currentReading;
  }
}

// ==========================================
// 18. TIMERS, SCHEDULES, ANALYTICS
// ==========================================
void handleClockWork() {
  if (millis() - lastTimerTick < 1000) return;
  lastTimerTick = millis();

  // Countdown seconds (manual sleep timer or inching pulse)
  for (int i = 0; i < NUM_RELAYS; i++) {
    if (countdownSeconds[i] > 0) {
      countdownSeconds[i]--;
      if (countdownSeconds[i] == 0) {
        bool wasManual = countdownIsManual[i];
        applyRelayState(i, false, wasManual ? "Timer Complete" : "Inching Pulse");
        Serial.printf("[TIMER] Channel %d auto-off.\n", i + 1);
      }
    }
  }

  struct tm ti;
  if (!readLocalTime(ti)) {
    return;
  }

  syncCalendarDay(ti);

  if (ti.tm_min != lastCheckedMinute) {
    lastCheckedMinute = ti.tm_min;
    int nowMinutes = ti.tm_hour * 60 + ti.tm_min;

    for (int i = 0; i < NUM_RELAYS; i++) {
      if (!schedules[i].enabled) continue;

      int onMinutes  = schedules[i].onHour * 60 + schedules[i].onMin;
      int offMinutes = schedules[i].offHour * 60 + schedules[i].offMin;

      // 2-minute catch-up window so a late tick or reboot does not silently
      // skip the day's schedule.
      bool inOnWindow  = (nowMinutes >= onMinutes  && nowMinutes < onMinutes + 2);
      bool inOffWindow = (nowMinutes >= offMinutes && nowMinutes < offMinutes + 2);

      if (inOnWindow && lastSchedOnDate[i] != todayDate) {
        lastSchedOnDate[i] = todayDate;
        if (!relayStates[i]) {
          applyRelayState(i, true, "Schedule ON");
          Serial.printf("[SCHED] Channel %d ON.\n", i + 1);
        }
      }
      if (inOffWindow && lastSchedOffDate[i] != todayDate) {
        lastSchedOffDate[i] = todayDate;
        if (relayStates[i]) {
          applyRelayState(i, false, "Schedule OFF");
          Serial.printf("[SCHED] Channel %d OFF.\n", i + 1);
        }
      }
    }
  }

  // Periodic flash save. Runtime is also saved on every transition.
  if (millis() - lastHistorySave >= 600000UL) {
    lastHistorySave = millis();
    for (int i = 0; i < NUM_RELAYS; i++) {
      preferences.putULong(("tot" + String(i)).c_str(), totalRuntimeSecs[i]);
      preferences.putULong(("tdy" + String(i)).c_str(), todayRuntimeSecs[i]);
    }
    preferences.putUInt("today_date", todayDate);
  }
}

// ==========================================
// 19. STATUS LED
// ==========================================
void updateStatusLed() {
  static unsigned long lastBlink = 0;
  static bool ledOn = false;

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(STATUS_LED, HIGH);
    ledOn = true;
    return;
  }

  unsigned long interval = apSetupMode ? 200 : 1000;
  if (millis() - lastBlink >= interval) {
    lastBlink = millis();
    ledOn = !ledOn;
    digitalWrite(STATUS_LED, ledOn ? HIGH : LOW);
  }
}

// ==========================================
// 20. SETUP
// ==========================================
void loadRelaySettings() {
  for (int i = 0; i < NUM_RELAYS; i++) {
    pinMode(relayPins[i], OUTPUT);

    relayNames[i] = preferences.getString(("name" + String(i)).c_str(), "Relay " + String(i + 1));
    relayIcons[i] = preferences.getString(("icon" + String(i)).c_str(),
                                          i == 0 ? "lightbulb" : (i == 1 ? "fan" : (i == 2 ? "lamp" : "power")));

    powerOnMode[i]    = preferences.getInt(("pwm" + String(i)).c_str(), 2);
    inchingSeconds[i] = preferences.getInt(("inch" + String(i)).c_str(), 0);

    switchCount[i]      = preferences.getUInt(("sw" + String(i)).c_str(), 0);
    totalRuntimeSecs[i] = preferences.getULong(("tot" + String(i)).c_str(), 0);
    todayRuntimeSecs[i] = preferences.getULong(("tdy" + String(i)).c_str(), 0);

    schedules[i].enabled = preferences.getBool(("sc_e" + String(i)).c_str(), false);
    schedules[i].onHour  = preferences.getInt(("sc_onh" + String(i)).c_str(), 18);
    schedules[i].onMin   = preferences.getInt(("sc_onm" + String(i)).c_str(), 0);
    schedules[i].offHour = preferences.getInt(("sc_ofh" + String(i)).c_str(), 22);
    schedules[i].offMin  = preferences.getInt(("sc_ofm" + String(i)).c_str(), 0);

    bool initialState = false;
    if (powerOnMode[i] == 0) {
      initialState = false;
    } else if (powerOnMode[i] == 1) {
      initialState = true;
    } else {
      char key[8];
      snprintf(key, sizeof(key), "r%d", i);
      initialState = preferences.getBool(key, false);
    }

    relayStates[i] = initialState;
    setRelayHardware(i, initialState);
    if (initialState) {
      sessionStartMillis[i] = millis();
      // A restored pulse channel should still finish its pulse instead of
      // staying on forever after a power cut.
      if (inchingSeconds[i] > 0) {
        countdownSeconds[i] = (unsigned long)inchingSeconds[i];
        countdownIsManual[i] = false;
      }
    }

    Serial.printf("[INIT] %s (GPIO %d) -> %s\n",
                  relayNames[i].c_str(), relayPins[i], initialState ? "ON" : "OFF");
  }
}

void setupRelaySwitches() {
  for (int i = 0; i < NUM_RELAYS; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);
    lastSwitchState[i] = digitalRead(switchPins[i]);
    // Seed the confirmed state from the actual level. Without this, a wall
    // switch that is left on would toggle the relay once at every boot.
    confirmedSwitchState[i] = lastSwitchState[i];
  }
}

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println("\n\n==========================================");
  Serial.println("  ESP32 Smart Home Pro v" FW_VERSION);
  Serial.println("  Wi-Fi setup + captive portal + voice");
  Serial.println("==========================================");

  preferences.begin("smarthome", false);

  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  loadRelaySettings();
  setupRelaySwitches();
  loadWifiSettings();
  wifiProvisioned = preferences.getBool("wifi_ok", false);
  todayDate = preferences.getUInt("today_date", 0);

#if ENABLE_ARDUINO_CLOUD
  cloudConfigured = (strlen(CLOUD_BOARD_ID) > 8 && strlen(CLOUD_DEVICE_KEY) > 8);
  if (!cloudConfigured) {
    Serial.println("[CLOUD] No Arduino Cloud credentials in cloud_config.h; voice control disabled.");
  }
#endif

  WiFi.onEvent(onWiFiEvent);
  WiFi.setHostname(mdnsName);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);

  if (hasWifiCredentials()) {
    Serial.printf("[WIFI] Trying saved network '%s'...\n", wifiSsid.c_str());
    WiFi.mode(WIFI_STA);
    applyStaticConfig();
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
    lastStaAttempt = millis();

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_BOOT_TIMEOUT_MS) {
      delay(100);
    }
  } else {
    Serial.println("[WIFI] No saved credentials. Starting the setup hotspot.");
  }

  if (WiFi.status() == WL_CONNECTED) {
    markStaConnected();
  } else {
    staDownSince = millis();
    startAP(!hasWifiCredentials() || !wifiProvisioned);
  }

  setupArduinoOTA();
  setupWebRoutes();
  server.begin();

  if (WiFi.status() == WL_CONNECTED) {
    startMdns();
  }

  Serial.printf("[HTTP] Server ready at http://%s\n", currentIpString().c_str());
  if (apRunning) {
    Serial.printf("[SETUP] Join '%s' and open http://%s\n",
                  apSetupMode ? apSetupSsid : apFallbackSsid,
                  WiFi.softAPIP().toString().c_str());
    if (!apSetupMode) {
      Serial.printf("[SETUP] Fallback hotspot password: %s (change it in the dashboard)\n",
                    apPassword.c_str());
    }
  }
}

// ==========================================
// 21. MAIN LOOP
// ==========================================
void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  if (apRunning) {
    dnsServer.processNextRequest();
  }

  handleNetworkState();
  handleSwitches();
  handleClockWork();
  updateCloud();
  updateStatusLed();
}
