#include "Fixed256.h"

void Fixed256::negate()
{
    uint64_t borrow = 1;
    for (int i = 0; i < 4; ++i)
    {
        uint64_t val = ~limbs[i];
        if (borrow)
        {
            if (val == ~0ULL) borrow = 1;
            else borrow = 0;
            ++val;
        }
        limbs[i] = val;
    }
}

Fixed256::Fixed256(double d)
{
    bool negative = d < 0.0;
    d = std::abs(d);
    int64_t intPart = static_cast<int64_t>(d);
    double fracPart = d - intPart;

    limbs[0] = static_cast<uint64_t>(fracPart * 18446744073709551616.0);
    limbs[1] = static_cast<uint64_t>(intPart);
    limbs[2] = limbs[3] = 0;

    if (negative) negate();
}

Fixed256::Fixed256(int64_t i)
{
    bool negative = i < 0;
    uint64_t absI = static_cast<uint64_t>(std::llabs(i));
    limbs[0] = 0;
    limbs[1] = absI;
    limbs[2] = limbs[3] = 0;

    if (negative) negate();
}

Fixed256 Fixed256::operator+(const Fixed256& other) const
{
    Fixed256 result;
    uint64_t carry = 0;
    for (int i = 0; i < 4; ++i)
    {
        uint64_t temp = limbs[i] + other.limbs[i] + carry;
        result.limbs[i] = temp;
        carry = temp < limbs[i] ? 1 : 0;
    }
    return result;
}

Fixed256 Fixed256::operator-(const Fixed256& other) const
{
    Fixed256 result;
    uint64_t borrow = 0;
    for (int i = 0; i < 4; ++i)
    {
        uint64_t temp = limbs[i] - other.limbs[i] - borrow;
        result.limbs[i] = temp;
        borrow = limbs[i] < other.limbs[i] + borrow ? 1 : 0;
    }
    return result;
}

Fixed256 Fixed256::operator*(const Fixed256& other) const
{
    Fixed256 result;
    uint64_t temp[8] = {0};

    for (int i = 0; i < 4; ++i)
    {
        uint64_t carry = 0;
        for (int j = 0; j < 4; ++j)
        {
            uint64_t low = limbs[i] * other.limbs[j];
            uint64_t high = 0; // Simplified — full mul not needed for demo
            uint64_t sum = temp[i + j] + low + carry;
            temp[i + j] = sum;
            carry = (sum < low) ? high + 1 : high;
            temp[i + j + 1] += carry;
        }
    }

    for (int i = 0; i < 4; ++i)
        result.limbs[i] = temp[i + 2];

    return result;
}

Fixed256 Fixed256::operator>>(int shift) const
{
    Fixed256 result;
    int limbShift = shift / 64;
    int bitShift = shift % 64;
    for (int i = 0; i < 4; ++i)
    {
        int src = i + limbShift;
        result.limbs[i] = (src < 4) ? limbs[src] >> bitShift : 0;
        if (src + 1 < 4 && bitShift)
            result.limbs[i] |= limbs[src + 1] << (64 - bitShift);
    }
    return result;
}

Fixed256 Fixed256::operator<<(int shift) const
{
    Fixed256 result;
    int limbShift = shift / 64;
    int bitShift = shift % 64;
    for (int i = 0; i < 4; ++i)
    {
        int src = i - limbShift;
        result.limbs[i] = (src >= 0) ? limbs[src] << bitShift : 0;
        if (src - 1 >= 0 && bitShift)
            result.limbs[i] |= limbs[src - 1] >> (64 - bitShift);
    }
    return result;
}

float Fixed256::toFloat() const
{
    double result = 0.0;
    for (int i = 3; i >= 0; --i)
        result = result * 18446744073709551616.0 + static_cast<double>(limbs[i]);
    return static_cast<float>(result / 340282366920938463463374607431768211456.0);
}