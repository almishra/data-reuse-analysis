CC = clang
CC_FLAGS =  -fopenmp -Wall
GPU_FLAGS = -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=$(CUDA_TOOLKIT_ROOT_DIR) -DOMP_OFFLOAD

all: check-env offload offload_noreuse offload6 offload_noreuse6 offload7 offload_noreuse7

check-env:
#ifndef COMPUTE_CAPABILITY
#	$(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
#endif
ifndef CUDA_TOOLKIT_ROOT_DIR
	$(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

offload: saxpy.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35 -o saxpy saxpy.c

offload_noreuse: saxpy.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35 -DOMP_OFFLOAD_NOREUSE -o saxpy_noreuse saxpy.c

offload6: saxpy.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60 -o saxpy6 saxpy.c

offload_noreuse6: saxpy.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60 -DOMP_OFFLOAD_NOREUSE -o saxpy_noreuse6 saxpy.c

offload7: saxpy.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70 -o saxpy7 saxpy.c

offload_noreuse7: saxpy.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70 -DOMP_OFFLOAD_NOREUSE -o saxpy_noreuse7 saxpy.c

clean:
	$(RM) saxpy saxpy_noreuse saxpy6 saxpy_noreuse6 saxpy7 saxpy_noreuse7

