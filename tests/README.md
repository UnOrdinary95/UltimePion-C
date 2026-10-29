# Automated tests

Run from the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Memory and undefined-behavior checks (GCC or Clang):

```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure
```

## Coverage

- Initial board, turn number, and penalties.
- 30,250 movement validation cases: both players, every source/destination
  square, every piece type, and every target type. An independent rule-based
  oracle supplies expected results; validation must not change the board.
- Board boundaries, invalid player numbers, and redeployment square validation.
- Independent moves, all four duel combinations, stationary captures, evasions,
  pawn/knight swaps, and invalid moves (including capture of the invalid mover).
- Both players' promotions and redeployments, simultaneous promotions,
  simultaneous redeployments, redeployment collisions, occupied targets,
  and a knight captured during the turn changing promotion eligibility.
- Forbidden last-rank redeployment onto a square vacated during the turn
  (including the reported Platon case), missing coordinates, and out-of-bounds
  coordinates, for both players. Invalid redeployment loses the pawn without
  adding a movement penalty.
- End conditions: pawn counts, blocked players, penalties, penalty precedence,
  simultaneous last-pawn loss, and losing the last pawn through promotion.
- Loading/writing game states, compact input, and a full turn with expected
  output files. Whitespace is normalized; every output token must match.
- Three complete interactive sessions checking each possible result and
  rejecting contradictory winner messages.

`test_game.c` includes the original source with a renamed `main`. The submitted
source is not modified. Checks do not use `assert`, so release builds still test
the conditions. Every test runs in a separate process with a timeout.

## Scope and limitations

The reference is the assignment's game rules, not the implementation. These
tests are local development tools, not the official Platon tests or proof that
all possible games are correct. Only states and counters relevant to a legal
game are used for end-condition tests.

Explicit missing coordinates (`{-1, -1}`) and out-of-bounds redeployments are
covered. Uninitialized C fields are not valid test inputs and are not covered;
drivers must initialize the complete displacement structures.

Interactive tests supply complete input. EOF handling and all interactive
promotion paths are not covered. The CLI currently retries forever at EOF;
the test timeouts prevent incomplete fixtures from hanging indefinitely.

The assignment requires only `ultime_pion.c` and a `LisezMoi` in the submission;
do not include the development build directories or tests unless requested.

## CMake references

- https://cmake.org/cmake/help/latest/command/add_test.html
- https://cmake.org/cmake/help/latest/command/execute_process.html
- https://cmake.org/cmake/help/latest/module/CTest.html
