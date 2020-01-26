CC = clang++
CC_FLAGS = -lm
OFFLOAD_CC_FLAGS = -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=${CUDA_TOOLKIT_ROOT_DIR}

all: check-env offload offload_noreuse offload6 offload_noreuse6 offload7 offload_noreuse7

check-env:
#ifndef COMPUTE_CAPABILITY
#    $(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
#endif
ifndef CUDA_TOOLKIT_ROOT_DIR
    $(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

offload: ex_particle_OPENMP_seq.cpp
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_35 ex_particle_OPENMP_seq.cpp -o offload

offload_noreuse: ex_particle_OPENMP_seq.cpp
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_35 -DOMP_OFFLOAD_NOREUSE ex_particle_OPENMP_seq.cpp -o offload_noreuse

offload6: ex_particle_OPENMP_seq.cpp
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_60 ex_particle_OPENMP_seq.cpp -o offload6

offload_noreuse6: ex_particle_OPENMP_seq.cpp
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_60 -DOMP_OFFLOAD_NOREUSE ex_particle_OPENMP_seq.cpp -o offload_noreuse6

offload7: ex_particle_OPENMP_seq.cpp
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_70 ex_particle_OPENMP_seq.cpp -o offload7

offload_noreuse7: ex_particle_OPENMP_seq.cpp
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_70 -DOMP_OFFLOAD_NOREUSE ex_particle_OPENMP_seq.cpp -o offload_noreuse7

clean:
	rm -f result.txt needle offload offload_noreuse offload6 offload_noreuse6 offload7 offload_noreuse7
