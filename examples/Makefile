CFLAGS = -Wall -O0
OMPFLAG = -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=$(CUDA_TOOLKIT_ROOT_DIR) -Xopenmp-target -march=sm_$(COMPUTE_CAPABILITY)
CC=clang

all: check-env jacobi bfs lud mm nw

check-env:
ifndef COMPUTE_CAPABILITY
    $(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
endif
ifndef CUDA_TOOLKIT_ROOT_DIR
    $(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

jacobi: jacobi/jacobi.c
	cd jacobi; make

bfs:
	cd bfs; make

lud: lud/lud_omp_offload.c
	cd lud; make

mm:
	cd MatrixMultiplication; make

nw: nw/needle.cpp
	cd nw; make

clean:
	cd jacobi; make clean; cd ..
	cd bfs; make clean; cd ..
	cd lud; make clean; cd ..
	cd mm; make clean; cd ..
	cd nw; make clean; cd ..
