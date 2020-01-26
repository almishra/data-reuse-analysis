CC = clang
CC_FLAGS = -g -O3 -fopenmp -Wall
OFFLOAD_CC_FLAGS = -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=${CUDA_TOOLKIT_ROOT_DIR}

all: check-env nn nn_offload nn_offload_noreuse nn_offload6 nn_offload_noreuse6 nn_offload7 nn_offload_noreuse7

check-env:
#ifndef COMPUTE_CAPABILITY
#    $(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
#endif
ifndef CUDA_TOOLKIT_ROOT_DIR
    $(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

nn: nn_openmp.c
	$(CC) $(CC_FLAGS) nn_openmp.c -o nn 

nn_offload: nn_openmp.c
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_35 -DOMP_OFFLOAD nn_openmp.c -o nn_offload

nn_offload_noreuse: nn_openmp.c
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_35  -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE nn_openmp.c -o nn_offload_noreuse

nn_offload6: nn_openmp.c
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_60 -DOMP_OFFLOAD nn_openmp.c -o nn_offload6

nn_offload_noreuse6: nn_openmp.c
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_60  -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE nn_openmp.c -o nn_offload_noreuse6

nn_offload7: nn_openmp.c
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_70 -DOMP_OFFLOAD nn_openmp.c -o nn_offload7

nn_offload_noreuse7: nn_openmp.c
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_70  -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE nn_openmp.c -o nn_offload_noreuse7

clean:
	rm -f result.txt nn nn_offload nn_offload_noreuse nn_offload6 nn_offload_noreuse6 nn_offload7 nn_offload_noreuse7
