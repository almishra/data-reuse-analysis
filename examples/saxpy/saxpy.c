#include <stdio.h>
#include <stdlib.h> 
#include <sys/time.h>

#include <omp.h>


int main() {
    //float *x = new float[N], *y = new float[N];
    const unsigned N = (1 << 27);
    const float XVAL = rand() % 1000000;
    const float YVAL = rand() % 1000000;
    const float AVAL = rand() % 1000000;
    float x[N], y[N];

    long total_size = 0;
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
#ifndef OMP_OFFLOAD_NOREUSE
    total_size += 2*sizeof(int)*N + 3*sizeof(int);
#pragma omp target data map(to:XVAL, YVAL, AVAL) map(alloc:x,y)
    {
#endif
#ifdef OMP_OFFLOAD_NOREUSE
    total_size += 2*sizeof(int)*N + 2*sizeof(int);
#pragma omp target data map(XVAL, YVAL, x, y)
#endif
#pragma omp target teams distribute parallel for
    for (int i = 0; i < N; ++i) {
        x[i] = XVAL;
        y[i] = YVAL;
    }

#ifdef OMP_OFFLOAD_NOREUSE
    total_size += 2*sizeof(int)*N + sizeof(int);
#pragma omp target data map(AVAL, x, y)
#endif
#pragma omp target teams distribute parallel for
    for (int i = 0; i < N; ++i) {
        y[i] += AVAL * x[i];
    }
#ifndef OMP_OFFLOAD_NOREUSE
    }
#endif
    gettimeofday(&tv2, NULL);
    printf("Total size transferred: %lf\n", total_size/1024.0/1024.0);
    printf("Total time: %.3f s\n", (tv2.tv_sec * 1000000 + tv2.tv_usec - 
                            (tv1.tv_sec * 1000000 + tv1.tv_usec)) / (1000000.0));
    return 0;
}

