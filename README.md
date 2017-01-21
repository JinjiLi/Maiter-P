<<<<<<< HEAD
# Maiter-P
Maiter-P is an Parallel Asynchronous Graph Processing Framework for Delta-based Accumulative Iterative Computation.
=======
>>>>>>> my first version

-- BUILDING

Simply run 'make all' to excute Makefile.

-- CONFIGURATION

-- INPUT DATA
rename input file "input_graph"

-- FORMAT:
Adjacency List:src1 dst1 dst2 dst3 dst4 ...


Which would allow for running up to 16 threads (+ 1 main thread).

The following is the  conf run pagerank.
---------------------------------------------
ALGORITHM=Pagerank
THREADS=16
INPUT=input/pagerank
RESULT=result/pagerank
NODES=2000000
TERMTHRESH=0.01
PORTION=1
TERMCHECKINTERVAL=1


-- RUNNING

run pagerank:
sh pagerank.sh

run shotpath:
sh shortpath.sh
<<<<<<< HEAD
=======

>>>>>>> my first version
