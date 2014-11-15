#ifndef VECTOR_H_
#define VECTOR_H_

#include <cmath>

class Vec2 {
public:
    float x;
    float y;

    Vec2() 
        : x(0.0f), 
          y(0.0f) 
    { }

    Vec2(float x, float y) 
        : x(x), 
          y(y) 
    { }

    Vec2(const Vec2& other)
        : x(other.x), 
          y(other.y)
    { }

    bool operator==(const Vec2& rhs) {
        return x == rhs.x && y == rhs.y;
    }

    bool operator!=(const Vec2& rhs) {
        return x != rhs.x || y != rhs.y;
    }

    bool operator<(const Vec2& other) const {
        if (y < other.y) return true;
        if (y == other.y && x < other.x) return true;
        return false;
    }

    Vec2& operator=(const Vec2& other) {
        x = other.x;
        y = other.y;
        return *this;
    }

    Vec2& operator-() {
        x = -x;
        y = -y;
        return *this;
    }

    Vec2& operator*=(int other) {
        x *= other;
        y *= other;
        return  *this;
    }

    float Length() const {
        return static_cast<float>(std::sqrt(x * x + y * y));
    }

    float LengthSquared() const {
        return x * x + y * y;
    }

    double Normalize() {
        float len = Length();
        if (len < DBL_MIN * 2) return 0.0;
        float inv_len = 1.0f / len;
        x *= inv_len;
        y *= inv_len;
        return len;
    }

    static float Dot(Vec2 a, Vec2 b) {
        return a.x * b.x + a.y * b.y;
    }
};

inline Vec2 operator+(const Vec2& a, const Vec2& other) {
    return Vec2(a.x + other.x, a.y + other.y);
}

inline Vec2 operator-(const Vec2& a, const Vec2& other) {
    return Vec2(a.x - other.x, a.y - other.y);
}

inline Vec2 operator*(const Vec2& a, const Vec2& other) {
    return Vec2(a.x * other.x, a.y * other.y);
}

inline Vec2 operator/(const Vec2& a, const Vec2& other) {
    return Vec2(a.x / other.x, a.y / other.y);
}

inline Vec2 operator+(float scalar, const Vec2& a) {
    return Vec2(scalar + a.x, scalar + a.y);
}

inline Vec2 operator+(const Vec2& a, float scalar) {
    return Vec2(a.x + scalar, a.y + scalar);
}

inline Vec2 operator-(float scalar, const Vec2& a) {
    return Vec2(scalar - a.x, scalar - a.y);
}

inline Vec2 operator-(const Vec2& a, float scalar) {
    return Vec2(a.x - scalar, a.y - scalar);
}

inline Vec2 operator*(float scalar, const Vec2& a) {
    return Vec2(scalar * a.x, scalar * a.y);
}

inline Vec2 operator*(const Vec2& a, float scalar) {
    return Vec2(a.x * scalar, a.y * scalar);
}

inline Vec2 operator/(float scalar, const Vec2& a) {
    return Vec2(scalar / a.x, scalar / a.y);
}

inline Vec2 operator/(const Vec2& a, float scalar) {
    return Vec2(a.x / scalar, a.y / scalar);
}

bool operator<(const Vec2& first, const Vec2& second);

#endif
