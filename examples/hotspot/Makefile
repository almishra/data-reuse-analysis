# C compiler
CC = clang
CC_FLAGS = #-g -fopenmp -O2
OFFLOAD_CC_FLAGS = -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=${CUDA_TOOLKIT_ROOT_DIR}

all: hotspot_reuse hotspot_offload hotspot_reuse6 hotspot_offload6 hotspot_reuse7 hotspot_offload7

hotspot_reuse: hotspot_openmp.cpp 
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_35 hotspot_openmp.cpp -o hotspot_reuse

hotspot_offload: hotspot_openmp.cpp 
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_35 -DOMP_OFFLOAD_NOREUSE hotspot_openmp.cpp -o hotspot_noreuse

hotspot_reuse6: hotspot_openmp.cpp 
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_60 hotspot_openmp.cpp -o hotspot_reuse6

hotspot_offload6: hotspot_openmp.cpp 
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_60 -DOMP_OFFLOAD_NOREUSE hotspot_openmp.cpp -o hotspot_noreuse6

hotspot_reuse7: hotspot_openmp.cpp 
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_70 hotspot_openmp.cpp -o hotspot_reuse7

hotspot_offload7: hotspot_openmp.cpp 
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -Xopenmp-target -march=sm_70 -DOMP_OFFLOAD_NOREUSE hotspot_openmp.cpp -o hotspot_noreuse7

clean:
	rm -f hotspot_reuse hotspot_noreuse hotspot_reuse6 hotspot_noreuse6  hotspot_reuse7 hotspot_noreuse7
