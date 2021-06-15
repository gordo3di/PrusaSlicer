#include "TryCatchSignal.hpp"



std::sig_atomic_t Slic3r::detail::TryCatchSignal::m_flag = false;
std::jmp_buf Slic3r::detail::TryCatchSignal::m_jbuf;

[[noreturn]] void Slic3r::detail::TryCatchSignal::sig_catcher(int)
{
    m_flag = true;
    std::longjmp(m_jbuf, 0);
}

#ifdef _MSC_VER
#include <windows.h>

int signal_seh_filter(int signal_code, unsigned long seh_code) {
    int ret = EXCEPTION_CONTINUE_SEARCH;
    switch (signal_code) {
    case SIGSEGV:
        if (seh_code == STATUS_ACCESS_VIOLATION)
            ret = EXCEPTION_EXECUTE_HANDLER;
        break;
    case SIGILL:
        if (seh_code == STATUS_ILLEGAL_INSTRUCTION)
            ret = EXCEPTION_EXECUTE_HANDLER;
        break;
    case SIGFPE:
        if (seh_code == STATUS_FLOAT_DIVIDE_BY_ZERO ||
            seh_code == STATUS_FLOAT_OVERFLOW ||
            seh_code == STATUS_FLOAT_UNDERFLOW ||
            seh_code == STATUS_INTEGER_DIVIDE_BY_ZERO)
            ret = EXCEPTION_EXECUTE_HANDLER;
        break;
    default: ret = EXCEPTION_CONTINUE_SEARCH;
    }

    return ret;
}

void Slic3r::try_catch_signal_seh(int sigcnt, const int *sigs, std::function<void()> &&fn, std::function<void()> &&cfn)
{
    __try {
        fn();
    }
    __except(signal_seh_filter(sigs[0], GetExceptionCode())) {
        cfn();
    }
}

#endif
