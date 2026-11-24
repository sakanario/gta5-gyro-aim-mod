// Minimal runtime binding to ScriptHookV.dll.
// Resolves exports with GetProcAddress, so it needs neither the SDK nor ScriptHookV.lib
// and builds with any Windows x64 compiler (MSVC, clang-cl, MinGW).
#pragma once
#include <windows.h>
#include <cstdint>

namespace shv {

using ScriptMain = void (*)();

struct Api {
    void (*scriptRegister)(HMODULE, ScriptMain) = nullptr;
    void (*scriptUnregister)(HMODULE) = nullptr;
    void (*scriptWait)(DWORD) = nullptr;
    void (*nativeInit)(uint64_t) = nullptr;
    void (*nativePush64)(uint64_t) = nullptr;
    uint64_t* (*nativeCall)() = nullptr;
    int (*getGameVersion)() = nullptr;
};

inline Api g;

// ScriptHookV exports MSVC-mangled C++ names.
inline bool load() {
    HMODULE m = GetModuleHandleA("ScriptHookV.dll");
    if (!m) m = LoadLibraryA("ScriptHookV.dll");
    if (!m) return false;
    auto get = [m](const char* name) { return reinterpret_cast<void*>(GetProcAddress(m, name)); };
    g.scriptRegister   = reinterpret_cast<decltype(g.scriptRegister)>(get("?scriptRegister@@YAXPEAUHINSTANCE__@@P6AXXZ@Z"));
    g.scriptUnregister = reinterpret_cast<decltype(g.scriptUnregister)>(get("?scriptUnregister@@YAXPEAUHINSTANCE__@@@Z"));
    g.scriptWait       = reinterpret_cast<decltype(g.scriptWait)>(get("?scriptWait@@YAXK@Z"));
    g.nativeInit       = reinterpret_cast<decltype(g.nativeInit)>(get("?nativeInit@@YAX_K@Z"));
    g.nativePush64     = reinterpret_cast<decltype(g.nativePush64)>(get("?nativePush64@@YAX_K@Z"));
    g.nativeCall       = reinterpret_cast<decltype(g.nativeCall)>(get("?nativeCall@@YAPEA_KXZ"));
    g.getGameVersion   = reinterpret_cast<decltype(g.getGameVersion)>(get("?getGameVersion@@YA?AW4eGameVersion@@XZ"));
    return g.scriptRegister && g.scriptUnregister && g.scriptWait &&
           g.nativeInit && g.nativePush64 && g.nativeCall;
}

// Calls a native by hash. Integer/bool/handle args only (enough for this mod).
template <typename R, typename... Args>
R invoke(uint64_t hash, Args... args) {
    g.nativeInit(hash);
    (g.nativePush64(static_cast<uint64_t>(args)), ...);
    return *reinterpret_cast<R*>(g.nativeCall());
}

} // namespace shv

// Native hashes (stable across Legacy/Enhanced; looked up by hash, not by address).
namespace natives {
inline int  PLAYER_ID()                        { return shv::invoke<int>(0x4F8644AF03D0E0D6ull); }
inline int  PLAYER_PED_ID()                    { return shv::invoke<int>(0xD80958FC74E988A6ull); }
inline bool IS_PLAYER_FREE_AIMING(int player)  { return shv::invoke<int>(0x2E397FD2ECD37C87ull, player) != 0; }
inline bool IS_AIM_CAM_ACTIVE()                { return shv::invoke<int>(0x68EDDA28A5976D07ull) != 0; }
inline bool IS_PED_IN_ANY_VEHICLE(int ped, bool atGetIn) { return shv::invoke<int>(0x997ABD671D25CA0Bull, ped, atGetIn ? 1 : 0) != 0; }
} // namespace natives
