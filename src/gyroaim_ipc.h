// Shared-memory contract between GyroAim.asi (writer, inside GTA) and any reader
// (the monitor tool now, the SISR fork later).
//
// Mapping name : "Local\GyroAimState"   (same user session; no admin rights needed)
// Size         : sizeof(GyroAimState) = 32 bytes, little-endian, no padding surprises
//
// Offset Size Field
//   0     4   magic        'GYRO' = 0x4F525947
//   4     4   version      layout version, currently 1
//   8     4   flags        bit0 freeAiming, bit1 aimCam, bit2 inVehicle, bit3 scriptRunning
//  12     4   frame        incremented every game frame the script runs
//  16     8   updatedMs    GetTickCount64() at the last write (heartbeat)
//  24     4   writerPid    process id of the game
//  28     4   reserved
//
// The writer stores `flags` with a single aligned 32-bit write, so readers always see a
// consistent set of bits. Readers must treat the state as stale (gyro off) when
// now - updatedMs exceeds ~250 ms: the game is paused, loading, minimised or closed.
#pragma once
#include <cstdint>

namespace gyroaim {

inline constexpr const char* kMappingName = "Local\\GyroAimState";
inline constexpr uint32_t kMagic   = 0x4F525947; // "GYRO"
inline constexpr uint32_t kVersion = 1;
inline constexpr uint64_t kStaleMs = 250;

enum Flags : uint32_t {
    FreeAiming    = 1u << 0,
    AimCam        = 1u << 1,
    InVehicle     = 1u << 2,
    ScriptRunning = 1u << 3,
};

struct GyroAimState {
    uint32_t magic;
    uint32_t version;
    volatile uint32_t flags;
    volatile uint32_t frame;
    volatile uint64_t updatedMs;
    uint32_t writerPid;
    uint32_t reserved;
};
static_assert(sizeof(GyroAimState) == 32, "IPC layout changed");

} // namespace gyroaim
