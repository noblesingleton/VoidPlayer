#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>

class Fixed256
{
public:
    Fixed256() { std::memset(limbs, 0, sizeof(limbs)); }

    Fixed256(double d);
    Fixed256(int64_t i);

    Fixed256 operator+(const Fixed256& other) const;
    Fixed256 operator-(const Fixed256& other) const;
    Fixed256 operator*(const Fixed256& other) const;

    Fixed256 operator>>(int shift) const;
    Fixed256 operator<<(int shift) const;

    float toFloat() const;

private:
    uint64_t limbs[4];

    void negate();
};