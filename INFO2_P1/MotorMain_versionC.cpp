#include "Motor_versionC.h"
#include "Order_versionC.h"
#include "Numeric.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include "WindowsMotor.h"
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
