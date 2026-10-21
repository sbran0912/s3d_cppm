module;

#include <array>
#include <cmath>
#include <random>
#include <algorithm>

namespace {

std::mt19937& rng() {
    static std::mt19937 generator{std::random_device{}()};
    return generator;
}

}

export module math_core;

// ====================================================================
// TYPEN
// ====================================================================

// Matrix4x4 first so Vec3 can reference it
export using Matrix4x4 = std::array<std::array<double, 4>, 4>;

// Forward declarations so matrix function declarations compile
export struct Vec3;
export struct Vec2;

// ---- Matrix function declarations (use forward-declared Vec3/Vec2) ----
export Matrix4x4 TranslateMatrix(double dx, double dy, double dz);
export Matrix4x4 RotateMatrix(double ax, double ay, double az);
export Matrix4x4 MultMatrix(const Matrix4x4& a, const Matrix4x4& b);
export Matrix4x4 IdentityMatrix();
export Matrix4x4 ViewWorldMatrix(const Matrix4x4& view, const Matrix4x4& world);
export Matrix4x4 LookAtMatrix(const Vec3& cameraPos, const Vec3& target, const Vec3& up);
export Vec3 WorldToCamera(const Vec3& point, const Matrix4x4& view, const Matrix4x4& world);
export Vec3 RotateAround(const Vec3& point, const Vec3& pivot, const Matrix4x4& rotation);
export Matrix4x4 PerspectiveMatrix(double fovY, double aspect, double near, double far);
export Vec2 Project(double fov, const Vec3& v);
export double Constrain(double value, double lo, double hi);
export int Random(int n1, int n2);
export double RandomFloat(double lo, double hi);

// ---- Vec3 ----
export struct Vec3 {
    double x = 0, y = 0, z = 0;

    Vec3() = default;
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    // Arithmetic
    Vec3 Add(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3 Sub(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 Scale(double s) const { return {x * s, y * s, z * s}; }

    Vec3 Mag(double m) const {
        const double length = Length();
        if (length == 0) return {0, 0, 0};
        return Scale(m / length);
    }

    Vec3 Limit(double maxLen) const {
        const double mSq = SquaredLength();
        if (mSq > maxLen * maxLen) return Scale(maxLen / std::sqrt(mSq));
        return Clone();
    }

    Vec3 Negate() const { return {-x, -y, -z}; }

    double Dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

    Vec3 Cross(const Vec3& v) const {
        return {
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x,
        };
    }

    double SquaredLength() const { return x * x + y * y + z * z; }
    double Length() const { return std::sqrt(SquaredLength()); }
    double DistanceTo(const Vec3& v) const { return Sub(v).Length(); }

    Vec3 Normalize() const {
        const double length = Length();
        if (length == 0) return {0, 0, 0};
        return Scale(1 / length);
    }

    Vec3 Lerp(const Vec3& v, double t) const { return Add(v.Sub(*this).Scale(t)); }
    Vec3 Clone() const { return {x, y, z}; }

    bool Equals(const Vec3& v, double epsilon) const {
        return std::fabs(x - v.x) < epsilon &&
               std::fabs(y - v.y) < epsilon &&
               std::fabs(z - v.z) < epsilon;
    }

    // Transform (defined after matrix functions)
    Vec3 Transform(const Matrix4x4& m) const;
    Vec3 TransformDir(const Matrix4x4& m) const;
};

// ---- Vec2 ----
export struct Vec2 {
    double X = 0, Y = 0, S = 0;
};

// ====================================================================
// MATRIX-FUNKTIONEN
// ====================================================================

inline Matrix4x4 TranslateMatrix(double dx, double dy, double dz) {
    return Matrix4x4{{
        {1, 0, 0, dx},
        {0, 1, 0, dy},
        {0, 0, 1, dz},
        {0, 0, 0, 1},
    }};
}

inline Matrix4x4 RotateMatrix(double ax, double ay, double az) {
    const Matrix4x4 rx{{
        {1, 0, 0, 0},
        {0, std::cos(ax), -std::sin(ax), 0},
        {0, std::sin(ax), std::cos(ax), 0},
        {0, 0, 0, 1},
    }};
    const Matrix4x4 ry{{
        {std::cos(ay), 0, std::sin(ay), 0},
        {0, 1, 0, 0},
        {-std::sin(ay), 0, std::cos(ay), 0},
        {0, 0, 0, 1},
    }};
    const Matrix4x4 rz{{
        {std::cos(az), -std::sin(az), 0, 0},
        {std::sin(az), std::cos(az), 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
    }};
    return MultMatrix(rz, MultMatrix(ry, rx));
}

inline Matrix4x4 MultMatrix(const Matrix4x4& a, const Matrix4x4& b) {
    Matrix4x4 result{};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return result;
}

inline Matrix4x4 IdentityMatrix() {
    return Matrix4x4{{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
    }};
}

inline Matrix4x4 ViewWorldMatrix(const Matrix4x4& view, const Matrix4x4& world) {
    return MultMatrix(view, world);
}

inline Matrix4x4 LookAtMatrix(const Vec3& cameraPos, const Vec3& target, const Vec3& up) {
    const Vec3 forward = target.Sub(cameraPos).Normalize();
    const Vec3 right = forward.Cross(up).Normalize();
    const Vec3 realUp = right.Cross(forward);

    return Matrix4x4{{
        {right.x, right.y, right.z, -right.Dot(cameraPos)},
        {realUp.x, realUp.y, realUp.z, -realUp.Dot(cameraPos)},
        {forward.x, forward.y, forward.z, -forward.Dot(cameraPos)},
        {0, 0, 0, 1},
    }};
}

inline Vec3 WorldToCamera(const Vec3& point, const Matrix4x4& view, const Matrix4x4& world) {
    return point.Transform(MultMatrix(view, world));
}

inline Vec3 RotateAround(const Vec3& point, const Vec3& pivot, const Matrix4x4& rotation) {
    const Vec3 rel = point.Sub(pivot);
    const Vec3 rotated = rel.Transform(rotation);
    return rotated.Add(pivot);
}

inline Matrix4x4 PerspectiveMatrix(double fovY, double aspect, double near, double far) {
    const double f = 1.0 / std::tan(fovY / 2);
    return Matrix4x4{{
        {f / aspect, 0, 0, 0},
        {0, f, 0, 0},
        {0, 0, (far + near) / (far - near), -2 * near * far / (far - near)},
        {0, 0, 1, 0},
    }};
}

// ---- Vec3::Transform / TransformDir definitions (need Matrix4x4) ----

inline Vec3 Vec3::Transform(const Matrix4x4& m) const {
    return {
        m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3],
        m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3],
        m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3],
    };
}

inline Vec3 Vec3::TransformDir(const Matrix4x4& m) const {
    return {
        x * m[0][0] + y * m[0][1] + z * m[0][2],
        x * m[1][0] + y * m[1][1] + z * m[1][2],
        x * m[2][0] + y * m[2][1] + z * m[2][2],
    };
}

// ====================================================================
// MATH-HELFER
// ====================================================================

export Vec2 Project(double fov, const Vec3& v) {
    const double s = fov / (fov + v.z);
    return Vec2{v.x * s, v.y * s, s};
}

export double Constrain(double value, double lo, double hi) {
    return std::min(hi, std::max(lo, value));
}

export int Random(int n1, int n2) {
    return static_cast<int>(std::floor(RandomFloat(0.0, 1.0) * (n2 - n1) + n1));
}

export double RandomFloat(double lo, double hi) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng()) * (hi - lo) + lo;
}
