To build the code goto examples/nw and run
```bash
make
```

To execute the code a run file is added to run for a matrix of size 30000
For Data Reuse
```bash
./needle_offload <max_rows/max_cols> <num_threads>
```

For No data Reuse
```bash
./needle_offload_noreuse <max_rows/max_cols> <num_threads>
```
