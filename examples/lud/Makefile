CC = clang
CC_FLAGS =  -fopenmp -Wall
GPU_FLAGS = -fopenmp-targets=nvptx64-nvidia-cuda  -Xopenmp-target -march=sm_$(COMPUTE_CAPABILITY) --cuda-path=$(CUDA_TOOLKIT_ROOT_DIR) -DOMP_OFFLOAD

all: check-env lud common offload noreuse

check-env:
ifndef COMPUTE_CAPABILITY
	$(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
endif
ifndef CUDA_TOOLKIT_ROOT_DIR
	$(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif


offload: lud common lud_omp_offload.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -c lud_omp_offload.c -o lud_omp_offload.o
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -o lud_omp_offload lud.o lud_omp_offload.o common.o -lm

noreuse: lud common lud_omp_offload.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -DOMP_OFFLOAD_NOREUSE -c lud_omp_offload.c -o lud_omp_offload_noreuse.o
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -o lud_omp_offload_noreuse lud.o lud_omp_offload_noreuse.o common.o -lm

lud: lud.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -c -I../common lud.c -o lud.o

common: common.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -c common.c -o common.o

clean:
	$(RM) lud_omp_offload_noreuse lud_omp_offload lud.o lud_omp_offload.o lud_omp_offload_noreuse.o common.o *~

