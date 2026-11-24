// GyroAim: reads the player's aim state every frame and publishes it to shared memory
// ("Local\GyroAimState", see gyroaim_ipc.h) for an external gyro controller (SISR fork).
// Also logs every state change to GyroAim.log next to the game exe.
#include "shv.h"
#include "gyroaim_ipc.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

static HMODULE g_module = nullptr;
static FILE* g_log = nullptr;
static HANDLE g_mapping = nullptr;
static gyroaim::GyroAimState* g_state = nullptr;

static void logf(const char* fmt, ...) {
    if (!g_log) return;
    SYSTEMTIME t; GetLocalTime(&t);
    fprintf(g_log, "[%02d:%02d:%02d.%03d] ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list ap; va_start(ap, fmt); vfprintf(g_log, fmt, ap); va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
}

static bool openSharedState() {
    g_mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                   sizeof(gyroaim::GyroAimState), gyroaim::kMappingName);
    if (!g_mapping) { logf("ERROR: CreateFileMapping failed (%lu)", GetLastError()); return false; }
    g_state = static_cast<gyroaim::GyroAimState*>(
        MapViewOfFile(g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(gyroaim::GyroAimState)));
    if (!g_state) { logf("ERROR: MapViewOfFile failed (%lu)", GetLastError()); return false; }
    g_state->flags = 0;
    g_state->frame = 0;
    g_state->updatedMs = GetTickCount64();
    g_state->writerPid = GetCurrentProcessId();
    g_state->reserved = 0;
    g_state->version = gyroaim::kVersion;
    MemoryBarrier();
    g_state->magic = gyroaim::kMagic; // written last: readers check magic before trusting the rest
    logf("shared memory '%s' ready", gyroaim::kMappingName);
    return true;
}

static void closeSharedState() {
    if (g_state) {
        g_state->flags = 0;
        g_state->updatedMs = 0; // instantly stale for readers
        UnmapViewOfFile(g_state);
        g_state = nullptr;
    }
    if (g_mapping) { CloseHandle(g_mapping); g_mapping = nullptr; }
}

static void scriptMain() {
    logf("script started, game version enum = %d",
         shv::g.getGameVersion ? shv::g.getGameVersion() : -1);
    if (!g_state) openSharedState();

    uint32_t last = ~0u;
    while (true) {
        const int player = natives::PLAYER_ID();
        const int ped = natives::PLAYER_PED_ID();
        uint32_t flags = gyroaim::ScriptRunning;
        if (natives::IS_PLAYER_FREE_AIMING(player))       flags |= gyroaim::FreeAiming;
        if (natives::IS_AIM_CAM_ACTIVE())                 flags |= gyroaim::AimCam;
        if (natives::IS_PED_IN_ANY_VEHICLE(ped, false))   flags |= gyroaim::InVehicle;

        if (g_state) {
            g_state->flags = flags;
            g_state->frame = g_state->frame + 1;
            g_state->updatedMs = GetTickCount64();
        }
        if (flags != last) {
            logf("freeAiming=%d aimCam=%d inVehicle=%d",
                 !!(flags & gyroaim::FreeAiming), !!(flags & gyroaim::AimCam), !!(flags & gyroaim::InVehicle));
            last = flags;
        }
        shv::g.scriptWait(0); // yield until next frame
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        char path[MAX_PATH];
        GetModuleFileNameA(module, path, MAX_PATH);
        if (char* slash = strrchr(path, '\\')) strcpy(slash + 1, "GyroAim.log");
        g_log = fopen(path, "w");
        if (!shv::load()) {
            logf("ERROR: ScriptHookV.dll not found or exports missing - is ScriptHookV installed?");
            return TRUE; // stay loaded but inert, so the game still starts
        }
        logf("ScriptHookV found, registering script");
        shv::g.scriptRegister(module, scriptMain);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (shv::g.scriptUnregister) shv::g.scriptUnregister(g_module);
        closeSharedState();
        if (g_log) { logf("unloaded"); fclose(g_log); g_log = nullptr; }
    }
    return TRUE;
}
