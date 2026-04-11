#include "coordinate_system.h"

#include <stddef.h>

#include "general_def.h"

void rotation_of_coordinate_system(float x,
                                   float y,
                                   float z,
                                   float *target_x,
                                   float *target_y,
                                   float *target_z,
                                   float yaw,
                                   float pitch,
                                   float roll,
                                   int sign)
{
    float x1;
    float y1;
    float z1;
    float r = (float)sign * roll * RAD_PER_DEG;
    float p = (float)sign * pitch * RAD_PER_DEG;
    float yw = (float)sign * yaw * RAD_PER_DEG;

    x1 = x * arm_cos_f32(yw) + y * arm_sin_f32(yw);
    y1 = -x * arm_sin_f32(yw) + y * arm_cos_f32(yw);
    z1 = z;

    x = x1;
    y = y1 * arm_cos_f32(p) + z1 * arm_sin_f32(p);
    z = -y1 * arm_sin_f32(p) + z1 * arm_cos_f32(p);

    x1 = x * arm_cos_f32(r) - z * arm_sin_f32(r);
    z1 = x * arm_sin_f32(r) + z * arm_cos_f32(r);

    if (target_x != NULL)
    {
        *target_x = x1;
    }
    if (target_y != NULL)
    {
        *target_y = y;
    }
    if (target_z != NULL)
    {
        *target_z = z1;
    }
}

float convert_angle_system(float angle_3oclock)
{
    float target = 90.0f - angle_3oclock;

    while (target > 180.0f)
    {
        target -= 360.0f;
    }
    while (target <= -180.0f)
    {
        target += 360.0f;
    }

    return target;
}
