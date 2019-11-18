/**
 * This is a parallel code with collapsed loops
 */

#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char**argv) {
    int iter_max = 5000;
    int iter;
    double err = 1.0;
    double tol = 0.0000000001;
    double A[500*500];
    double Anew[500*500];
    double alpha = 0.0543;

    for (int i=0; i<500; i++){
        for (int j=0; j<500; j++){
            A[i*500 + j] = (rand()%3) * alpha;
        }
    }

    iter = 0;

    while (err>tol && iter<iter_max) {
        err=0.0;
        #pragma omp target teams distribute parallel for reduction(max:err) collapse(2)
        for(int i=1; i<500-1; i++) {
            for(int j = 1; j < 500 - 1; j++) {
                Anew[i*500 + j] = 0.25 * (A[i*500+j+1] + A[i*500+j-1] + A[(i-1)*500+j] + A[(i+1)*500+j]);
                if(err < fabs(Anew[i*500+j] - A[i*500+j]))
                    err = fabs(Anew[i*500+j] - A[i*500+j]);
            }
        }

        #pragma omp target teams distribute parallel for collapse(2)
        for(int i=1; i<500-1; i++) {
            for(int j=1; j<500-1; j++) {
                A[i*500+j] = Anew[i*500+j];      
            }
        }
        iter++;
    }
    return 0;
}
