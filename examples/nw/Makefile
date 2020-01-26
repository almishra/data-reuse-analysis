CC = clang++
ICC = clang++
CC_FLAGS = -g -O3 -fopenmp -Wall
OFFLOAD_CC_FLAGS = -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=${CUDA_TOOLKIT_ROOT_DIR}

all: check-env needle needle_offload needle_offload_noreuse needle_offload6 needle_offload_noreuse6 needle_offload7 needle_offload_noreuse7

check-env:
#ifndef COMPUTE_CAPABILITY
#    $(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
#endif
ifndef CUDA_TOOLKIT_ROOT_DIR
    $(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

needle: needle.cpp
	$(CC) $(CC_FLAGS) needle.cpp -o needle 

needle_offload: needle.cpp
	$(ICC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_35 -DOMP_OFFLOAD needle.cpp -o needle_offload

needle_offload_noreuse: needle.cpp
	$(ICC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_35  -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE needle.cpp -o needle_offload_noreuse

needle_offload6: needle.cpp
	$(ICC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_60 -DOMP_OFFLOAD needle.cpp -o needle_offload6

needle_offload_noreuse6: needle.cpp
	$(ICC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_60  -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE needle.cpp -o needle_offload_noreuse6

needle_offload7: needle.cpp
	$(ICC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_70 -DOMP_OFFLOAD needle.cpp -o needle_offload7

needle_offload_noreuse7: needle.cpp
	$(ICC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_70  -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE needle.cpp -o needle_offload_noreuse7

clean:
	rm -f result.txt needle needle_offload needle_offload_noreuse needle_offload6 needle_offload_noreuse6 needle_offload7 needle_offload_noreuse7
