/**
 * This is a parallel code with collapsed loops
 */

#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <sys/time.h>

void initialize(int n, int m, double alpha, double f[m][n])
{
    int i, j;
    /* Initilize initial condition*/
    for (i=0; i<m; i++){
        for (j=0; j<n; j++){
            f[j][i] = (rand()%3) * alpha;
        }
    }
}

int main(int argc, char**argv) {
    int iter_max = 5000;
    int iter;
    double err = 1.0;
    double tol = 0.0000000001;
    int m = (argc > 1 ? atoi(argv[1]) : 500);
    int n = (argc > 1 ? atoi(argv[1]) : 500);
    double A[m][n];
    double Anew[m][n];
    double alpha = 0.0543;
    initialize(n, m, alpha, A);

    int i, j;
    struct timeval t1, t2;
    double total_in = 0;
    double total_out = 0;

    gettimeofday(&t1, NULL);
    iter = 0;

#ifdef OMP_OFFLOAD
#ifndef OMP_OFFLOAD_NOREUSE
    total_in += sizeof(double)*m*n;
    total_out += sizeof(double)*m*n;
    #pragma omp target data map(alloc:Anew) map(A)
#endif
#endif
    while (err>tol && iter<iter_max) {
        err=0.0;
#ifdef OMP_OFFLOAD
#ifdef OMP_OFFLOAD_NOREUSE
        total_in += 2*sizeof(double)*m*n + sizeof(double);
        total_out += 2*sizeof(double)*m*n + sizeof(double);
        #pragma omp target data map(err, Anew, A)
#endif
        #pragma omp target teams distribute reduction(max:err)
#else
        #pragma omp parallel for reduction(max:err)
#endif
        for(i=1; i<n-1; i++) {
            #pragma omp parallel for reduction(max:err)
            for( j = 1; j < m - 1; j++) {
                Anew[i][j] = 0.25 * (A[i][j+1] + A[i][j-1] + A[i-1][j] + A[i+1][j]);
                double val = Anew[i][j] - A[i][j]; 
                if(val < 0) val *= -1;
                if(err < val)
                    err = val;
            }
        }

#ifdef OMP_OFFLOAD
#ifdef OMP_OFFLOAD_NOREUSE
        total_in += 2*sizeof(double)*m*n + sizeof(double);
        total_out += 2*sizeof(double)*m*n + sizeof(double);
        #pragma omp target data map(Anew, A)
#endif
        #pragma omp target teams distribute parallel for collapse(2)
#else
        #pragma omp parallel for collapse(2)
#endif
        for(i=1; i<n-1; i++) {
            for(j=1; j<m-1; j++) {
                A[i][j] = Anew[i][j];      
            }
        }
        iter++;
    }
    gettimeofday(&t2, NULL);
    double runtime = (t2.tv_sec - t1.tv_sec);
    runtime += (t2.tv_usec - t1.tv_usec) / 1000000.0;

    printf("%.3f %d\n", runtime, iter);

    printf("Total memory transfer in = %lf\n", total_in);
    printf("Total memory transfer out = %lf\n", total_out);
    return 0;
}
