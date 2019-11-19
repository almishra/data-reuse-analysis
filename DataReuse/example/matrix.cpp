#include <stdio.h>
#define N 100

int main() {
    double A[N][N], B[N][N], C[N][N], D[N][N], temp[N][N];

    for(int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            A[i][j] = (double)(i*j)/ 10000;
            B[i][j] = (double)(i+j) / 10000;
            C[i][j] = (double)(i*i-j*j) / 10000;
            D[i][j] = 0;
            temp[i][j] = 0;
        }
    }

#pragma omp target teams distribute parallel for
    for(int i=0; i<N; i++)
        for(int j=0; j<N; j++)
            for(int k=0; k<N; k++)
                temp[i][j] += A[i][k]*B[k][j];

#pragma omp target teams distribute parallel for
    for(int i=0; i<N; i++)
        for(int j=0; j<N; j++)
            for(int k=0; k<N; k++)
                D[i][j] += temp[i][k]*C[k][j];

    return 0;
}
