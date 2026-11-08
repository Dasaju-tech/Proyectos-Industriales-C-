#include "Motor_versionC.h"
#include "Order_versionC.h"
#include "DirectedGraph.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

namespace {
int checks = 0;
int failures = 0;

void check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "line " << line << ": " << expression << "\n";
    }
}

#define CHECK(expression) check((expression), #expression, __LINE__)

bool near(double actual, double expected)
{
    return std::fabs(actual - expected) <= 1e-9 * (1.0 + std::fabs(expected));
}

bool same(const sMotorSim& a, const sMotorSim& b)
{
    return a.tao == b.tao && a.Km == b.Km && a.T_ms == b.T_ms &&
        a.Vi == b.Vi && a.Vo == b.Vo && a.Vel == b.Vel && a.Pos == b.Pos;
}

bool same_order(const sORDER& a, const sORDER& b)
{
    if (a.cmd != b.cmd || a.argC != b.argC) return false;
    for (int i = 0; i < 5; ++i)
        if (a.argV[i] != b.argV[i]) return false;
    return true;
}

void test_motor_response()
{
    sMotorSim motor;
    CHECK(MotorSim_Init(&motor, 10.0, 100.0, 1000.0));
    CHECK(near(motor.Vo, 0.0));
    CHECK(near(MotorSim_GetVel(&motor), 0.0));
    CHECK(near(MotorSim_GetPos(&motor), 0.0));
    CHECK(MotorSim_SetVi(&motor, 1.0));
    CHECK(MotorSim_Step(&motor));
    CHECK(near(motor.Vo, 0.1));
    CHECK(near(MotorSim_GetVel(&motor), 10.0));
    CHECK(near(MotorSim_GetPos(&motor), 60.0));

    double expected_position = 60.0;
    for (int step = 2; step <= 300; ++step) {
        CHECK(MotorSim_Step(&motor));
        const double expected_vo = 1.0 - std::pow(0.9, step);
        expected_position += expected_vo * 100.0 * 6.0;
        CHECK(near(motor.Vo, expected_vo));
        CHECK(near(motor.Vel, expected_vo * 100.0));
        CHECK(near(motor.Pos, expected_position));
    }
    CHECK(near(motor.Vel, 100.0));

    CHECK(MotorSim_SetVi(&motor, 0.0));
    const double previous_vo = motor.Vo;
    CHECK(MotorSim_Step(&motor));
    CHECK(near(motor.Vo, previous_vo * 0.9));

    CHECK(MotorSim_SetVi(&motor, -2.0));
    CHECK(MotorSim_SetKm(&motor, 50.0));
    CHECK(MotorSim_SetTao(&motor, 5.0));
    CHECK(MotorSim_SetPos(&motor, -100.0));
    const double start_vo = motor.Vo;
    CHECK(MotorSim_Step(&motor));
    const double next_vo = start_vo + (-2.0 - start_vo) / 5.0;
    CHECK(near(motor.Vo, next_vo));
    CHECK(near(motor.Vel, next_vo * 50.0));
    CHECK(near(motor.Pos, -100.0 + next_vo * 50.0 * 6.0));
    for (int i = 0; i < 200; ++i) CHECK(MotorSim_Step(&motor));
    CHECK(near(motor.Vel, -100.0));

    // The permitted dt=tau boundary reaches the target in one Euler step.
    CHECK(MotorSim_Init(&motor, 0.25, 7.0, 250.0));
    CHECK(MotorSim_SetVi(&motor, -3.0));
    CHECK(MotorSim_Step(&motor));
    CHECK(near(motor.Vo, -3.0));
    CHECK(near(motor.Vel, -21.0));
    CHECK(near(motor.Pos, -31.5));
}

void test_invalid_motor_values()
{
    sMotorSim motor;
    CHECK(MotorSim_Init(&motor, 10.0, 100.0, 1000.0));
    CHECK(MotorSim_SetVi(&motor, 1.0));
    CHECK(MotorSim_Step(&motor));
    const sMotorSim original = motor;
    const double infinity = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();

    CHECK(!MotorSim_Init(&motor, 0.0, 100.0, 1000.0));
    CHECK(same(motor, original));
    CHECK(!MotorSim_Init(&motor, 1.0, -1.0, 100.0));
    CHECK(same(motor, original));
    CHECK(!MotorSim_Init(&motor, 1.0, 1.0, 0.0));
    CHECK(same(motor, original));
    CHECK(!MotorSim_Init(&motor, 1.0, 1.0, 1001.0));
    CHECK(same(motor, original));
    CHECK(!MotorSim_Init(&motor, infinity, 1.0, 100.0));
    CHECK(same(motor, original));
    CHECK(!MotorSim_SetVi(&motor, nan));
    CHECK(!MotorSim_SetPos(&motor, infinity));
    CHECK(!MotorSim_SetKm(&motor, -0.1));
    CHECK(!MotorSim_SetKm(&motor, infinity));
    CHECK(!MotorSim_SetTao(&motor, 0.0));
    CHECK(!MotorSim_SetTao(&motor, 0.5));
    CHECK(same(motor, original));

    // Finite inputs can still overflow the model: reject the whole step.
    CHECK(MotorSim_SetKm(&motor, std::numeric_limits<double>::max()));
    CHECK(MotorSim_SetVi(&motor, std::numeric_limits<double>::max()));
    const sMotorSim overflowing = motor;
    CHECK(!MotorSim_Step(&motor));
    CHECK(same(motor, overflowing));

    CHECK(!MotorSim_Init(0, 1.0, 1.0, 1.0));
    CHECK(!MotorSim_Step(0));
    CHECK(!MotorSim_SetVi(0, 1.0));
    CHECK(!MotorSim_SetPos(0, 1.0));
    CHECK(!MotorSim_SetKm(0, 1.0));
    CHECK(!MotorSim_SetTao(0, 1.0));
}

