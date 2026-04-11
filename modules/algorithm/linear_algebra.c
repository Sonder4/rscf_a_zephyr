#include "linear_algebra.h"

static double matrix_abs(double value)
{
    return (value < 0.0) ? -value : value;
}

void matrix_transpose(double *matrix1, double *matrix2, int matrix_row, int matrix_list)
{
    int row;
    int col;

    for (row = 0; row < matrix_row; ++row)
    {
        for (col = 0; col < matrix_list; ++col)
        {
            matrix2[col * matrix_row + row] = matrix1[row * matrix_list + col];
        }
    }
}

void matrix_addition(double *matrix1,
                     int matrix1_row,
                     int matrix1_list,
                     int coefficient1,
                     double *matrix2,
                     int matrix2_row,
                     int matrix2_list,
                     int coefficient2,
                     double *matrix3)
{
    int row;
    int col;

    if ((matrix1_row == 1) && (matrix1_list == 1))
    {
        for (row = 0; row < matrix2_row; ++row)
        {
            for (col = 0; col < matrix2_list; ++col)
            {
                matrix3[row * matrix2_list + col] =
                    (*matrix1) * coefficient1 + matrix2[row * matrix2_list + col] * coefficient2;
            }
        }
        return;
    }

    for (row = 0; row < matrix1_row; ++row)
    {
        for (col = 0; col < matrix1_list; ++col)
        {
            matrix3[row * matrix1_list + col] =
                matrix1[row * matrix1_list + col] * coefficient1 +
                matrix2[row * matrix2_list + col] * coefficient2;
        }
    }
}

void matrix_multiplication(double *matrix1,
                           int matrix1_row,
                           int matrix1_list,
                           double *matrix2,
                           int matrix2_row,
                           int matrix2_list,
                           double *matrix3)
{
    int row;
    int col;
    int inner;

    (void)matrix2_row;
    for (row = 0; row < matrix1_row; ++row)
    {
        for (col = 0; col < matrix2_list; ++col)
        {
            double sum = 0.0;

            for (inner = 0; inner < matrix1_list; ++inner)
            {
                sum += matrix1[row * matrix1_list + inner] * matrix2[inner * matrix2_list + col];
            }
            matrix3[row * matrix2_list + col] = sum;
        }
    }
}

int matrix_inversion(double *matrix1, double *matrix2, double *Identity_matrix, int dimension)
{
    int row;
    int col;
    int pivot;

    for (row = 0; row < dimension; ++row)
    {
        for (col = 0; col < dimension; ++col)
        {
            Identity_matrix[row * dimension + col] = (row == col) ? 1.0 : 0.0;
            matrix2[row * dimension + col] = matrix1[row * dimension + col];
        }
    }

    for (pivot = 0; pivot < dimension; ++pivot)
    {
        double pivot_value = matrix2[pivot * dimension + pivot];

        if (matrix_abs(pivot_value) < 1e-12)
        {
            int swap_row = pivot + 1;

            while (swap_row < dimension)
            {
                if (matrix_abs(matrix2[swap_row * dimension + pivot]) >= 1e-12)
                {
                    for (col = 0; col < dimension; ++col)
                    {
                        double temp = matrix2[pivot * dimension + col];

                        matrix2[pivot * dimension + col] = matrix2[swap_row * dimension + col];
                        matrix2[swap_row * dimension + col] = temp;
                        temp = Identity_matrix[pivot * dimension + col];
                        Identity_matrix[pivot * dimension + col] = Identity_matrix[swap_row * dimension + col];
                        Identity_matrix[swap_row * dimension + col] = temp;
                    }
                    pivot_value = matrix2[pivot * dimension + pivot];
                    break;
                }
                ++swap_row;
            }
        }

        if (matrix_abs(pivot_value) < 1e-12)
        {
            return -1;
        }

        for (col = 0; col < dimension; ++col)
        {
            matrix2[pivot * dimension + col] /= pivot_value;
            Identity_matrix[pivot * dimension + col] /= pivot_value;
        }

        for (row = 0; row < dimension; ++row)
        {
            double factor;

            if (row == pivot)
            {
                continue;
            }

            factor = matrix2[row * dimension + pivot];
            for (col = 0; col < dimension; ++col)
            {
                matrix2[row * dimension + col] -= factor * matrix2[pivot * dimension + col];
                Identity_matrix[row * dimension + col] -= factor * Identity_matrix[pivot * dimension + col];
            }
        }
    }

    for (row = 0; row < dimension; ++row)
    {
        for (col = 0; col < dimension; ++col)
        {
            matrix2[row * dimension + col] = Identity_matrix[row * dimension + col];
        }
    }

    return 1;
}
