CC = clang++
ICC = clang++
CC_FLAGS = -g -O3 -fopenmp -Wall
OFFLOAD_CC_FLAGS = -fopenmp-targets=nvptx64-nvidia-cuda  -Xopenmp-target -march=sm_$(COMPUTE_CAPABILITY) --cuda-path=${CUDA_TOOLKIT_ROOT_DIR}

all: check-env needle needle_offload needle_offload_noreuse

check-env:
ifndef COMPUTE_CAPABILITY
    $(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
endif
ifndef CUDA_TOOLKIT_ROOT_DIR
    $(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

needle: needle.cpp
	$(CC) $(CC_FLAGS) needle.cpp -o needle 

needle_offload: needle.cpp
	$(ICC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -DOMP_OFFLOAD needle.cpp -o needle_offload

needle_offload_noreuse: needle.cpp
	$(ICC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE needle.cpp -o needle_offload_noreuse

clean:
	rm -f needle needle_offload needle_offload_noreuse result.txt
