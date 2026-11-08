#ifndef WINDOWS_MOTOR_H
#define WINDOWS_MOTOR_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "Motor_versionC.h"
#include "Order_versionC.h"

// Timer ownership stays on the control thread; Apply/Snapshot share the state
// lock with the callback. Stop waits for all callbacks before destruction.
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
        if (failed_ || order.argC != 1) return false;
        switch (order.cmd)
        {
        case MOTOR_KM: return MotorSim_SetKm(&state_, order.argV[0]);
        case MOTOR_TIME_CONSTANT: return MotorSim_SetTao(&state_, order.argV[0]);
        case MOTOR_VOLTAJE: return MotorSim_SetVi(&state_, order.argV[0]);
        default: return false;
        }
    }

    sMotorSim Snapshot()
    {
        CriticalLock guard(&lock_);
        return state_;
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

#endif
