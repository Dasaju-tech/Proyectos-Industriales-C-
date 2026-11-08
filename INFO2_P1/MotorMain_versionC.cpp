#include "Motor_versionC.h"
#include "Order_versionC.h"
#include "Numeric.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace
{
void Usage()
{
    std::puts("Motor DC / Euler / C++03\n"
              "  motor --simulate STEPS [VOLTAGE [TAU [KM [DT_MS]]]]\n"
              "  defaults: voltage=1 V, tau=10 s, Km=100 rpm/V, dt=1000 ms\n"
              "  requires: 1 <= steps <= 1000000, tau>0, Km>=0, 0<dt/1000<=tau\n"
              "  motor --help\n"
              "  motor (Windows): interactive timer, commands 0, 1 Km, 2 tau, 3 Vi");
}

bool ParseNumber(const char* text, double* value)
{
    if (!text || !*text || std::strlen(text) > MAX_ORDER_LEN - 3) return false;
    char line[MAX_ORDER_LEN];
    std::strcpy(line, "3 ");
    std::strcat(line, text);
    sORDER order;
    if (Order_parse(line, &order) != 0) return false;
    *value = order.argV[0];
    return true;
}

void CsvRow(long step, const sMotorSim& s)
{
    std::printf("%ld,%.12g,%.12g,%.12g,%.12g,%.12g\n", step,
                step * (s.T_ms / 1000.0), s.Vi, s.Vo, s.Vel, s.Pos);
}

int Simulate(int argc, char** argv)
{
    if (argc < 3 || argc > 7)
    {
        std::fputs("Invalid number of simulation arguments.\n", stderr);
        return 1;
    }
    // Require decimal integer steps, rather than truncating fractional input.
    for (const char* p = argv[2]; *p; ++p)
        if (*p < '0' || *p > '9')
        {
            std::fputs("Invalid step count: expected a decimal integer.\n", stderr);
            return 1;
        }
    errno = 0;
    char* end = 0;
    const long steps = std::strtol(argv[2], &end, 10);
    if (end == argv[2] || *end || errno == ERANGE || steps < 1 || steps > 1000000)
    {
        std::fputs("Invalid step count.\n", stderr);
        return 1;
    }
    double values[4] = {1.0, 10.0, 100.0, 1000.0};
    for (int i = 3; i < argc; ++i)
    {
        if (!ParseNumber(argv[i], &values[i - 3]))
        {
            std::fputs("Invalid numeric parameter.\n", stderr);
            return 1;
        }
    }
    sMotorSim s;
    if (!MotorSim_Init(&s, values[1], values[2], values[3]) ||
        !MotorSim_SetVi(&s, values[0]) ||
        !Motor_IsFinite(steps * (values[3] / 1000.0)))
    {
        std::fputs("Invalid motor parameters: require tau>0, Km>=0, 0<dt<=tau.\n", stderr);
        return 1;
    }
    std::puts("step,time_s,vi_v,vo_v,vel_rpm,pos_deg");
    CsvRow(0, s);
    for (long step = 1; step <= steps; ++step)
    {
        if (!MotorSim_Step(&s))
        {
            std::fprintf(stderr, "Non-finite model result at step %ld.\n", step);
            return 1;
        }
        CsvRow(step, s);
    }
    return std::ferror(stdout) ? 1 : 0;
}

#ifdef _WIN32
class CriticalLock
{
public:
    explicit CriticalLock(CRITICAL_SECTION* lock) : lock_(lock)
        { EnterCriticalSection(lock_); }
    ~CriticalLock() { LeaveCriticalSection(lock_); }
private:
    CRITICAL_SECTION* lock_;
    CriticalLock(const CriticalLock&);
    CriticalLock& operator=(const CriticalLock&);
};

class WindowsMotor
{
public:
    WindowsMotor() : queue_(0), timer_(0), failed_(false)
    {
        InitializeCriticalSection(&lock_);
        MotorSim_Init(&state_, 10.0, 100.0, 1000.0);
    }

    ~WindowsMotor()
    {
        if (!Stop())
        {
            // Never destroy state/lock while a callback could still use them.
            std::fputs("Fatal: timer queue could not be stopped.\n", stderr);
            std::abort();
        }
        DeleteCriticalSection(&lock_);
    }

    bool Start()
    {
        if (queue_) return false;
        queue_ = CreateTimerQueue();
        if (!queue_) return false;
        // Context belongs to this instance, not to a global singleton.
        if (!CreateTimerQueueTimer(&timer_, queue_, TimerMotor, this,
                                   1000, 1000, WT_EXECUTEDEFAULT))
        {
            const DWORD error = GetLastError();
            Stop();
            SetLastError(error);
            return false;
        }
        return true;
    }

    bool Stop()
    {
        if (!queue_) return true;
        // Do not hold lock_ here: a pending callback needs it to finish.
        if (!DeleteTimerQueueEx(queue_, INVALID_HANDLE_VALUE)) return false;
        queue_ = 0;
        timer_ = 0;
        return true;
    }

    bool Apply(const sORDER& order)
    {
        CriticalLock guard(&lock_);
        if (failed_) return false;
        switch (order.cmd)
        {
        case MOTOR_KM: return MotorSim_SetKm(&state_, order.argV[0]);
        case MOTOR_TIME_CONSTANT: return MotorSim_SetTao(&state_, order.argV[0]);
        case MOTOR_VOLTAJE: return MotorSim_SetVi(&state_, order.argV[0]);
        default: return false;
        }
    }

    bool Failed()
    {
        CriticalLock guard(&lock_);
        return failed_;
    }

private:
    static VOID CALLBACK TimerMotor(PVOID context, BOOLEAN)
    {
        WindowsMotor* self = static_cast<WindowsMotor*>(context);
        CriticalLock guard(&self->lock_);
        if (!self->failed_ && !MotorSim_Step(&self->state_)) self->failed_ = true;
        self->Display();
    }

    void Display()
    {
        const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (!GetConsoleScreenBufferInfo(output, &info)) return;
        char status[256];
        // Scientific notation keeps even large finite inputs within this buffer.
        std::sprintf(status, "Vi=% .3e V  Vel=% .3e rpm  Pos=% .3e deg%s",
                     state_.Vi, state_.Vel, state_.Pos,
                     failed_ ? " ERROR: numeric overflow; press Enter" : "");
        const DWORD width = static_cast<DWORD>(info.dwSize.X);
        const COORD origin = {0, 0};
        DWORD written = 0;
        // Write to a fixed row without moving the input cursor.
        FillConsoleOutputCharacterA(output, ' ', width, origin, &written);
        const DWORD length = static_cast<DWORD>(std::strlen(status));
        WriteConsoleOutputCharacterA(output, status, length < width ? length : width,
                                     origin, &written);
    }

    sMotorSim state_;
    HANDLE queue_;
    HANDLE timer_;
    CRITICAL_SECTION lock_;
    bool failed_;
    WindowsMotor(const WindowsMotor&);
    WindowsMotor& operator=(const WindowsMotor&);
};

int Interactive()
{
    std::puts("\nMotor DC: dt=1 s, tau=10 s, Km=100 rpm/V, Vi=0 V\n"
              "Orders: 0 exit | 1 Km | 2 tau_seconds | 3 voltage\n"
              "Use a decimal point. tau must be >= 1 s; Km must be >= 0.");
    WindowsMotor motor;
    if (!motor.Start())
    {
        std::fprintf(stderr, "CreateTimerQueue/Timer failed (%lu).\n",
                     static_cast<unsigned long>(GetLastError()));
        return 1;
    }
    while (!motor.Failed())
    {
        std::fputs("> ", stdout);
        std::fflush(stdout);
        sORDER order;
        const int result = Order_getOrder(&order);
        if (result == 2) break;
        if (result != 0)
        {
            std::puts("Invalid order. Example: 3 -2.5");
            continue;
        }
        if (order.cmd == MOTOR_EXIT) break;
        if (!motor.Apply(order)) std::puts("Rejected: invalid parameter or model failure.");
    }
    const bool failed = motor.Failed();
    if (!motor.Stop())
    {
        std::fputs("Timer shutdown failed.\n", stderr);
        return 1;
    }
    if (failed) std::fputs("Simulation stopped: non-finite model result.\n", stderr);
    return failed ? 1 : 0;
}
#endif
}

int main(int argc, char** argv)
{
    if (argc >= 2 && std::strcmp(argv[1], "--simulate") == 0)
        return Simulate(argc, argv);
    if (argc == 2 && std::strcmp(argv[1], "--help") == 0)
    {
        Usage();
        return 0;
    }
    if (argc != 1)
    {
        std::fputs("Unknown option or unexpected arguments.\n", stderr);
        Usage();
        return 1;
    }
#ifdef _WIN32
    return Interactive();
#else
    Usage();
    std::puts("Linux: use --simulate for deterministic model execution.");
    return 0;
#endif
}
