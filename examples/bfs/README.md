To build the code goto examples/bfs and run
```bash
make
```

To generate the input file goto examples/bfs/inputGen and run
```bash
make
./graphgen <number_of_nodes> <name_of_graph>
```

To execute the code a run file is added to run for a 200Million nodes graph
For Data Reuse
```bash
./bfs_offload <No_of_threads> <graph_file>
```

For No data Reuse
```bash
./bfs_offload_noreuse <No_of_threads> <graph_file>
```
