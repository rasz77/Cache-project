System Programming - Cache Lab

Overview:
This project implements a cache simulator in C that models how a CPU cache behaves when accessing memory addresses. The simulator reads a sequence of memory addresses from an input file and determines whether each access results in a cache hit or miss.
The program supports three common cache replacement policies:
1. LRU (Least Recently Used) – Replaces the cache line that has not been used for the longest time.
2. FIFO (First In First Out) – Replaces the cache line that was inserted earliest.
3. Optimal – Replaces the cache line that will not be used for the longest time in the future.
The simulator reports each access result and prints a final summary including hits, misses, miss rate, and total running time in cycles.

This project demonstrates several computer architecture concepts:
- Cache memory organization
- Set associative caches
- Address decomposition (tag, set index, block offset)
- Replacement policies
- Memory access performance analysis
