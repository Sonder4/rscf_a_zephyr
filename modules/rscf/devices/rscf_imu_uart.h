#ifndef RSCF_IMU_UART_H_
#define RSCF_IMU_UART_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RSCF_SABER_IMU_TOPIC_NAME "saber_imu"

struct rscf_saber_vector3f {
  float x;
  float y;
  float z;
};

struct rscf_saber_euler {
  float roll;
  float pitch;
  float yaw;
};

struct rscf_saber_topic_data {
  struct rscf_saber_vector3f kalman_acc;
  struct rscf_saber_vector3f kalman_gyro;
  struct rscf_saber_euler euler;
  uint32_t update_tick;
  uint32_t sample_seq;
};

int RSCFImuUartInit(void);
int RSCFImuUartInjectFrame(const uint8_t *data, size_t len);
const struct rscf_saber_topic_data *RSCFImuUartLatest(void);
bool RSCFImuUartTryGetLatest(struct rscf_saber_topic_data *out);
bool RSCFImuUartReady(void);

#endif /* RSCF_IMU_UART_H_ */
