# Gamma-Slop

An experimental Aroma plugin that lets you change the gamma of the Wii U GamePad screen from the plugin config menu.

**This project was made entirely with AI (Claude, by Anthropic). It has not been compiled or tested on real hardware. Expect bugs and use it at your own risk.**

## How it works

1. **Plugin system.** Gamma-Slop is a WUPS plugin. Aroma's plugin loader runs it inside the Wii U Menu and every game.
2. **The hook.** The system library `avm.rpl` has a function called `AVMSetDRCGamma(float *gamma)` that sets the gamma sent to the GamePad (DRC). Gamma-Slop replaces that function with its own version using WUPS function patching.
3. **The override.** While the override is on, any call to `AVMSetDRCGamma`, from the system or a game, gets Gamma-Slop's value instead of the one it asked for. Gamma-Slop also calls the function itself whenever you move the slider and every time an app starts, in case nothing else ever calls it.
4. **The menu.** Press L + Down + Select on the GamePad to open the plugin menu. You get an on/off toggle and a slider. Slider value / 20 is the gamma, so 20 = 1.00, 10 = 0.50 and 60 = 3.00.
5. **Saving.** Settings are saved with the WUPS storage API, so they survive reboots.

## Known limits

- The scale `AVMSetDRCGamma` expects is undocumented. Gamma-Slop treats it as a plain gamma value, and 1.00 may not be the neutral setting. Tune by eye.
- Gamma only. There is no saturation, contrast or tint control.
- If the gamma resets during gameplay, a per-frame re-apply would be needed. It isn't included.
- Plugins don't run in vWii mode, so Wii games are unaffected.
- Turning the override off stops Gamma-Slop from changing the value, but the last value stays until you reboot.

## Install

1. Copy `Gamma-Slop.wps` to `sd:/wiiu/environments/aroma/plugins`.
2. Boot Aroma.
3. Press L + Down + Select on the GamePad and open "Gamma-Slop".
4. Turn on "Override GamePad gamma", then move "Gamma step".

If the screen looks wrong, turn the override off and reboot. If the console won't boot, delete `Gamma-Slop.wps` from the SD card.

## Build

### Option 1: Docker (no toolchain install, about 5 minutes the first time)

```
docker build . -t gamma-slop-builder
docker run -it --rm -v ${PWD}:/project gamma-slop-builder make
```

Add `DEBUG=1` after `make` for logging. Clean with `make clean` the same way.

### Option 2: Local toolchain

1. Install devkitPro with the `wut` package.
2. Build and install [WUPS](https://github.com/wiiu-env/WiiUPluginSystem) from source.
3. Set `DEVKITPRO` in your environment.
4. Run `make` in this folder.

Output: `Gamma-Slop.wps`.

## Files

- `src/main.cpp`: the whole plugin
- `Makefile`, `Dockerfile`: build setup, adapted from the WUPS C++ example plugin
- `LICENSE`, `.gitignore`

## Author and licence

Written by Claude (AI), made entirely with AI.

Licensed under the [GNU General Public License v3.0](LICENSE). In plain terms:

- You can use, copy, modify and share it.
- If you share a modified version, you must share its source under the same licence.
- There is no warranty.

It links against [WUPS](https://github.com/wiiu-env/WiiUPluginSystem) (LGPL-3.0) and [wut](https://github.com/devkitPro/wut) (zlib), which keep their own licences.
