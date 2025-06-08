#include <math.h>
#include "app_datafusion.h"
#include "vqf/basicvqf.h"
#include "MahonyAHRS/MahonyAHRS.h"

// Kalman Filter
typedef struct {
    float angle;      // The calculated angle
    float bias;       // The gyro bias
    float rate;       // The rate from gyro

    float P[2][2];    // Error covariance matrix
    float Q_angle;    // Process noise variance for angle
    float Q_bias;     // Process noise variance for bias
    float R_measure;  // Measurement noise variance
} KalmanFilter;

// Initialize Kalman Filter
static void KalmanFilter_Init(KalmanFilter *kf) 
{
    kf->angle = 0.0f;
    kf->bias = 0.0f;
    kf->rate = 0.0f;

    kf->P[0][0] = 0.0f;
    kf->P[0][1] = 0.0f;
    kf->P[1][0] = 0.0f;
    kf->P[1][1] = 0.0f;

    kf->Q_angle = 0.005f;
    kf->Q_bias = 0.003f;
    kf->R_measure = 0.03f;
}

// Update Kalman Filter
static float KalmanFilter_Update(KalmanFilter *kf, float newAngle, float newRate, float dt) 
{
    // Predict step
    kf->rate = newRate - kf->bias;
    kf->angle += dt * kf->rate;

    kf->P[0][0] += dt * (dt * kf->P[1][1] - kf->P[0][1] - kf->P[1][0] + kf->Q_angle);
    kf->P[0][1] -= dt * kf->P[1][1];
    kf->P[1][0] -= dt * kf->P[1][1];
    kf->P[1][1] += kf->Q_bias * dt;

    // Measurement update
    float S = kf->P[0][0] + kf->R_measure;
    float K[2]; // Kalman gain
    K[0] = kf->P[0][0] / S;
    K[1] = kf->P[1][0] / S;

    float y = newAngle - kf->angle; // Angle difference
    kf->angle += K[0] * y;
    kf->bias += K[1] * y;

    // Update error covariance matrix
    float P00_temp = kf->P[0][0];
    float P01_temp = kf->P[0][1];

    kf->P[0][0] -= K[0] * P00_temp;
    kf->P[0][1] -= K[0] * P01_temp;
    kf->P[1][0] -= K[1] * P00_temp;
    kf->P[1][1] -= K[1] * P01_temp;

    return kf->angle;
}

void datafusion_update(imu_data_t * imu_data, float dt)
{
    static int initialized = 0;
#define METHOD 1

#if METHOD==1
    static Mahony filter;
    if (!initialized) {
        filter.begin(1.0f / dt);
        initialized = 1;
    }
    // Update the Mahony filter with IMU data
    filter.updateIMU(imu_data->gyro.x, imu_data->gyro.y, imu_data->gyro.z,
                     imu_data->acc.x, imu_data->acc.y, imu_data->acc.z);
    // Get the roll, pitch, and yaw angles
    filter.getAngle(imu_data->angle.data);
#elif METHOD==2
    static KalmanFilter kfRoll, kfPitch;

    if (!initialized) {
        KalmanFilter_Init(&kfRoll);
        KalmanFilter_Init(&kfPitch);
        initialized = 1;
    }

    // Calculate roll and pitch angles from accelerometer
    float accRoll = atan2f(imu_data->acc.y, imu_data->acc.z) * RAD_TO_DEG;
    float accPitch = atan2f(-imu_data->acc.x, sqrtf(imu_data->acc.y * imu_data->acc.y + imu_data->acc.z * imu_data->acc.z)) * RAD_TO_DEG;

    // Update Kalman filters for roll and pitch
    imu_data->angle.x = KalmanFilter_Update(&kfRoll, accRoll, imu_data->gyro.x, dt);
    imu_data->angle.y = KalmanFilter_Update(&kfPitch, accPitch, imu_data->gyro.y, dt);

    // For yaw, integrate gyroZ directly (no accelerometer correction)
    static float yawAngle = 0.0f;
    yawAngle += imu_data->gyro.z * dt;
    imu_data->angle.z = yawAngle;
#elif METHOD==3
    // Use BasicVQF for data fusion
    static BasicVQF vqf(0.01); // Example sampling times
    vqf.updateGyr((vqf_real_t[3]){imu_data->gyro.x, imu_data->gyro.y, imu_data->gyro.z}, dt);
    vqf.updateAcc((vqf_real_t[3]){imu_data->acc.x, imu_data->acc.y, imu_data->acc.z});
    vqf_real_t quat[4];
    vqf.getQuat6D(quat);
    quat_to_euler(quat, imu_data->angle.data); // 四元数转欧拉角
    
#else
    // Default method: Use Mahony filter
#endif
}
