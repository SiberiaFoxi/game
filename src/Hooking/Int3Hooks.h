#pragma once
// ============================================================================
//  Лёгкий менеджер хуков через int3-точки останова и Vectored Exception
//  Handler. Не требует внешних зависимостей (MinHook не нужен).
//
//  Принцип:
//   * первый байт целевой функции заменяется на 0xCC;
//   * при EXCEPTION_BREAKPOINT с Eip == target+1 вызывается наш обработчик;
//   * PRE-режим (RunOriginal): исходный байт восстанавливается, Eip = target,
//     ставится флаг трассировки (TF); после выполнения первой инструкции
//     приходит EXCEPTION_SINGLE_STEP — снова ставим 0xCC и продолжаем;
//   * SKIP-режим: Eip направляется на заглушку из одной инструкции `ret`,
//     тело функции не выполняется (хук сохраняется), Eax = skipReturnValue.
//
//  Хуки ставятся только на функции главного потока игры — гонок нет.
// ============================================================================
#include <cstdint>

// Переносимость: на платформе проверки синтаксиса (Linux) stdcall нет
#if defined(_WIN32) && !defined(STALKER_SYNTAX_CHECK)
    #define STK_STDCALL __stdcall
#else
    #define STK_STDCALL
#endif

namespace hooking {

// Контекст в момент срабатывания int3 (до выполнения пролога функции)
struct HookContext {
    uint32_t eip;  // адрес точки останова (function+1)
    uint32_t esp;  // стек: [esp] = адрес возврата
    uint32_t ecx;  // this для thiscall
    uint32_t edx;
    uint32_t eax;

    // Аргумент cdecl №i (0-й = первый) со стека
    uint32_t StackArg(int i) const { return *reinterpret_cast<const uint32_t*>(esp + 4 + i * 4); }
    // this для thiscall
    void* This() const { return reinterpret_cast<void*>(ecx); }
};

enum class HookAction {
    RunOriginal, // выполнить оригинальную функцию (PRE-хук)
    Skip,        // пропустить функцию; Eax = возвращаемое значение
};

struct HookCallbacks {
    HookAction (*onCall)(const HookContext& ctx, void* userData) = nullptr;
    uint32_t skipReturnValue = 0;
};

class Int3HookManager {
public:
    static constexpr int kMaxHooks = 16;

    struct Hook {
        uint32_t      address = 0;
        uint8_t       originalByte = 0;
        bool          enabled = false;
        bool          armedStep = false;
        HookCallbacks cb;
        void*         userData = nullptr;
    };

    static Int3HookManager& Get();

    // Устанавливает хук (патчит 0xCC). Вызывать из главного потока игры.
    bool Install(uint32_t address, const HookCallbacks& cb, void* userData);

    // Обработчик исключений. 0 = EXCEPTION_CONTINUE_EXECUTION, -1 = SEARCH.
    long STK_STDCALL VectoredHandler(void* info);

    uint32_t RetStub() const { return m_retStub; }

private:
    Int3HookManager() = default;

    Hook* FindByBreakpoint(uint32_t eip);
    Hook* FindArmed();

    Hook      m_hooks[kMaxHooks];
    int       m_count = 0;
    uint32_t  m_retStub = 0;
    bool      m_initialized = false;
};

} // namespace hooking
