CFLAGS = -Wall -O0 -fopenmp
OFFLOAD_FLAG = -fopenmp-targets=nvptx64-nvidia-cuda --cuda-path=$(CUDA_TOOLKIT_ROOT_DIR)
CC=clang

all: mm reuse noreuse reuse6 noreuse6 reuse7 noreuse7

check-env:
#ifndef COMPUTE_CAPABILITY
#    $(error COMPUTE_CAPABILITY is undefined. Please export COMPUTE_CAPABILITY to proper the compute capability of your target device. For example V100 has a compute capability of 7.0. Then export COMPUTE_CAPABILITY=70)
#endif
ifndef CUDA_TOOLKIT_ROOT_DIR
    $(error CUDA_TOOLKIT_ROOT_DIR is undefined. Please set CUDA_TOOLKIT_ROOT_DIR to the install location of cuda.)
endif

mm: mm.c
	$(CC) $(CFLAGS) mm.c -o mm

reuse: mm.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_35 mm.c -o mm_reuse -DOMP_OFFLOAD

noreuse: mm.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_35 mm.c -o mm_noreuse -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE

reuse6: mm.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_60 mm.c -o mm_reuse6 -DOMP_OFFLOAD

noreuse6: mm.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_60 mm.c -o mm_noreuse6 -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE

reuse7: mm.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_70 mm.c -o mm_reuse7 -DOMP_OFFLOAD

noreuse7: mm.c
	$(CC) $(CFLAGS) $(OFFLOAD_FLAG) -Xopenmp-target -march=sm_70 mm.c -o mm_noreuse7 -DOMP_OFFLOAD -DOMP_OFFLOAD_NOREUSE

clean:
	$(RM) mm mm_reuse mm_noreuse mm_reuse6 mm_noreuse6 mm_reuse7 mm_noreuse7
