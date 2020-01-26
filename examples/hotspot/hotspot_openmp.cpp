#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>

// Returns the current system time in microseconds
long long get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000) + tv.tv_usec;

}

//using namespace std;

#define BLOCK_SIZE 16
#define BLOCK_SIZE_C BLOCK_SIZE
#define BLOCK_SIZE_R BLOCK_SIZE

#define STR_SIZE	256

/* maximum power density possible (say 300W for a 10mm x 10mm chip)	*/
#define MAX_PD	(3.0e6)
/* required precision in degrees	*/
#define PRECISION	0.001
#define SPEC_HEAT_SI 1.75e6
#define K_SI 100
/* capacitance fitting factor	*/
#define FACTOR_CHIP	0.5
#define OPEN

typedef float FLOAT;

/* chip parameters	*/
const FLOAT t_chip = 0.0005;
const FLOAT chip_height = 0.016;
const FLOAT chip_width = 0.016;

/* ambient temperature, assuming no package at all	*/
const FLOAT amb_temp = 80.0;

void fatal(char *s)
{
    fprintf(stderr, "error: %s\n", s);
    exit(1);
}

void writeoutput(FLOAT *vect, int grid_rows, int grid_cols, char *file) {

    int i,j, index=0;
    FILE *fp;
    char str[STR_SIZE];

    if( (fp = fopen(file, "w" )) == 0 )
        printf( "The file was not opened\n" );


    for (i=0; i < grid_rows; i++)
        for (j=0; j < grid_cols; j++)
        {

            sprintf(str, "%d\t%g\n", index, vect[i*grid_cols+j]);
            fputs(str,fp);
            index++;
        }

    fclose(fp);	
}

void read_input(FLOAT *vect, int grid_rows, int grid_cols, char *file)
{
    int i, index;
    FILE *fp;
    char str[STR_SIZE];
    FLOAT val;

    fp = fopen (file, "r");
    if (!fp)
        fatal ((char*)"file could not be opened for reading");

    for (i=0; i < grid_rows * grid_cols; i++) {
        fgets(str, STR_SIZE, fp);
        if (feof(fp))
            fatal((char*)"not enough lines in file");
        if ((sscanf(str, "%f", &val) != 1) )
            fatal((char*)"invalid file format");
        vect[i] = val;
    }

    fclose(fp);	
}

void usage(int argc, char **argv)
{
    fprintf(stderr, "Usage: %s <grid_rows> <grid_cols> <sim_time> ", argv[0]);
    fprintf(stderr, "<no. of threads><temp_file> <power_file>\n");
    fprintf(stderr, "\t<grid_rows>  - number of rows in the grid ");
    fprintf(stderr, "(positive integer)\n");
    fprintf(stderr, "\t<grid_cols>  - number of columns in the grid ");
    fprintf(stderr, "(positive integer)\n");
    fprintf(stderr, "\t<sim_time>   - number of iterations\n");
    fprintf(stderr, "\t<no. of threads>   - number of threads\n");
    fprintf(stderr, "\t<temp_file>  - name of the file containing the initial ");
    fprintf(stderr, "temperature values of each cell\n");
    fprintf(stderr, "\t<power_file> - name of the file containing the ");
    fprintf(stderr, "dissipated power values of each cell\n");
    fprintf(stderr, "\t<output_file> - name of the output file\n");
    exit(1);
}

