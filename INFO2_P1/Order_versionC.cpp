#include "Order_versionC.h"
#include "Numeric.h"
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
const char* SkipSpace(const char* text)
{
    while (*text && std::isspace(static_cast<unsigned char>(*text))) ++text;
    return text;
}
}

int Order_parse(const char* line, sORDER* order)
{
    if (!line || !order) return 1;
    // Bounded scan: reject oversized lines before conversion.
    std::size_t length = 0;
    while (line[length] && line[length] != '\r' && line[length] != '\n')
    {
        if (++length >= MAX_ORDER_LEN) return 1;
    }
    // Only a terminal CR/LF is allowed; never silently ignore a second order.
    const char* tail = line + length;
    if (*tail == '\r') ++tail;
    if (*tail == '\n') ++tail;
    if (*tail) return 1;

    const char* token = SkipSpace(line);
    if (!*token) return 1;
    errno = 0;
    char* end = 0;
    const long command = std::strtol(token, &end, 10);
    if (end == token || errno == ERANGE || command < MOTOR_EXIT ||
        command > MOTOR_VOLTAJE ||
        (*end && !std::isspace(static_cast<unsigned char>(*end)))) return 1;

    sORDER parsed = {0, 0, {0.0, 0.0, 0.0, 0.0, 0.0}};
    parsed.cmd = static_cast<int>(command);
    token = SkipSpace(end);
    if (command != MOTOR_EXIT)
    {
        if (!*token) return 1;
        // A decimal argument, not a hex float or platform-specific nan/inf.
        const char* p = token;
        if (*p == '+' || *p == '-') ++p;
        bool digit = false;
        while (*p >= '0' && *p <= '9') { digit = true; ++p; }
        if (*p == '.')
        {
            ++p;
            while (*p >= '0' && *p <= '9') { digit = true; ++p; }
        }
        if (!digit) return 1;
        if (*p == 'e' || *p == 'E')
        {
            ++p;
            if (*p == '+' || *p == '-') ++p;
            const char* exponent = p;
            while (*p >= '0' && *p <= '9') ++p;
            if (p == exponent) return 1;
        }
        if (*p && !std::isspace(static_cast<unsigned char>(*p))) return 1;
        errno = 0;
        parsed.argV[0] = std::strtod(token, &end);
        if (end != p || errno == ERANGE || !Motor_IsFinite(parsed.argV[0]))
            return 1;
        parsed.argC = 1;
        token = SkipSpace(end);
    }
    if (*token) return 1;
    *order = parsed;
    return 0;
}

int Order_getOrder(sORDER* order)
{
    if (!order) return 1;
    char line[MAX_ORDER_LEN + 2];
    if (!std::fgets(line, sizeof(line), stdin)) return 2;
    if (!std::strchr(line, '\n') && !std::feof(stdin))
    {
        int c;
        do { c = std::getchar(); } while (c != '\n' && c != EOF);
        return 1;
    }
    return Order_parse(line, order);
}
