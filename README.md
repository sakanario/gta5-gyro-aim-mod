# GTA V Gyro Aim Mod

Gyro aiming in **GTA V Enhanced** with a Nintendo Switch Pro controller, or any controller with a gyro that Steam Input supports. The gyro moves the camera **only while you are aiming**, on foot or when aiming from a vehicle (LB drive-by).

GTA V does not know when to turn gyro on, and the controller software does not know when you are aiming. This project bridges the two:

```
GTA V ──(ScriptHookV)──> GyroAim.asi ──(shared memory)──> SISR fork ──> virtual Xbox 360 pad ──> GTA V
                         "is the player aiming?"           adds gyro to the right stick while aiming
```

- **GyroAim.asi** (this repo) is a ScriptHookV plugin. Every frame it reads `IS_PLAYER_FREE_AIMING`, `IS_AIM_CAM_ACTIVE` and `IS_PED_IN_ANY_VEHICLE`, then publishes them to the shared memory `Local\GyroAimState`.
- **The SISR fork** ([sakanario/SISR, branch `gtav-gyro-aim`](https://github.com/sakanario/SISR/tree/gtav-gyro-aim), a fork of [Alia5/SISR](https://github.com/Alia5/SISR)) reads that state and mixes the gyro into the emulated right stick while you aim. It has a UI with sensitivity, power curve, minimum output, deadband, invert and gyro calibration.

> Story Mode only. ScriptHookV closes the game in GTA Online.

## Requirements

- GTA V Enhanced (tested on the Epic Games version, build 1.0.1158.x)
- [ScriptHookV](http://www.dev-c.com/gtav/scripthookv/) build with Enhanced support (tested with v3889.0 / 1158.13)
- Steam (for Steam Input) and the [SISR gyro aim fork](https://github.com/sakanario/SISR/tree/gtav-gyro-aim)
- A gyro controller, for example a Switch Pro Controller

## Install

1. **ScriptHookV:** copy `ScriptHookV.dll` and the Enhanced ASI loader (`xinput1_4.dll`) into the GTA V folder. Do not add `dinput8.dll`.
2. **Launch option:** add `-nobattleye` to the game's launch arguments (Epic: Library → GTA V → Manage → Launch Options).
3. **This mod:** copy `GyroAim.asi` (from `build/`, or a Release) into the GTA V folder.
4. **Steam** → Settings → Controller:
   - *Enable Steam Input for Switch Pro controllers*: **ON**
   - *Enable Steam Input for Xbox controllers*: **OFF**. Otherwise Steam wraps SISR's virtual Xbox pad and input can drop after a few minutes.
5. In the SISR controller layout (SISR Marker), set **Gyro Behavior = None**, because the fork does the gyro itself.
6. Start the SISR fork, then start GTA V normally from Epic. Open the **Gyro Aim** card in SISR to tune it.

To check that it works, look at `GyroAim.log` next to `GTA5_Enhanced.exe`, which logs every aim state change, or run `GyroAimMonitor.exe` while the game is running:

```
[20:14:03.512] freeAiming=1 aimCam=1 inVehicle=0
[20:14:03.512] ==> GYRO ON
[20:14:05.087] freeAiming=0 aimCam=0 inVehicle=0
[20:14:05.087] ==> GYRO OFF
```

## Tuning tips

| Setting | What it does |
|---|---|
| Gyro ° Sensitivity | Full stick at `360 / sensitivity` °/s. Higher = faster camera. |
| Joystick Power Curve | 1 = linear, below 1 = aggressive, above 1 = relaxed. |
| Minimum Stick Output | Pushes small movements past GTA's stick deadzone. |
| Gyro Deadband | Ignores slower movement than this (hand jitter, drift). |
| Calibrate | Removes the gyro's resting bias. Put the controller on a table and press it. SISR also recalibrates automatically while the controller is still and you are not aiming. |

If the aim slowly slides while you hold still, calibrate, or raise the deadband a little.

## Build

Needs a C++17 compiler for Windows x64: [llvm-mingw](https://github.com/mstorsjo/llvm-mingw), MSVC, or MinGW g++.

```powershell
.\build.ps1
```

The output goes to `build/GyroAim.asi`, plus `build/GyroAimMonitor.exe` when llvm-mingw is at `..\tools\llvm-mingw`. The plugin binds to `ScriptHookV.dll` at runtime with `GetProcAddress`, so the ScriptHookV SDK is not needed.

## Shared memory contract

`src/gyroaim_ipc.h` defines a 32-byte struct at `Local\GyroAimState`:

| Field | Type | Notes |
|---|---|---|
| magic | u32 | `'GYRO'` (0x4F525947), written last |
| version | u32 | 1 |
| flags | u32 | bit0 freeAiming, bit1 aimCam, bit2 inVehicle, bit3 scriptRunning |
| frame | u32 | increments every game frame |
| updatedMs | u64 | `GetTickCount64()` at the last write |
| writerPid | u32 | GTA V process id |
| reserved | u32 | 0 |

Readers should treat data older than 250 ms as "not aiming". GTA pauses its scripts while unfocused, paused or loading.

## Files

```
src/main.cpp        ScriptHookV plugin (GyroAim.asi)
src/shv.h           Minimal runtime binding to ScriptHookV.dll + natives used
src/gyroaim_ipc.h   Shared memory layout shared with SISR
tools/monitor.cpp   GyroAimMonitor.exe, prints the aim state live
build.ps1           Build script
```

## Disclaimer

Not affiliated with Rockstar Games, Take-Two, Nintendo or Valve. Use it in single player only.