int main(int argc, char **argv)
{
    int grid_rows, grid_cols, sim_time, i;
    char *tfile, *pfile, *ofile;
    int num_omp_threads;

    /* check validity of inputs	*/
    if (argc != 8)
        usage(argc, argv);
    if ((grid_rows = atoi(argv[1])) <= 0 ||
            (grid_cols = atoi(argv[2])) <= 0 ||
            (sim_time = atoi(argv[3])) <= 0 ||
            (num_omp_threads = atoi(argv[4])) <= 0
       )
        usage(argc, argv);

    FLOAT temp[grid_rows*grid_cols], power[grid_rows*grid_cols];
    FLOAT result[grid_rows*grid_cols];

    /* read initial temperatures and input power	*/
    tfile = argv[5];
    pfile = argv[6];
    ofile = argv[7];

    read_input(temp, grid_rows, grid_cols, tfile);
    read_input(power, grid_rows, grid_cols, pfile);

    printf("Start computing the transient temperature\n");

    long long start_time = get_time();

    FLOAT grid_height = chip_height / grid_rows;
    FLOAT grid_width = chip_width / grid_cols;

    FLOAT Cap = FACTOR_CHIP * SPEC_HEAT_SI * t_chip * grid_width * grid_height;
    FLOAT Rx = grid_width / (2.0 * K_SI * t_chip * grid_height);
    FLOAT Ry = grid_height / (2.0 * K_SI * t_chip * grid_width);
    FLOAT Rz = t_chip / (K_SI * grid_height * grid_width);

    FLOAT max_slope = MAX_PD / (FACTOR_CHIP * t_chip * SPEC_HEAT_SI);
    FLOAT step = PRECISION / max_slope / 1000.0;

    FLOAT Rx_1=1.f/Rx;
    FLOAT Ry_1=1.f/Ry;
    FLOAT Rz_1=1.f/Rz;
    FLOAT Cap_1 = step/Cap;

    long total_size = 0;
#ifndef OMP_OFFLOAD_NOREUSE
    total_size += 3*sizeof(float)*grid_rows*grid_cols +
                    2*sizeof(int) + 5*sizeof(float);
#pragma omp target data \
    map(temp)\
    map(to: power, grid_rows, grid_cols, Cap_1, Rx_1, Ry_1, Rz_1, step ) \
    map(result)
#endif
    {
        for (int i = 0; i < sim_time; i++)
        {
            FLOAT delta;
            int r, c;
            int chunk;
            int num_chunk = grid_rows*grid_cols / (BLOCK_SIZE_R * BLOCK_SIZE_C);
            int chunks_in_row = grid_cols/BLOCK_SIZE_C;
            int chunks_in_col = grid_rows/BLOCK_SIZE_R;

#ifdef OMP_OFFLOAD_NOREUSE
            total_size += 3*sizeof(float)*grid_rows*grid_cols +
                            2*sizeof(int) + 5*sizeof(float);
            #pragma omp target data \
                map(temp, power, grid_rows, grid_cols, Cap_1, Rx_1, Ry_1, Rz_1)\
                map(step, result)
#endif
            #pragma omp target teams distribute parallel for \
                shared(power, temp, result) private(chunk, r, c, delta) \
                firstprivate(grid_rows, grid_cols, num_chunk, chunks_in_row) \
                schedule(static)
            for ( chunk = 0; chunk < num_chunk; ++chunk )
            {
                int r_start = BLOCK_SIZE_R*(chunk/chunks_in_col);
                int c_start = BLOCK_SIZE_C*(chunk%chunks_in_row);
                int r_end = r_start + BLOCK_SIZE_R > grid_rows ? grid_rows 
                                                       : r_start + BLOCK_SIZE_R;
                int c_end = c_start + BLOCK_SIZE_C > grid_cols ? grid_cols 
                                                       : c_start + BLOCK_SIZE_C;

                if ( r_start == 0 || c_start == 0 || r_end == grid_rows 
                                                         || c_end == grid_cols )
                {
                    for ( r = r_start; r < r_start + BLOCK_SIZE_R; ++r ) {
                        for ( c = c_start; c < c_start + BLOCK_SIZE_C; ++c ) {
                            /* Corner 1 */
                            if ( (r == 0) && (c == 0) ) {
                                delta = (Cap_1) * (power[0] +
                                        (temp[1] - temp[0]) * Rx_1 +
                                        (temp[grid_cols] - temp[0]) * Ry_1 +
                                        (amb_temp - temp[0]) * Rz_1);
                            }	/* Corner 2 */
                            else if ((r == 0) && (c == grid_cols-1)) {
                                delta = (Cap_1) * (power[c] +
                                        (temp[c-1] - temp[c]) * Rx_1 +
                                        (temp[c+grid_cols] - temp[c]) * Ry_1 +
                                        (   amb_temp - temp[c]) * Rz_1);
                            }	/* Corner 3 */
                            else if ((r == grid_rows-1) && (c == grid_cols-1)) {
                                delta = (Cap_1) * (power[r*grid_cols+c] +
                                        (temp[r*grid_cols+c-1] - temp[r*grid_cols+c]) * Rx_1 +
                                        (temp[(r-1)*grid_cols+c] - temp[r*grid_cols+c]) * Ry_1 +
                                        (amb_temp - temp[r*grid_cols+c]) * Rz_1);					
                            }	/* Corner 4	*/
                            else if ((r == grid_rows-1) && (c == 0)) {
                                delta = (Cap_1) * (power[r*grid_cols] +
                                        (temp[r*grid_cols+1] - temp[r*grid_cols]) * Rx_1 +
                                        (temp[(r-1)*grid_cols] - temp[r*grid_cols]) * Ry_1 +
                                        (amb_temp - temp[r*grid_cols]) * Rz_1);
                            }	/* Edge 1 */
                            else if (r == 0) {
                                delta = (Cap_1) * (power[c] +
                                        (temp[c+1] + temp[c-1] - 2.0*temp[c]) * Rx_1 +
                                        (temp[grid_cols+c] - temp[c]) * Ry_1 +
                                        (amb_temp - temp[c]) * Rz_1);
                            }	/* Edge 2 */
                            else if (c == grid_cols-1) {
                                delta = (Cap_1) * (power[r*grid_cols+c] +
                                        (temp[(r+1)*grid_cols+c] + temp[(r-1)*grid_cols+c] - 2.0*temp[r*grid_cols+c]) * Ry_1 +
                                        (temp[r*grid_cols+c-1] - temp[r*grid_cols+c]) * Rx_1 +
                                        (amb_temp - temp[r*grid_cols+c]) * Rz_1);
                            }	/* Edge 3 */
                            else if (r == grid_rows-1) {
                                delta = (Cap_1) * (power[r*grid_cols+c] +
                                        (temp[r*grid_cols+c+1] + temp[r*grid_cols+c-1] - 2.0*temp[r*grid_cols+c]) * Rx_1 +
                                        (temp[(r-1)*grid_cols+c] - temp[r*grid_cols+c]) * Ry_1 +
                                        (amb_temp - temp[r*grid_cols+c]) * Rz_1);
                            }	/* Edge 4 */
                            else if (c == 0) {
                                delta = (Cap_1) * (power[r*grid_cols] +
                                        (temp[(r+1)*grid_cols] + temp[(r-1)*grid_cols] - 2.0*temp[r*grid_cols]) * Ry_1 +
                                        (temp[r*grid_cols+1] - temp[r*grid_cols]) * Rx_1 +
                                        (amb_temp - temp[r*grid_cols]) * Rz_1);
                            }
                            result[r*grid_cols+c] = temp[r*grid_cols+c]+ delta;
                        }
                    }
                    continue;
                }

                for ( r = r_start; r < r_start + BLOCK_SIZE_R; ++r ) {
                    //#pragma omp simd
                    for ( c = c_start; c < c_start + BLOCK_SIZE_C; ++c ) {
                        /* Update Temperatures */
                        result[r*grid_cols+c] =temp[r*grid_cols+c]+
                            ( Cap_1 * (power[r*grid_cols+c] +
                                       (temp[(r+1)*grid_cols+c] + temp[(r-1)*grid_cols+c] - 2.f*temp[r*grid_cols+c]) * Ry_1 +
                                       (temp[r*grid_cols+c+1] + temp[r*grid_cols+c-1] - 2.f*temp[r*grid_cols+c]) * Rx_1 +
                                       (amb_temp - temp[r*grid_cols+c]) * Rz_1));
                    }
                }
            }

#ifdef OMP_OFFLOAD_NOREUSE
            total_size += 2*sizeof(float)*grid_rows*grid_cols;
            #pragma omp target data map(temp, result)
#endif
            #pragma omp target teams distribute parallel for 
            for(int i=0; i<grid_rows*grid_cols; i++)
                temp[i] = result[i];
        }
    }

    long long end_time = get_time();

    printf("Ending simulation\n");
    printf("Total size transferred = %lf MB\n", total_size / 1024.0 / 1024.0);
    printf("Total time: %.3f seconds\n", ((float) (end_time - start_time)) / (1000*1000));

    // writeoutput((1&sim_time) ? result : temp, grid_rows, grid_cols, ofile);

    return 0;
}
