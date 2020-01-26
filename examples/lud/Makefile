CC = clang
CC_FLAGS =  -fopenmp -Wall
GPU_FLAGS = -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=$(CUDA_TOOLKIT_ROOT_DIR) -DOMP_OFFLOAD

all: check-env lud common offload noreuse offload6 noreuse6 offload7 noreuse7

check-env:
#ifndef COMPUTE_CAPABILITY
#	$(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
#endif
ifndef CUDA_TOOLKIT_ROOT_DIR
	$(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif


offload: lud common lud_omp_offload.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35 -c lud_omp_offload.c -o lud_omp_offload.o
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35 -o lud_omp_offload lud.o lud_omp_offload.o common.o -lm

noreuse: lud common lud_omp_offload.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35 -DOMP_OFFLOAD_NOREUSE -c lud_omp_offload.c -o lud_omp_offload_noreuse.o
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35 -o lud_omp_offload_noreuse lud.o lud_omp_offload_noreuse.o common.o -lm

offload6: lud6 common6 lud_omp_offload.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60 -c lud_omp_offload.c -o lud_omp_offload6.o
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60 -o lud_omp_offload6 lud6.o lud_omp_offload6.o common6.o -lm

noreuse6: lud6 common6 lud_omp_offload.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60 -DOMP_OFFLOAD_NOREUSE -c lud_omp_offload.c -o lud_omp_offload_noreuse6.o
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60 -o lud_omp_offload_noreuse6 lud6.o lud_omp_offload_noreuse6.o common6.o -lm

offload7: lud7 common7 lud_omp_offload.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70 -c lud_omp_offload.c -o lud_omp_offload7.o
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70 -o lud_omp_offload7 lud7.o lud_omp_offload7.o common7.o -lm

noreuse7: lud7 common7 lud_omp_offload.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70 -DOMP_OFFLOAD_NOREUSE -c lud_omp_offload.c -o lud_omp_offload_noreuse7.o
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70 -o lud_omp_offload_noreuse7 lud7.o lud_omp_offload_noreuse7.o common7.o -lm

lud: lud.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35 -c -I../common lud.c -o lud.o

lud6: lud.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60 -c -I../common lud.c -o lud6.o

lud7: lud.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70 -c -I../common lud.c -o lud7.o

common: common.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35  -c common.c -o common.o

common6: common.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60  -c common.c -o common6.o

common7: common.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70  -c common.c -o common7.o

clean:
	$(RM) lud_omp_offload_noreuse lud_omp_offload lud_omp_offload_noreuse6 lud_omp_offload6  lud_omp_offload_noreuse7 lud_omp_offload7  *.o *~

