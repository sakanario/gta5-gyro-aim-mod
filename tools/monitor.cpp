// GyroAimMonitor: reads "Local\GyroAimState" from outside the game and prints what a
// gyro controller would do. This is the same logic the SISR fork will use.
#include "../src/gyroaim_ipc.h"
#include <windows.h>
#include <cstdio>

using namespace gyroaim;

static SYSTEMTIME g_t; // time of the current poll, captured before any printing
static void stamp() {
    const SYSTEMTIME& t = g_t;
    printf("[%02d:%02d:%02d.%03d] ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0); // show lines immediately, also when piped to a file
    timeBeginPeriod(1);
    printf("GyroAimMonitor - waiting for GTA (shared memory %s)...\n", kMappingName);

    HANDLE mapping = nullptr;
    const GyroAimState* s = nullptr;
    while (!s) {
        mapping = OpenFileMappingA(FILE_MAP_READ, FALSE, kMappingName);
        if (mapping) s = static_cast<const GyroAimState*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(GyroAimState)));
        if (!s) Sleep(500);
    }
    GetLocalTime(&g_t);
    stamp(); printf("connected\n");

    int lastGyro = -1;
    uint32_t lastFlags = ~0u;
    bool lastStale = false;
    uint32_t lastFrame = 0;
    ULONGLONG fpsStart = GetTickCount64();

    while (true) {
        GetLocalTime(&g_t);
        const bool valid = s->magic == kMagic && s->version == kVersion;
        const uint32_t flags = s->flags;
        const ULONGLONG updated = s->updatedMs;
        const ULONGLONG now = GetTickCount64();
        const bool stale = !valid || updated == 0 || now - updated > kStaleMs;

        // Gyro policy: on while the player is free-aiming (on foot with ZL, drive-by with L).
        const int gyro = (!stale && (flags & FreeAiming)) ? 1 : 0;

        if (stale != lastStale) {
            stamp(); printf(stale ? "game not updating (paused/loading/closed) -> gyro OFF\n" : "game live again\n");
            lastStale = stale;
        }
        if (!stale && flags != lastFlags) {
            stamp();
            printf("freeAiming=%d aimCam=%d inVehicle=%d\n",
                   !!(flags & FreeAiming), !!(flags & AimCam), !!(flags & InVehicle));
            lastFlags = flags;
        }
        if (gyro != lastGyro) {
            stamp(); printf("==> GYRO %s\n", gyro ? "ON" : "OFF");
            lastGyro = gyro;
        }
        if (now - fpsStart >= 10000 && !stale) {
            const uint32_t frame = s->frame;
            stamp(); printf("heartbeat ok, script ticks %.1f/s, age %llums\n",
                            (frame - lastFrame) / ((now - fpsStart) / 1000.0), now - updated);
            lastFrame = frame; fpsStart = now;
        }
        Sleep(2);
    }
}
