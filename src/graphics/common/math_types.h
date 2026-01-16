/*
 * math_types.h
 *
 * Graphics API-independent math types for FreeFalcon
 *
 * This header provides vector, matrix, and quaternion types that replace
 * DirectX-specific types (D3DXVECTOR3, D3DXMATRIX, etc.) with portable
 * equivalents that work with both DirectX and Vulkan backends.
 *
 * For now, this provides a thin wrapper around DirectX types to allow
 * gradual migration. In Phase 2, these will be replaced with GLM or
 * custom implementations for Vulkan.
 */

#ifndef GRAPHICS_MATH_TYPES_H
#define GRAPHICS_MATH_TYPES_H

#include <cmath>

// Phase 1: Use DirectX types as backend (for compatibility during migration)
// TODO Phase 2: Replace with GLM (glm::vec2, glm::vec3, glm::vec4, glm::mat4, glm::quat)
#ifdef USE_VULKAN_BACKEND
    // Future: Use GLM for Vulkan
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/gtc/quaternion.hpp>
    #include <glm/gtc/type_ptr.hpp>

    namespace Graphics {
        using Vector2 = glm::vec2;
        using Vector3 = glm::vec3;
        using Vector4 = glm::vec4;
        using Matrix4x4 = glm::mat4;
        using Quaternion = glm::quat;
    }
#else
    // Current: Use DirectX types (during transition)
    #include <d3d.h>
    #include <d3dxmath.h>

    namespace Graphics {
        // Vector types
        using Vector2 = D3DVECTOR2;
        using Vector3 = D3DVECTOR;
        using Vector4 = D3DVECTOR4;

        // Matrix type
        using Matrix4x4 = D3DMATRIX;

        // Quaternion type (D3DX doesn't have quaternion in D3D7, so we define our own)
        struct Quaternion {
            float x, y, z, w;

            Quaternion() : x(0), y(0), z(0), w(1) {}
            Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
        };
    }
#endif

namespace Graphics {

// Common math constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI / 2.0f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// Forward declarations for math utility functions
// These will be implemented in math_utils.cpp

// Vector operations
float VectorLength(const Vector3& v);
float VectorLengthSquared(const Vector3& v);
Vector3 VectorNormalize(const Vector3& v);
float VectorDot(const Vector3& a, const Vector3& b);
Vector3 VectorCross(const Vector3& a, const Vector3& b);
Vector3 VectorAdd(const Vector3& a, const Vector3& b);
Vector3 VectorSubtract(const Vector3& a, const Vector3& b);
Vector3 VectorScale(const Vector3& v, float scale);
Vector3 VectorLerp(const Vector3& a, const Vector3& b, float t);

// Matrix operations
Matrix4x4 MatrixIdentity();
Matrix4x4 MatrixMultiply(const Matrix4x4& a, const Matrix4x4& b);
Matrix4x4 MatrixTranspose(const Matrix4x4& m);
Matrix4x4 MatrixInverse(const Matrix4x4& m);
float MatrixDeterminant(const Matrix4x4& m);

// Transformation matrices
Matrix4x4 MatrixTranslation(float x, float y, float z);
Matrix4x4 MatrixRotationX(float angle);
Matrix4x4 MatrixRotationY(float angle);
Matrix4x4 MatrixRotationZ(float angle);
Matrix4x4 MatrixRotationAxis(const Vector3& axis, float angle);
Matrix4x4 MatrixRotationQuaternion(const Quaternion& q);
Matrix4x4 MatrixScaling(float sx, float sy, float sz);

// View and projection matrices
Matrix4x4 MatrixLookAtLH(const Vector3& eye, const Vector3& at, const Vector3& up); // Left-handed (DX)
Matrix4x4 MatrixLookAtRH(const Vector3& eye, const Vector3& at, const Vector3& up); // Right-handed (Vulkan)
Matrix4x4 MatrixPerspectiveFovLH(float fovy, float aspect, float zn, float zf); // Left-handed (DX)
Matrix4x4 MatrixPerspectiveFovRH(float fovy, float aspect, float zn, float zf); // Right-handed (Vulkan)
Matrix4x4 MatrixOrthographicLH(float width, float height, float zn, float zf);
Matrix4x4 MatrixOrthographicRH(float width, float height, float zn, float zf);

// Quaternion operations
Quaternion QuaternionIdentity();
Quaternion QuaternionMultiply(const Quaternion& a, const Quaternion& b);
Quaternion QuaternionNormalize(const Quaternion& q);
Quaternion QuaternionSlerp(const Quaternion& a, const Quaternion& b, float t);
Quaternion QuaternionFromAxisAngle(const Vector3& axis, float angle);
Quaternion QuaternionFromEuler(float pitch, float yaw, float roll);
void QuaternionToEuler(const Quaternion& q, float& pitch, float& yaw, float& roll);

// Transform a vector by a matrix
Vector3 VectorTransform(const Vector3& v, const Matrix4x4& m);
Vector4 VectorTransform4(const Vector4& v, const Matrix4x4& m);
Vector3 VectorTransformCoord(const Vector3& v, const Matrix4x4& m); // Perspective divide
Vector3 VectorTransformNormal(const Vector3& v, const Matrix4x4& m); // No translation

// Utility: Convert degrees to radians and vice versa
inline float ToRadians(float degrees) { return degrees * DEG_TO_RAD; }
inline float ToDegrees(float radians) { return radians * RAD_TO_DEG; }

// Utility: Clamp value
template<typename T>
inline T Clamp(T value, T min, T max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Utility: Linear interpolation
inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

} // namespace Graphics

#endif // GRAPHICS_MATH_TYPES_H
