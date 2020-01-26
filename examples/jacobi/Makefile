CFLAGS = -Wall -O3 -fopenmp 
OFFLOAD_FLAG = -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=$(CUDA_TOOLKIT_ROOT_DIR)
CC=clang

all: check-env jacobi reuse noreuse reuse6 noreuse6 reuse7 noreuse7

check-env:
#ifndef COMPUTE_CAPABILITY
#	$(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
#endif
ifndef CUDA_TOOLKIT_ROOT_DIR
	$(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

jacobi: jacobi.c
	$(CC) $(CFLAGS) jacobi.c -o jacobi

reuse: jacobi.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_35 jacobi.c -o jacobi_reuse -DOMP_OFFLOAD

noreuse: jacobi.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_35 jacobi.c -o jacobi_noreuse -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE

reuse6: jacobi.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_60 jacobi.c -o jacobi_reuse6 -DOMP_OFFLOAD

noreuse6: jacobi.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_60 jacobi.c -o jacobi_noreuse6 -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE

reuse7: jacobi.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_70 jacobi.c -o jacobi_reuse7 -DOMP_OFFLOAD

noreuse7: jacobi.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_70 jacobi.c -o jacobi_noreuse7 -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE

clean:
	$(RM) jacobi jacobi_reuse jacobi_noreuse jacobi_reuse6 jacobi_noreuse6 jacobi_reuse7 jacobi_noreuse7
