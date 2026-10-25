# Contributing

Thanks for helping make this project better. Contributions of all kinds are
welcome: bug reports, documentation fixes, feature ideas and pull requests.

## Reporting bugs

Open an issue and include:

- ESP32 core version (Tools → Board Manager) and board variant
- Partition scheme used
- What you expected, what happened, and how to reproduce it
- Relevant serial monitor output at **115200 baud**

## Pull requests

1. Fork, create a branch, keep commits focused and well described.
2. Match the existing code style: C++ in Arduino style, `camelCase`
   functions, 2-space indent, `//` comments that explain *why*.
3. Test on real hardware — the CI workflow only proves the firmware
   **compiles**, not that it behaves.
4. If you touch `web_assets.h`, note that it is hand-maintained; regenerate
   `app_icons.h` with `tools/make_icons.py` only when icons change.

## Security ground rules

- **Never commit `cloud_config.h`** or any Board ID / Device Key / Wi-Fi
  password. Use `cloud_config.example.h` for documentation.
- New endpoints that change state must keep the existing origin checks and
  stay local-LAN-first; the device is not meant to face the internet.
- Do not weaken the setup-mode lockdown (relay control blocked until Wi-Fi is
  provisioned).

## Development loop

- Build: Arduino IDE, board **ESP32 Dev Module**, partition scheme with a
  ≥ 2 MB app partition (e.g. *No FS 4MB (2MB APP x2)* / *Huge APP*).
- Flash once over USB; afterwards `/update` (web OTA, user `admin`) is faster.
- Serial monitor at 115200 shows `[WIFI]`, `[RELAY]`, `[CLOUD]` … tagged logs.
