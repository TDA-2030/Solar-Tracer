/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include <cmath>
#include <array>

// 定义一个向量类
class Vector3 {
public:
    double x, y, z;

    Vector3(double x = 0.0, double y = 0.0, double z = 0.0) : x(x), y(y), z(z) {}

    // 向量加法
    Vector3 operator+(const Vector3 &other) const
    {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    // 向量减法
    Vector3 operator-(const Vector3 &other) const
    {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    // 向量数乘
    Vector3 operator*(double scalar) const
    {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    // 点积
    double dot(const Vector3 &other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }

    // 归一化
    Vector3 normalize() const
    {
        double length = std::sqrt(x * x + y * y + z * z);
        return Vector3(x / length, y / length, z / length);
    }
};

// 定义一个旋转矩阵类
class Matrix3x3 {
public:
    std::array<std::array<double, 3>, 3> data;

    // 构造单位矩阵
    Matrix3x3()
    {
        data = {{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
    }

    // 旋转矩阵乘法
    Matrix3x3 operator*(const Matrix3x3 &other) const
    {
        Matrix3x3 result;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result.data[i][j] = 0.0;
                for (int k = 0; k < 3; ++k) {
                    result.data[i][j] += data[i][k] * other.data[k][j];
                }
            }
        }
        return result;
    }

    // 向量与矩阵的乘法
    Vector3 operator*(const Vector3 &vec) const
    {
        return Vector3(
                   data[0][0] * vec.x + data[0][1] * vec.y + data[0][2] * vec.z,
                   data[1][0] * vec.x + data[1][1] * vec.y + data[1][2] * vec.z,
                   data[2][0] * vec.x + data[2][1] * vec.y + data[2][2] * vec.z
               );
    }
};


Matrix3x3 rotationMatrixX(double angle);
Matrix3x3 rotationMatrixY(double angle);
Matrix3x3 rotationMatrixZ(double angle);
Vector3 eulerToVector(double alpha, double beta, double gamma);
void vectorToEulerAngles(const Vector3 &vec, double &roll, double &pitch, double &yaw);


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
