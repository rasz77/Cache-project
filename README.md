# Cache Lab — Cache Replacement Policy Simulator

A C program that simulates how a CPU cache responds to a trace of memory addresses. It models a configurable set-associative cache and supports three replacement policies: **LRU**, **FIFO**, and **Optimal** (Belady's algorithm).


## Overview

This project implements a cache simulator that reads a trace of hexadecimal memory addresses and determines, for each one, whether it results in a **hit** or a **miss** in a simulated set-associative cache. Cache geometry (number of sets, associativity, block size) and the replacement policy are all configured via command-line flags. After processing the full trace, the program prints a final summary with total hits, misses, miss rate, and an estimated total running time in cycles.

## Features

- Configurable **set-associative cache**: number of sets, associativity, and block size are all set via command-line flags
- Three interchangeable **replacement policies**, selected at runtime:
  - `lru` — Least Recently Used
  - `fifo` — First In, First Out
  - `optimal` — Belady's optimal (lookahead-based) policy
- Per-access **hit/miss** reporting as the trace is processed
- End-of-run **summary report**: hits, misses, miss rate, and estimated total cycles

## How It Works

1. **Parse command-line arguments** for cache geometry, the trace file path, and the replacement policy.
2. **Read the trace file** — one hexadecimal memory address per line.
3. **Decompose each address** into:
   - **Tag** — `addr >> (b + s)`, identifies which block of memory is cached
   - **Set index** — `(addr >> b) & ((1 << s) - 1)`, determines which set the block maps to
   - *(Block offset bits exist in the address space but aren't extracted separately, since the simulator only needs to know which block is being accessed, not the byte within it.)*
4. **Check the relevant set** for a line with a matching valid tag (hit) or not (miss).
5. **On a miss**, fill an empty line if one exists; otherwise evict a line according to the active replacement policy.
6. **Repeat** for every address in the trace, then print the final summary.

## Replacement Policies

| Policy | Flag value | Description |
|--------|------------|-------------|
| **LRU** | `lru` | Evicts the line in the set with the oldest "last used" timestamp. |
| **FIFO** | `fifo` | Evicts the line in the set with the oldest insertion timestamp, regardless of recent use. |
| **Optimal** | `optimal` | Looks ahead through the remaining trace and evicts the line whose tag is reused furthest in the future (or never reused at all). Used as a theoretical best-case benchmark — it requires full knowledge of future accesses, which a real cache doesn't have. |

## Build

Requires `cachelab.h` (defines the `HIT_TIME` and `MISS_PENALTY` constants used in the cycle estimate) in the same directory.

```bash
gcc -Wall -o cache_project cache_project.c
```

> **Note:** `main()` contains a nested function definition (`printResult`), which is a GNU C extension. This compiles fine with GCC's default settings, but will fail under strict-standard flags like `-std=c11 -pedantic`, and won't compile with non-GNU compilers (e.g. MSVC). See [Known Limitations](#known-limitations).

## Usage

```bash
./cache_project -m <m> -s <s> -e <e> -b <b> -i <trace_file> -r <policy>
```

| Flag | Meaning |
|------|---------|
| `-m` | Address bit width (currently parsed but not used in the simulation logic) |
| `-s` | Number of set-index bits — number of sets = `2^s` |
| `-e` | Associativity exponent — lines per set = `2^e` |
| `-b` | Number of block-offset bits — block size = `2^b` bytes |
| `-i` | Path to the trace file |
| `-r` | Replacement policy: `lru`, `fifo`, or `optimal` |

All six flags are required; the program prints a usage message and exits with an error if any are missing.

**Example** — 16 sets, 4-way associative, 16-byte blocks, LRU policy:

```bash
./cache_project -m 32 -s 4 -e 2 -b 4 -i trace.txt -r lru
```

## Input File Format

The trace file should contain **one hexadecimal memory address per line** (no `0x` prefix needed). Blank lines and leading whitespace are ignored.

```
7ffd2a3c
7ffd2a40
7ffd2a44
```

> This is a simpler format than the classic Valgrind-style trace (`L 10,4`, `S 18,1`, etc.) — this simulator reads each line as a raw hex address only, with no operation type or access size.

## Output Format

For each address, the program prints the address in uppercase hex followed by `H` (hit) or `M` (miss):

```
<ADDRESS> <H|M>
```

After the full trace is processed, it prints a one-line summary:

```
[result] hits: <hits> misses: <misses> miss rate: <rate>% total running time: <cycles> cycle
```

> The miss rate and cycle estimate are based on a simplified average-access-time model (`HIT_TIME` and `MISS_PENALTY` from `cachelab.h`) rather than a per-instruction timing trace, and the printed percentage is truncated rather than rounded.

## Sample Run

```
$ ./cache_project -m 32 -s 4 -e 2 -b 4 -i trace.txt -r lru
7FFD2A3C M
7FFD2A40 H
7FFD2A44 H
...
[result] hits: 850 misses: 150 miss rate: 15% total running time: 12450 cycle
```