void test_order_parser()
{
    sORDER order;
    CHECK(Order_parse("0", &order) == 0);
    CHECK(order.cmd == MOTOR_EXIT && order.argC == 0);
    CHECK(Order_parse("  3 -2.5 \t\r\n", &order) == 0);
    CHECK(order.cmd == MOTOR_VOLTAJE && order.argC == 1);
    CHECK(near(order.argV[0], -2.5));
    CHECK(Order_parse("1 1e3", &order) == 0);
    CHECK(order.cmd == MOTOR_KM && near(order.argV[0], 1000.0));
    CHECK(Order_parse("2 10", &order) == 0);
    CHECK(order.cmd == MOTOR_TIME_CONSTANT && near(order.argV[0], 10.0));

    const char* invalid[] = {
        "", " \t\r\n", "1", "2", "3", "0 1", "1 2 3", "4 1",
        "-1", "1.0 2", "3 bad", "3 2junk", "3 nan", "3 inf",
        "3 1e999", "3 1,5", "3 1\n0", "unknown"
    };
    const sORDER previous = order;
    const int count = static_cast<int>(sizeof(invalid) / sizeof(invalid[0]));
    for (int i = 0; i < count; ++i) {
        CHECK(Order_parse(invalid[i], &order) != 0);
        CHECK(same_order(order, previous));
    }

    char boundary[MAX_ORDER_LEN + 3];
    boundary[0] = '3'; boundary[1] = ' ';
    for (int j = 2; j < MAX_ORDER_LEN - 1; ++j) boundary[j] = '0';
    boundary[MAX_ORDER_LEN - 1] = '\0';
    CHECK(Order_parse(boundary, &order) == 0);
    CHECK(near(order.argV[0], 0.0));
    boundary[MAX_ORDER_LEN - 1] = '\r';
    boundary[MAX_ORDER_LEN] = '\n';
    boundary[MAX_ORDER_LEN + 1] = '\0';
    CHECK(Order_parse(boundary, &order) == 0);
    const sORDER before_long = order;
    boundary[MAX_ORDER_LEN - 1] = '0';
    boundary[MAX_ORDER_LEN] = '\0';
    CHECK(Order_parse(boundary, &order) != 0);
    CHECK(same_order(order, before_long));
    CHECK(Order_parse(0, &order) != 0);
    CHECK(Order_parse("0", 0) != 0);
}

void test_directed_graph()
{
    using namespace DirectedGraph;
    const Edge edges[] = { { 0, 2 }, { 1, 2 }, { 2, 3 }, { 0, 4 } };
    int order[MAX_NODES];
    CHECK(TopologicalOrder(6, edges, 4, order));
    int position[6];
    bool seen[6] = { false, false, false, false, false, false };
    for (int i = 0; i < 6; ++i) {
        CHECK(order[i] >= 0 && order[i] < 6);
        if (order[i] >= 0 && order[i] < 6) {
            CHECK(!seen[order[i]]);
            seen[order[i]] = true;
            position[order[i]] = i;
        }
    }
    for (int j = 0; j < 4; ++j)
        CHECK(position[edges[j].from] < position[edges[j].to]);
    CHECK(TopologicalOrder(3, 0, 0, order));
    CHECK(TopologicalOrder(MAX_NODES, 0, 0, order));

    for (int k = 0; k < MAX_NODES; ++k) order[k] = -7;
    const Edge cycle[] = { { 0, 1 }, { 1, 0 } };
    CHECK(!TopologicalOrder(2, cycle, 2, order));
    CHECK(order[0] == -7 && order[1] == -7);
    const Edge self[] = { { 0, 0 } };
    CHECK(!TopologicalOrder(1, self, 1, order));
    CHECK(order[0] == -7);
    const Edge bad_index[] = { { 0, 3 } };
    CHECK(!TopologicalOrder(3, bad_index, 1, order));
    CHECK(!TopologicalOrder(MAX_NODES + 1, 0, 0, order));
    CHECK(!TopologicalOrder(-1, 0, 0, order));
    CHECK(!TopologicalOrder(2, 0, 1, order));
    CHECK(!TopologicalOrder(2, edges, MAX_EDGES + 1, order));
    CHECK(!TopologicalOrder(2, edges, -1, order));
    CHECK(!TopologicalOrder(2, 0, 0, 0));
}
}

int main()
{
    test_motor_response();
    test_invalid_motor_values();
    test_order_parser();
    test_directed_graph();
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
