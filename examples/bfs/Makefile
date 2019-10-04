# C compiler
CC = clang++
CC_FLAGS = -g -fopenmp -O2
GPU_FLAGS = -fopenmp-targets=nvptx64-nvidia-cuda  -Xopenmp-target -march=sm_$(COMPUTE_CAPABILITY) --cuda-path=$(CUDA_TOOLKIT_ROOT_DIR)

all: check-env bfs bfs_offload bfs_offload_noreuse

check-env:
ifndef COMPUTE_CAPABILITY
    $(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
endif
ifndef CUDA_TOOLKIT_ROOT_DIR
    $(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

bfs: bfs.cpp 
	$(CC) $(CC_FLAGS) bfs.cpp -o bfs 

bfs_offload: bfs.cpp
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -DOMP_OFFLOAD bfs.cpp -o bfs_offload

bfs_offload_noreuse: bfs.cpp
	$(CC) $(CC_FLAGS) $(GPU_FLAGS) -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE bfs.cpp -o bfs_offload_noreuse

input:
	cd inputGen;
	./graphgen 200000000 200M
	mv graph200M.txt ..
	cd ..

clean:
	rm -f bfs bfs_offload bfs_offload_noreuse result.txt
