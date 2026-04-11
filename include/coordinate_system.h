#ifndef COORDINATE_SYSTEM_H_
#define COORDINATE_SYSTEM_H_

#include "arm_math.h"

void rotation_of_coordinate_system(
    float x,
    float y,
    float z,
    float *target_x,
    float *target_y,
    float *target_z,
    float yaw,
    float pitch,
    float roll,
    int sign
);
float convert_angle_system(float angle_3oclock);

#endif /* COORDINATE_SYSTEM_H_ */
