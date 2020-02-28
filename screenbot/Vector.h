#ifndef VECTOR_H_
#define VECTOR_H_

#include <cmath>
#include <limits>

class Vec2 {
public:
    double x;
    double y;

    Vec2() 
        : x(0.0), 
          y(0.0) 
    { }

    Vec2(double x, double y) 
        : x(x), 
          y(y) 
    { }

    Vec2(const Vec2& other)
        : x(other.x), 
          y(other.y)
    { }

    bool operator==(const Vec2& rhs) const {
        double epsilon = 0.00001;
        return std::fabs(x - rhs.x) < epsilon && std::fabs(y - rhs.y) < epsilon;
    }

    bool operator!=(const Vec2& rhs) const {
        return !(*this == rhs);
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

    Vec2& operator*=(double other) {
        x *= other;
        y *= other;
        return  *this;
    }

    inline double Length() const {
        return std::sqrt(x * x + y * y);
    }

    inline double LengthSquared() const {
        return x * x + y * y;
    }

    void Normalize() {
        double length = Length();
        if (length > std::numeric_limits<double>::epsilon() * 2) {
            x /= length;
            y /= length;
        }
    }

    inline double Dot(const Vec2& other) const {
        return x * other.x + y * other.y;
    }

    Vec2 Perpendicular() const {
        return Vec2(-y, x);
    }

    inline double Distance(const Vec2& other) const {
        double dx = other.x - x;
        double dy = other.y - y;
        return std::sqrt(dx * dx + dy * dy);
    }

    inline void Truncate(double max) {
        if (Length() > max) {
            Normalize();
            *this *= max;
        }
    }

    int Sign(const Vec2& other) const {
        return (y * other.x > x * other.y) ? -1 : 1;
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

inline Vec2 operator+(double scalar, const Vec2& a) {
    return Vec2(scalar + a.x, scalar + a.y);
}

inline Vec2 operator+(const Vec2& a, double scalar) {
    return Vec2(a.x + scalar, a.y + scalar);
}

inline Vec2 operator-(double scalar, const Vec2& a) {
    return Vec2(scalar - a.x, scalar - a.y);
}

inline Vec2 operator-(const Vec2& a, double scalar) {
    return Vec2(a.x - scalar, a.y - scalar);
}

inline Vec2 operator*(double scalar, const Vec2& a) {
    return Vec2(scalar * a.x, scalar * a.y);
}

inline Vec2 operator*(const Vec2& a, double scalar) {
    return Vec2(a.x * scalar, a.y * scalar);
}

inline Vec2 operator/(double scalar, const Vec2& a) {
    return Vec2(scalar / a.x, scalar / a.y);
}

inline Vec2 operator/(const Vec2& a, double scalar) {
    return Vec2(a.x / scalar, a.y / scalar);
}

inline Vec2 Vec2Normalize(const Vec2& vec) {
    Vec2 normalized;
    double length = vec.Length();
    if (length > std::numeric_limits<double>::epsilon() * 2) {
        normalized.x = vec.x / length;
        normalized.y = vec.y / length;
    }
    return normalized;
}

inline Vec2 Vec2Perp(const Vec2& vec) {
  return Vec2(-vec.y, vec.x);
}

inline Vec2 Vec2Rotate(const Vec2& vec, double rads) {
    double cosA = std::cos(rads);
    double sinA = std::sin(rads);
    return Vec2(cosA * vec.x - sinA * vec.y, sinA * vec.x + cosA * vec.y);
}

#endif
