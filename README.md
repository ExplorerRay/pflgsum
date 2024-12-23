# pflgsum

Parallel Postfix log summarizer - Multi-threaded implementation

## Dependency

- RE2
  - `apt install libre2-dev` on Debian/Ubuntu

## Build & Run

1. git clone this repo
2. Run `make`
3. Run `./pflgsum <log filename> <thread count> <reduce mode>`
    - mode = 0: linear reduce
    - mode = 1: tree reduce
