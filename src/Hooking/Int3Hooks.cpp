#include "Int3Hooks.h"

#if !defined(STALKER_SYNTAX_CHECK)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace hooking {

// Минимальные структуры x86 для VEH (см. winnt.h)
struct X86Context {
    uint32_t ContextFlags;                 // +0x00
    uint32_t Dr0, Dr1, Dr2, Dr3, Dr6, Dr7; // +0x04..0x1B
    uint8_t  FloatSave[112];               // +0x1C (FLOATING_SAVE_AREA)
    uint32_t SegGs, SegFs, SegEs, SegDs;   // +0x8C
    uint32_t Edi, Esi, Ebx, Edx, Ecx, Eax; // +0x9C..0xB3
    uint32_t Ebp, Eip, SegCs, EFlags, Esp, SegSs; // +0xB4..0xCB
};
static_assert(offsetof(X86Context, Edi) == 0x9C, "CONTEXT x86 layout");
static_assert(offsetof(X86Context, Eip) == 0xB8, "CONTEXT x86 layout");

struct ExceptionRecord {
    uint32_t ExceptionCode;
    uint32_t ExceptionFlags;
    void*    ExceptionRecordPtr;
    void*    ExceptionAddress;
    uint32_t NumberParameters;
    uint32_t ExceptionInformation[15];
};

struct ExceptionPointers {
    ExceptionRecord* ExceptionRecord;
    X86Context*      ContextRecord;
};

static constexpr uint32_t STATUS_BREAKPOINT  = 0x80000003;
static constexpr uint32_t STATUS_SINGLE_STEP = 0x80000004;
static constexpr uint32_t TF_FLAG            = 0x100;

static bool ProtectMemory(uint32_t address, size_t size, uint32_t newProtect, uint32_t& oldProtect) {
    DWORD oldP = 0;
    BOOL ok = VirtualProtect(reinterpret_cast<void*>(address), size, newProtect, &oldP);
    oldProtect = oldP;
    return ok != FALSE;
}

Int3HookManager& Int3HookManager::Get() {
    static Int3HookManager instance;
    return instance;
}

bool Int3HookManager::Install(uint32_t address, const HookCallbacks& cb, void* userData) {
    if (m_count >= kMaxHooks) return false;
    for (int i = 0; i < m_count; ++i)
        if (m_hooks[i].address == address) return false; // уже есть

    if (!m_initialized) {
        // Заглушка из одной инструкции `ret` для SKIP-режима.
        void* stub = VirtualAlloc(nullptr, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!stub) return false;
        *reinterpret_cast<uint8_t*>(stub) = 0xC3; // ret
        m_retStub = reinterpret_cast<uint32_t>(stub);

        PVOID handler = AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS info) -> LONG {
            return Int3HookManager::Get().VectoredHandler(info);
        });
        if (!handler) return false;
        m_initialized = true;
    }

    Hook& h = m_hooks[m_count];
    h.address = address;
    h.cb = cb;
    h.userData = userData;
    h.armedStep = false;

    uint32_t oldProt = 0;
    if (!ProtectMemory(address, 1, PAGE_EXECUTE_READWRITE, oldProt)) return false;
    h.originalByte = *reinterpret_cast<uint8_t*>(address);
    *reinterpret_cast<uint8_t*>(address) = 0xCC;
    DWORD dummy = 0;
    VirtualProtect(reinterpret_cast<void*>(address), 1, oldProt, &dummy);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(address), 1);

    h.enabled = true;
    ++m_count;
    return true;
}

Int3HookManager::Hook* Int3HookManager::FindByBreakpoint(uint32_t eip) {
    for (int i = 0; i < m_count; ++i) {
        if (m_hooks[i].enabled && m_hooks[i].address + 1 == eip)
            return &m_hooks[i];
    }
    return nullptr;
}

Int3HookManager::Hook* Int3HookManager::FindArmed() {
    for (int i = 0; i < m_count; ++i) {
        if (m_hooks[i].enabled && m_hooks[i].armedStep)
            return &m_hooks[i];
    }
    return nullptr;
}

long STK_STDCALL Int3HookManager::VectoredHandler(void* info) {
    ExceptionPointers* ep = reinterpret_cast<ExceptionPointers*>(info);
    if (!ep || !ep->ExceptionRecord || !ep->ContextRecord) return -1; // EXCEPTION_CONTINUE_SEARCH

    const uint32_t code = ep->ExceptionRecord->ExceptionCode;

    if (code == STATUS_BREAKPOINT) {
        Hook* h = FindByBreakpoint(ep->ContextRecord->Eip);
        if (!h) return -1; // не наш int3 (отладчик и пр.) — отдаём дальше

        HookContext ctx;
        ctx.eip = ep->ContextRecord->Eip;
        ctx.esp = ep->ContextRecord->Esp;
        ctx.ecx = ep->ContextRecord->Ecx;
        ctx.edx = ep->ContextRecord->Edx;
        ctx.eax = ep->ContextRecord->Eax;

        HookAction action = HookAction::RunOriginal;
        if (h->cb.onCall)
            action = h->cb.onCall(ctx, h->userData);

        if (action == HookAction::Skip) {
            // Пропуск: вернуть управление вызывающему через `ret`-заглушку.
            // Работает для cdecl и thiscall без стековых аргументов
            // (все наши SKIP-цели такие).
            ep->ContextRecord->Eax = h->cb.skipReturnValue;
            ep->ContextRecord->Eip = m_retStub;
            return 0; // EXCEPTION_CONTINUE_EXECUTION
        }

        // PRE + оригинал: вернуть первый байт, Eip на начало, включить TF.
        uint32_t oldProt = 0;
        ProtectMemory(h->address, 1, PAGE_EXECUTE_READWRITE, oldProt);
        *reinterpret_cast<uint8_t*>(h->address) = h->originalByte;
        DWORD dummy = 0;
        VirtualProtect(reinterpret_cast<void*>(h->address), 1, oldProt, &dummy);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(h->address), 1);

        ep->ContextRecord->Eip = h->address;
        ep->ContextRecord->EFlags |= TF_FLAG;
        h->armedStep = true;
        return 0;
    }

    if (code == STATUS_SINGLE_STEP) {
        Hook* h = FindArmed();
        if (!h) return -1; // не наш TF — отдаём дальше
        uint32_t oldProt = 0;
        ProtectMemory(h->address, 1, PAGE_EXECUTE_READWRITE, oldProt);
        *reinterpret_cast<uint8_t*>(h->address) = 0xCC;
        DWORD dummy = 0;
        VirtualProtect(reinterpret_cast<void*>(h->address), 1, oldProt, &dummy);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(h->address), 1);

        ep->ContextRecord->EFlags &= ~TF_FLAG;
        h->armedStep = false;
        return 0;
    }

    return -1; // EXCEPTION_CONTINUE_SEARCH
}

} // namespace hooking

#else // STALKER_SYNTAX_CHECK — пустая реализация

namespace hooking {
Int3HookManager& Int3HookManager::Get() {
    static Int3HookManager inst;
    return inst;
}
bool Int3HookManager::Install(uint32_t, const HookCallbacks&, void*) { return true; }
long STK_STDCALL Int3HookManager::VectoredHandler(void*) { return 0; }
} // namespace hooking

#endif
