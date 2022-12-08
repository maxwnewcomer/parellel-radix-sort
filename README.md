# Parellel Radix Sort
by Max Newcomer

Team solution to project: https://github.com/remzi-arpacidusseau/ostep-projects/tree/master/concurrency-sort


## Preamble

When we first started this project, we actually thought that the sort was supposed to be within the same file. So we actually started with this pretty cool (but not super fast) thread latching radix sort. The problem with that sort was that there were many iterations of syncronizing the threads.

We went form that approach to our current implementation when reading the tests (and the project description lol) again. With the requirement of another output file we thought up our new implementation.


##  Current Implementation

1. Spawn 16 threads with id's 0 through F.
2. Each thread reads (in-parellel) the input file and grabs all records with the Most Sig Bits of the key matching the thread id
3. Threads keep track of how many they added into their personal memory
4. Thread 0 finds its starting point in the output mmap
    - Then thread 1, thread 2, ... , thread F
    - This is the only part where order matters
5. All threads radix sort within their own subarray within the mmap
    - making sure to use bit operations instead of integer operations

The main problem with this implementation is that the size of the initial gathering array is impossible to know. So, we default it to filesize / 100 / 16 records. With the ability to expand into "extra" arrays. This adds an annoying extra complexity, but it's pretty dynamic which is nice. 

Pretty proud of this sort and think it's actually pretty cool.

## Future Ideas

If we had a little more time and/or we were getting paid to actually deliver this we would like to make the number of threads a little more dynamic, allowing for numerous processor core cpus to take advantage of each core. This would look something like spawning n threads and then bit masking the first log_2(n) bits of each record to delegate work to each thread. 


## Extras

Did some performance tuning in valgrind as well.

