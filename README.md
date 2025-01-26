# Text Indexing Project (Trie Variants)

This project originates from the **Text-Indexierung** course (Winter Semester 2024/25, Dr. Florian Kurpicz) at KIT.
The goal is to implement multiple Trie data structure variants—here, three different versions are demonstrated:

1. **Vector-based Trie**
2. **Array-based Trie**
3. **Hash-based Trie**

These implementations support operations such as `insert`, `contains`, and `remove`.

---

## Building the Project

### Prerequisites

- A C++ compiler (supporting at least C++20)
- [CMake](https://cmake.org/) for building
- [Python3](https://python.org/) for benchmarks and plotting (not yet included)

### Dependencies

- The project uses my own cmake-utilities which are automatically downloaded by the CMakeLists.txt

### Build Steps

1. Generate the build system (Release configuration):
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release --preset release -S . -B cmake-build-release
   ```
2. Build the project:
   ```bash
   cmake --build cmake-build-release --target all
   ```

The executable `ti_programm` will be located in `cmake-build-release/bin/`.

---

## Running the Program

General usage:

```
ti_programm -variant_value=<1|2|3> <eingabe_datei> <query_datei>
```

- **`-variant_value`** selects which Trie implementation to use:
    - `1` - Vector Trie
    - `2` - Array Trie
    - `3` - Hash Trie
- **`<eingabe_datei>`** is a text file containing one word (null-terminated or $-terminated) per line.
- **`<query_datei>`** is a text file containing words plus an operation type (`c`, `i`, or `d`) per line.

### Output

- The program prints performance results (construction time, memory usage, query time) to **stdout**.
- It also writes the line-by-line results of the queries to a file named `result_<eingabe_datei>` in the current
  directory.
