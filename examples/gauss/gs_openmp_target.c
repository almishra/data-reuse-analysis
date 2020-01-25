#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <omp.h>

#define MAX_ITER 100

// Maximum value of the matrix element
#define MAX 100
#define TOL 0.000001

// Generate a random float number with the maximum value of max
float rand_float(const int max) {
	return ((float)rand() / (float)(RAND_MAX)) * max;
}

// Calculates how many rows are given, as maximum, to each thread
int get_max_rows(const int num_threads, const int n) {
    float val = (n-2) / num_threads + 2;
    int ret = (int)val;
    if(val != ret) ret++;
	return ret;
}

int main(int argc, char *argv[]) {

	if (argc < 2) {
		printf("Call this program with two parameters: matrix_size communication \n");
		printf("\t matrix_size: Add 2 to a power of 2 (e.g. : 18, 1026)\n");
		exit(1);
	}

	const int n = atoi(argv[1]);

	// Start recording the time
	const double i_total_t = omp_get_wtime();

	/*float *mat;
	alloc_matrix(&mat, n, n);*/
    float mat[n][n];
	//const int size = n*n;
	for (int i = 0; i < n; i++)
	    for (int j = 0; j < n; j++)
		    mat[i][j] = rand_float(MAX);

	// Calculate how many cells as maximum per thread
	//const int max_threads = omp_get_max_threads();
	//const int max_rows = get_max_rows(max_threads, n);
	//const int max_cells = max_rows * (n-2);

	// Initial operation time
	const double i_exec_t = omp_get_wtime();

	// Parallelized solver
	//solver(&mat, n, n, max_threads, max_cells);
	float diff;

	int done = 0;
	int cnt_iter = 0;
	const int mat_dim = n * n;
    long total_size = sizeof(int)*n*n;
#ifndef OMP_OFFLOAD_NOREUSE
#pragma omp target data map(to:mat)
#endif
    {
	while (!done && (cnt_iter < MAX_ITER)) {
		diff = 0;

		// Neither the first row nor the last row are solved
		// (that's why both 'i' and 'j' start at 1 and go up to '[nm]-1')
        total_size += sizeof(int);
#ifdef OMP_OFFLOAD_NOREUSE
        total_size += sizeof(int)*n*n;
#endif
		#pragma omp target teams distribute parallel for collapse(2) reduction(+:diff)
		for (int i = 1; i < n-1; i++) {
			for (int j = 1; j < n-1; j++) {

				const float temp = mat[i][j];

				mat[i][j] = 0.2f * (
						mat[i][j]
						+ mat[i][j-1]
						+ mat[i-1][j]
						+ mat[i][j+1]
						+ mat[i+1][j]
					);

                float x = mat[i][j] - temp;
                if(x < 0) x *= -1;
				diff += x;
			}
		}

		if (diff/mat_dim < TOL) {
			done = 1;
		}
		cnt_iter ++;
	}
    }

	printf("Solver converged after %d iterations\n", cnt_iter);
    printf("Total size - %ld\n", total_size);

	// Final operation time
	const double f_exec_t = omp_get_wtime();

	// Finish recording the time
	const double f_total_t = omp_get_wtime();

	const double total_time = f_total_t - i_total_t;
	const double exec_time = f_exec_t - i_exec_t;
	printf("Total time: %lf\n", total_time);
	printf("Operations time: %lf\n", exec_time);

	//write_to_file(n, "static", total_time, exec_time);
}
