#pragma once
#include <cmath>
#include <cstdint>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSq() const { return x * x + y * y; }

    Vec2 normalized() const {
        float len = length();
        if (len < 0.0001f) return {0, 0};
        return {x / len, y / len};
    }

    static float distance(const Vec2& a, const Vec2& b) {
        return (a - b).length();
    }

    static Vec2 lerp(const Vec2& a, const Vec2& b, float t) {
        return a + (b - a) * t;
    }
};

struct Color {
    uint8_t r, g, b, a;
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
};
