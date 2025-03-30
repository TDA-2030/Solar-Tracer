
#include "matrix.h"

// 创建绕x轴的旋转矩阵
Matrix3x3 rotationMatrixX(double angle)
{
    Matrix3x3 mat;
    double c = std::cos(angle);
    double s = std::sin(angle);
    mat.data = {{{1.0, 0.0, 0.0}, {0.0, c, -s}, {0.0, s, c}}};
    return mat;
}

// 创建绕y轴的旋转矩阵
Matrix3x3 rotationMatrixY(double angle)
{
    Matrix3x3 mat;
    double c = std::cos(angle);
    double s = std::sin(angle);
    mat.data = {{{c, 0.0, s}, {0.0, 1.0, 0.0}, {-s, 0.0, c}}};
    return mat;
}

// 创建绕z轴的旋转矩阵
Matrix3x3 rotationMatrixZ(double angle)
{
    Matrix3x3 mat;
    double c = std::cos(angle);
    double s = std::sin(angle);
    mat.data = {{{c, -s, 0.0}, {s, c, 0.0}, {0.0, 0.0, 1.0}}};
    return mat;
}

// 欧拉角到向量的转换
Vector3 eulerToVector(double alpha, double beta, double gamma)
{
    Matrix3x3 Rx = rotationMatrixX(alpha);
    Matrix3x3 Ry = rotationMatrixY(beta);
    Matrix3x3 Rz = rotationMatrixZ(gamma);

    Matrix3x3 R = Rz * Ry * Rx;

    // 假设初始向量为单位向量 [1, 0, 0]
    Vector3 initialVector(1.0, 0.0, 0.0);
    return R * initialVector;
}

// 将向量转换为欧拉角
void vectorToEulerAngles(const Vector3 &vec, double &roll, double &pitch, double &yaw)
{
    // 归一化向量
    Vector3 normalizedVec = vec.normalize();

    // 计算俯仰角（pitch）
    pitch = std::asin(-normalizedVec.z);

    // 计算偏航角（yaw）
    yaw = std::atan2(normalizedVec.y, normalizedVec.x);

    // 计算翻滚角（roll），对于单个向量，通常假设没有翻滚
    roll = 0.0;
}
