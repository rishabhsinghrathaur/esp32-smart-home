# Voice control: Arduino Cloud → Google Home → Gemini / Assistant

The Gemini app (and Google Assistant) can only control devices that
**Google Home** knows about. The **free** Arduino Cloud plan bridges this
ESP32 to Google Home. No Raspberry Pi, no hub, no paid subscription — but the
device must have internet access.

Everything below is optional: the local dashboard, wall switches, schedules
and timers all work without any of it.

## 1. Arduino Cloud

1. Create a free account at <https://cloud.arduino.cc/>.
2. Create a **Thing** named `Smart Home`.
3. Add four variables, named exactly:

   | Name | Type | Permission | Update |
   | --- | --- | --- | --- |
   | `relay1` | Smart home → **Switch** | Read & Write | On Change |
   | `relay2` | Smart home → **Switch** | Read & Write | On Change |
   | `relay3` | Smart home → **Switch** | Read & Write | On Change |
   | `relay4` | Smart home → **Switch** | Read & Write | On Change |

   Enable **Smart Home Integration → Google Home** for each variable.
4. Associate an ESP32 device with the Thing, then copy the device's
   **Board ID** and **Secret Device Key**.
5. On your machine:

   ```bash
   cp cloud_config.example.h cloud_config.h
   ```

   and paste the values:

   ```cpp
   #define CLOUD_BOARD_ID   "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
   #define CLOUD_DEVICE_KEY "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
   ```

6. Flash, join Wi-Fi. The dashboard's **Voice** chip changes to **Online**
   when the cloud connects.

The free plan allows 2 Things and 5 variables per Thing — four relays use one
Thing and four variables, so it fits.

> `cloud_config.h` is git-ignored. Your Board ID and Device Key are device
> credentials: never commit them, never paste them into issues.

## 2. Google Home

1. Google Home app → **Devices → Add → Works with Google Home** → **Arduino**.
2. Link the same Arduino account. The four switches appear.
3. Rename them in Google Home to natural names — *Living Room Light*, *Fan*,
   *Geyser*, … Voice control uses the **Google Home names**, not `relay1`.

## 3. Gemini

1. Gemini app → profile → **Extensions → Device control** → connect
   **Google Home**.
2. Try: *“Hey Google, turn on the fan”* or ask Gemini directly.

## How the sync behaves

- Local changes (dashboard, switches, schedules) are pushed to the cloud, so
  Google Home always shows the right state.
- On reconnect, **the device state wins** for the first sync, so a stale cloud
  value can never switch a relay on at boot.
- The cloud starts only when Wi-Fi is up **and** the NTP clock is valid (TLS
  certificate checks need the correct time).

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Voice chip stuck at "Offline" | Serial log at 115200, internet access, Board ID / Device Key. |
| Switches not appearing in Google Home | Smart Home Integration enabled per variable; Arduino service linked to the correct account. |
| Commands work slowly | Arduino Cloud round-trip; usually 1–3 s. Local dashboard stays instant. |
| "Device offline" in Google Home | Cloud disconnected — check the dashboard Voice chip and serial log. |

## References

- Arduino Cloud — Google Home guide: <https://docs.arduino.cc/arduino-cloud/guides/google-home/>
- Arduino Cloud plans: <https://cloud.arduino.cc/plans/>
- Google Gemini — control smart home devices: <https://support.google.com/gemini/answer/15335456>
