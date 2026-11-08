// Exercise the real Win32 Timer Queue API, under Windows or Wine.
#include "WindowsMotor.h"
#include <cmath>

namespace
{
int checks = 0;
int failures = 0;

void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        std::fprintf(stderr, "line %d: %s\n", line, expression);
    }
}
#define CHECK(expression) Check((expression), #expression, __LINE__)

bool Near(double a, double b)
{
    return std::fabs(a - b) <= 1e-9 * (1.0 + std::fabs(b));
}

bool WaitForChange(WindowsMotor& motor, double previous_vo, sMotorSim* state)
{
    const DWORD start = GetTickCount();
    do
    {
        *state = motor.Snapshot();
        if (state->Vo != previous_vo) return true;
        Sleep(20);
    } while (GetTickCount() - start < 8000);
    return false;
}
}

int main()
{
    WindowsMotor motor;
    CHECK(motor.Stop());
    CHECK(!motor.Failed());
    sORDER order = {MOTOR_VOLTAJE, 1, {1.0, 0.0, 0.0, 0.0, 0.0}};
    CHECK(motor.Apply(order));
    const bool started = motor.Start();
    CHECK(started);
    if (!started) return 1;
    CHECK(!motor.Start());
    sMotorSim state;
    CHECK(WaitForChange(motor, 0.0, &state));
    CHECK(state.Vo > 0.0 && state.Vo <= 1.0);
    CHECK(Near(state.Vel, state.Vo * 100.0));
    CHECK(state.Pos > 0.0);

    // Commands and callback share a lock, including rejected parameter changes.
    order.cmd = MOTOR_TIME_CONSTANT;
    order.argV[0] = 0.0;
    CHECK(!motor.Apply(order));
    CHECK(Near(motor.Snapshot().tao, 10.0));
    order.cmd = MOTOR_KM;
    order.argV[0] = 50.0;
    CHECK(motor.Apply(order));
    order.cmd = MOTOR_VOLTAJE;
    order.argV[0] = -1.0;
    CHECK(motor.Apply(order));
    const double previous_vo = motor.Snapshot().Vo;
    CHECK(WaitForChange(motor, previous_vo, &state));
    CHECK(state.Vo < previous_vo);
    CHECK(Near(state.Vel, state.Vo * 50.0));
    order.argC = 0;
    CHECK(!motor.Apply(order));
    CHECK(!motor.Failed());

    // Stop must wait for callbacks. A full subsequent period must not tick.
    CHECK(motor.Stop());
    const sMotorSim stopped = motor.Snapshot();
    Sleep(1200);
    state = motor.Snapshot();
    CHECK(state.Vo == stopped.Vo && state.Vel == stopped.Vel && state.Pos == stopped.Pos);
    CHECK(motor.Stop());
    CHECK(motor.Start());
    CHECK(WaitForChange(motor, stopped.Vo, &state));
    CHECK(motor.Stop());

    // A second instance must not affect the first instance's state.
    {
        WindowsMotor other;
        order.argC = 1;
        order.argV[0] = 2.0;
        CHECK(other.Apply(order));
        CHECK(other.Start());
        CHECK(WaitForChange(other, 0.0, &state));
        CHECK(state.Vo > 0.0);
        const sMotorSim first = motor.Snapshot();
        CHECK(first.Vi == -1.0);
        // Destructor exercises synchronous queue shutdown without explicit Stop.
    }
    std::printf("%d Win32 timer checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
