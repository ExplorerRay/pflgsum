# pflgsum
Parallel Postfix log summarizer, simply implement some basic features of `pflogsumm` in parallel.

## Dependency
* RE2
    * `apt install libre2-dev` on Debian/Ubuntu

## Build & Run
1. `git clone` this repo && `cd` into it
2. Run `make`
3. `srun --mpi=pmix -N <num> ./pflgsum <logfile>` (with Slurm)

## Get average runtime for MPI excluding I/O
After building, run `./get_aver.py <node_num> <iteration_num>`

## Branches
Use different branches for different features.
* `main` branch is the serial version.
* `MPI` branch is for parallelizing the program using MPI.
