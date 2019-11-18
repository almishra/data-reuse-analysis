#include <stdio.h>
#define N 100

int main() {
    int A[N][N], B[N][N], C[N][N], D[N][N], temp[N][N];

#pragma omp target teams distribute parallel for collapse(2)
    for(int i=0; i<N; i++)
        for(int j=0; j<N; j++)
            for(int k=0; k<N; k++)
                temp[i][j] += A[i][k]*B[k][j];

#pragma omp target teams distribute parallel for collapse(2)
    for(int i=0; i<N; i++)
        for(int j=0; j<N; j++)
            for(int k=0; k<N; k++)
                D[i][j] += temp[i][k]*C[k][j];

    return 0;
}
