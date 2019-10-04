#include <stdio.h>
#include <omp.h>

extern int omp_num_threads;

#define BS 16

#define AA(_i,_j) a[offset*size+_i*size+_j+offset]
#define BB(_i,_j) a[_i*size+_j]

double total = 0;
void lud_diagonal_omp (float* a, int size, int offset)
{
    int i, j, k;
#ifdef OMP_OFFLOAD
#ifdef OMP_OFFLOAD_NOREUSE
total+=sizeof(float)*size*size;
#pragma omp target data map(size, a[0:size*size])
#endif
#pragma omp target teams distribute parallel for 
#else
#pragma omp parallel for 
#endif
    for (i = 0; i < BS; i++) {

        for (j = i; j < BS; j++) {
            for (k = 0; k < i ; k++) {
                AA(i,j) = AA(i,j) - AA(i,k) * AA(k,j);
            }
        }
   
        float temp = 1.f/AA(i,i);
        for (j = i+1; j < BS; j++) {
            for (k = 0; k < i ; k++) {
                AA(j,i) = AA(j,i) - AA(j,k) * AA(k,i);
            }
            AA(j,i) = AA(j,i)*temp;
        }
    }

}


// implements block LU factorization 
void lud_omp(float *a, int size)
{
    int offset, chunk_idx, size_inter, chunks_in_inter_row, chunks_per_inter;

#ifdef OMP_OFFLOAD
#ifndef OMP_OFFLOAD_NOREUSE
total+=sizeof(float)*size*size;
#pragma omp target data map(to: size) map(a[0:size*size])
#endif
#endif
{
/*    omp_set_num_threads(224);
#else
    printf("running OMP on host\n");
    omp_set_num_threads(omp_num_threads);*/
    for (offset = 0; offset < size - BS ; offset += BS)
    {
        // lu factorization of left-top corner block diagonal matrix 
        //
        lud_diagonal_omp(a, size, offset);
            
        size_inter = size - offset -  BS;
        chunks_in_inter_row  = size_inter/BS;
        
        // calculate perimeter block matrices
        // 
        #ifdef OMP_OFFLOAD
        #ifdef OMP_OFFLOAD_NOREUSE
total+=sizeof(float)*size*size;
        #pragma omp target data map(size, a[0:size*size])
        #endif
        #pragma omp target teams distribute parallel for default(none) \
          private(chunk_idx) shared(size, chunks_per_inter, chunks_in_inter_row, offset, a) 
        #else
        #pragma omp parallel for default(none) \
          private(chunk_idx) shared(size, chunks_per_inter, chunks_in_inter_row, offset, a) 
        #endif
        for ( chunk_idx = 0; chunk_idx < chunks_in_inter_row; chunk_idx++)
        {
            int i, j, k, i_global, j_global, i_here, j_here;
            float sum;           
            float temp[BS*BS] __attribute__ ((aligned (64)));

            for (i = 0; i < BS; i++) {
                #pragma omp simd
                for (j =0; j < BS; j++){
                    temp[i*BS + j] = a[size*(i + offset) + offset + j ];
                }
            }
            i_global = offset;
            j_global = offset;
            
            // processing top perimeter
            //
            j_global += BS * (chunk_idx+1);
            for (j = 0; j < BS; j++) {
                for (i = 0; i < BS; i++) {
                    sum = 0.f;
                    for (k=0; k < i; k++) {
                        sum += temp[BS*i +k] * BB((i_global+k),(j_global+j));
                    }
                    i_here = i_global + i;
                    j_here = j_global + j;
                    BB(i_here, j_here) = BB(i_here,j_here) - sum;
                }
            }

            // processing left perimeter
            //
            j_global = offset;
            i_global += BS * (chunk_idx + 1);
            for (i = 0; i < BS; i++) {
                for (j = 0; j < BS; j++) {
                    sum = 0.f;
                    for (k=0; k < j; k++) {
                        sum += BB((i_global+i),(j_global+k)) * temp[BS*k + j];
                    }
                    i_here = i_global + i;
                    j_here = j_global + j;
                    a[size*i_here + j_here] = ( a[size*i_here+j_here] - sum ) / a[size*(offset+j) + offset+j];
                }
            }

        }
        
        // update interior block matrices
        //
        chunks_per_inter = chunks_in_inter_row*chunks_in_inter_row;

        #ifdef OMP_OFFLOAD
        #ifdef OMP_OFFLOAD_NOREUSE
total+=sizeof(float)*size*size;
        #pragma omp target data map(size, a[0:size*size])
        #endif
        #pragma omp target teams distribute parallel for schedule(auto) default(none) \
                private(chunk_idx ) shared(size, chunks_per_inter, chunks_in_inter_row, offset, a) 
        #else
        #pragma omp parallel for schedule(auto) default(none) \
                private(chunk_idx ) shared(size, chunks_per_inter, chunks_in_inter_row, offset, a) 
        #endif
        for  (chunk_idx =0; chunk_idx < chunks_per_inter; chunk_idx++)
        {
            int i, j, k, i_global, j_global;
            float temp_top[BS*BS] __attribute__ ((aligned (64)));
            float temp_left[BS*BS] __attribute__ ((aligned (64)));
            float sum[BS] __attribute__ ((aligned (64))) = {0.f};
            
            i_global = offset + BS * (1 +  chunk_idx/chunks_in_inter_row);
            j_global = offset + BS * (1 + chunk_idx%chunks_in_inter_row);

            for (i = 0; i < BS; i++) {
#pragma omp simd
                for (j =0; j < BS; j++){
                    temp_top[i*BS + j]  = a[size*(i + offset) + j + j_global ];
                    temp_left[i*BS + j] = a[size*(i + i_global) + offset + j];
                }
            }

            for (i = 0; i < BS; i++)
            {
                for (k=0; k < BS; k++) {
#pragma omp simd 
                    for (j = 0; j < BS; j++) {
                        sum[j] += temp_left[BS*i + k] * temp_top[BS*k + j];
                    }
                }
#pragma omp simd 
                for (j = 0; j < BS; j++) {
                    BB((i+i_global),(j+j_global)) -= sum[j];
                    sum[j] = 0.f;
                }
            }
        }
    }

    lud_diagonal_omp(a, size, offset);
}

    printf("Total size = %lf\n", total);
}
