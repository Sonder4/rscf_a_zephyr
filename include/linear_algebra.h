#ifndef LINEAR_ALGEBRA_H_
#define LINEAR_ALGEBRA_H_

void matrix_transpose(double *matrix1, double *matrix2, int matrix_row, int matrix_list);
void matrix_addition(
    double *matrix1,
    int matrix1_row,
    int matrix1_list,
    int coefficient1,
    double *matrix2,
    int matrix2_row,
    int matrix2_list,
    int coefficient2,
    double *matrix3
);
void matrix_multiplication(
    double *matrix1,
    int matrix1_row,
    int matrix1_list,
    double *matrix2,
    int matrix2_row,
    int matrix2_list,
    double *matrix3
);
int matrix_inversion(double *matrix1, double *matrix2, double *Identity_matrix, int dimension);

#endif /* LINEAR_ALGEBRA_H_ */
