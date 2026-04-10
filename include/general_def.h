#ifndef GENERAL_DEF_H
#define GENERAL_DEF_H

/* 通用数学常量，供算法与电机模块复用 */
#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifndef TWO_PI
#define TWO_PI (2.0f * PI)
#endif

#ifndef PI2
#define PI2 TWO_PI
#endif

#define RAD_2_DEGREE 57.29577951308232f
#define DEGREE_2_RAD 0.017453292519943295f
#define DEG_PER_RAD RAD_2_DEGREE
#define RAD_PER_DEG DEGREE_2_RAD

#define RPM_2_ANGLE_PER_SEC 6.0f
#define RPM_2_RAD_PER_SEC (TWO_PI / 60.0f)
#define RAD_PER_SEC_2_RPM (60.0f / TWO_PI)

#define MPS2_PER_G 9.80665f

#endif /* GENERAL_DEF_H */
