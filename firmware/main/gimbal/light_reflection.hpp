#pragma once
#include <vector>
#include <cmath>

class LightReflection {
public:
    struct Vector3 {
        double x, y, z;
        Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
        
        Vector3 operator+(const Vector3& other) const {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }
        
        Vector3 operator-(const Vector3& other) const {
            return Vector3(x - other.x, y - other.y, z - other.z);
        }
        
        Vector3 operator*(double scalar) const {
            return Vector3(x * scalar, y * scalar, z * scalar);
        }
        
        double dot(const Vector3& other) const {
            return x * other.x + y * other.y + z * other.z;
        }
    };

    /**
     * 将方位角和仰角转换为三维单位向量
     * @param azimuth 方位角(度)，从y轴正方向(北)开始顺时针计算
     *                0° = 北(y+)，90° = 东(x+)，180° = 南(y-)，270° = 西(x-)
     * @param elevation 仰角(度)，从x-y平面向上为正，范围[-90°, 90°]
     * @return [x, y, z]单位向量
     */
    Vector3 angle_to_vector(double azimuth, double elevation) {
        // 转换为弧度
        double az_rad = to_radians(azimuth);
        double el_rad = to_radians(elevation);
        
        // 计算三维向量分量
        double x = cos(el_rad) * sin(az_rad);
        double y = cos(el_rad) * cos(az_rad);
        double z = sin(el_rad);
        
        return normalize(Vector3(x, y, z));
    }

    /**
     * 将三维向量转换为方位角和仰角
     * @param vector [x, y, z]向量
     * @return pair<方位角, 仰角> 单位为度
     */
    std::pair<double, double> vector_to_angle(const Vector3& vector) {
        Vector3 norm_vector = normalize(vector);
        double elevation = to_degrees(asin(norm_vector.z));
        double azimuth = to_degrees(atan2(norm_vector.x, norm_vector.y));
        
        // 确保方位角在0-360度之间
        if (azimuth < 0) {
            azimuth += 360;
        }
            
        return std::make_pair(azimuth, elevation);
    }

    /**
     * 计算反射向量
     * @param incident_vector 入射光线向量，入射向量和反射向量都是指向外的方向
     * @param normal_vector 表面法向量
     * @return 反射向量
     */
    Vector3 calculate_reflection(Vector3 incident_vector, Vector3 normal_vector) {
        incident_vector = normalize(incident_vector * -1);  // 将入射向量反向
        normal_vector = normalize(normal_vector);
        
        double dot_product = incident_vector.dot(normal_vector);
        return normalize(incident_vector - normal_vector * (2 * dot_product));
    }

    /**
     * 根据入射向量和反射向量计算反射面的法向量
     * @param incident_vector 入射光线向量，入射向量和反射向量都是指向外的方向
     * @param reflection_vector 反射光线向量
     * @return 反射面的法向量（单位向量）
     */
    Vector3 calculate_normal(Vector3 incident_vector, Vector3 reflection_vector) {
        incident_vector = normalize(incident_vector);
        reflection_vector = normalize(reflection_vector);
        return normalize(incident_vector + reflection_vector);
    }

private:
    /**
     * 将向量标准化为单位向量
     */
    Vector3 normalize(const Vector3& vector) {
        double norm = sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
        if (norm == 0) return vector;
        return Vector3(vector.x / norm, vector.y / norm, vector.z / norm);
    }

    double to_radians(double degrees) {
        return degrees * PI / 180.0;
    }

    double to_degrees(double radians) {
        return radians * 180.0 / PI;
    }
    const double PI = 3.14159265358979323846;
};
