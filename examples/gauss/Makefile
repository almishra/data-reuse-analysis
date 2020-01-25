CC = clang
CC_FLAGS =  -fopenmp -Wall
GPU_FLAGS = -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=$(CUDA_TOOLKIT_ROOT_DIR) -DOMP_OFFLOAD

all: check-env offload offload_reuse offload6 offload_reuse6 offload7 offload_reuse7

check-env:
ifndef CUDA_TOOLKIT_ROOT_DIR
	$(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

offload: gs_openmp_target.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35 -DOMP_OFFLOAD_NOREUSE -o gs_offload_noreuse gs_openmp_target.c -lm

offload_reuse: gs_openmp_target.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_35 -o gs_offload gs_openmp_target.c -lm

offload6: gs_openmp_target.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60 -DOMP_OFFLOAD_NOREUSE -o gs_offload_noreuse6 gs_openmp_target.c -lm

offload_reuse6: gs_openmp_target.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_60 -o gs_offload6 gs_openmp_target.c -lm

offload7: gs_openmp_target.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70 -DOMP_OFFLOAD_NOREUSE -o gs_offload_noreuse7 gs_openmp_target.c -lm

offload_reuse7: gs_openmp_target.c
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -Xopenmp-target -march=sm_70 -o gs_offload7 gs_openmp_target.c -lm

clean:
	$(RM) gs_offload_noreuse gs_offload gs_offload_noreuse6 gs_offload6 gs_offload_noreuse7 gs_offload7

