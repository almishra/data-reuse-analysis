To build the code goto examples/lud and run
```bash
make
```

To execute the code a run file is added to run for a matrix of size 20000
For Data Reuse
```bash
./lud_omp_offload -s <matrix_size>
```

For No data Reuse
```bash
./lud_omp_offload_noreuse -s <matrix_size>
```
