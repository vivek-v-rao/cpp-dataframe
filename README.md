# C++ DataFrame Library

A header-only (with small cpp helpers) C++17 implementation of a DataFrame that mimics many of the features familiar from Python's pandas—CSV/binary I/O, typed indices (including dates/datetimes), arithmetic transformations, statistics, rolling windows, correlations, sampling, and more. This repository also contains multiple sample programs demonstrating common workflows (basic loading, arithmetic, statistics, indexing, I/O, custom construction, intraday data).

## Motivation

C++ frequently lacks a convenient, flexible in-memory tabular structure. Most teams end up reinventing custom row/column utilities or fall back to Python/R for data munging even when their core systems are C++. This project aims to provide a pragmatic DataFrame with the following goals:

- **Native C++17**: no external dependencies apart from the standard library.
- **Typed indices**: indexes can be integers, strings, `Date`, `DateTime`, etc., with custom parsing/formatting.
- **Rich operations**: arithmetic, transforms, rolling statistics, correlations, resampling, selection, sorting.
- **Simple I/O**: read/write CSVs, binary snapshots, contiguous buffers for hand-off to other libraries.
- **Testable examples**: multiple `x_*` sample apps show end-to-end usage.

## Key Features

- **Construction & I/O**
  - `from_csv`, `from_vectors`, `random_normal`, `random_uniform`, `from_binary`, `from_binary_file`.
  - `to_csv`, `to_csv_file`, `to_binary`, `to_binary_file`, `to_row_major`, `to_column_major`.
- **Index support**
  - Template `DataFrame<IndexT>` with built-in `Date` and `DateTime` helpers and parsing/formatting.
  - Selection: `select_rows`, `slice_rows_range`, `head/tail`, `sort_rows_by_column`, `sort_columns_by_row`.
- **Column operations**
  - Arithmetic (`add`, `subtract`, `multiply`, `divide`), log/exp, power, normalization, standardization, scaling by scalars or other frames.
  - `add_column` for derived series.
- **Statistics & Analytics**
  - Column stats, summary with missing-data info, percentiles, rolling mean/std/rms, EMA, correlations (Pearson, Spearman, Kendall), covariance, percentiles.
  - Resampling, NaN removal, random resampling, random-data generators (normal with optional correlation, uniform).
- **Printing utilities**
  - `print_frame`, column summaries, percentiles, autocorrelations.
  - Sample programs (`x_basic`, `x_arithmetic`, `x_stats`, `x_indexing`, `x_io`, `x_construct`, `x_intraday`) cover different use cases.

## Sample Programs

All executables build via `make`. `make run` executes:

| Program        | Description |
|----------------|-------------|
| `df_demo`      | Comprehensive tour: CSV load, returns, stats, rolling metrics, correlations, binary I/O, random data, percentiles. |
| `x_basic`      | Load prices and print shapes/head/tail. |
| `x_arithmetic` | Scalar and element-wise arithmetic/log/exp transforms. |
| `x_stats`      | Returns, summary stats, correlations, rolling stats. |
| `x_indexing`   | Row slicing, selection, sorting. |
| `x_io`         | CSV/binary round trip, contiguous buffer export. |
| `x_construct`  | Build frames from vectors and add columns. |
| `x_intraday`   | Intraday datetime indices, sorting, rolling mean. |

## Limitations / Future Work

- **Group-by / joins**: Not yet implemented; all operations treat columns independently. Users needing relational joins or aggregations must implement them manually or extend the library.
- **Type coverage**: Columns are `double` only. Adding string or integer data columns would require significant rework.
- **Performance**: Current storage is `std::vector<std::vector<double>>`; heavy numeric workloads might prefer contiguous storage and SIMD-friendly operations.
- **Error handling**: Many functions throw `std::runtime_error` for invalid input; there is no soft error mode.
- **Kendall tau complexity**: The naive O(n^2) implementation may be slow for very wide/tall datasets; optimized algorithms could replace it if needed.
- **Thread safety**: No synchronization primitives; users must guard access if using from multiple threads.
- **Binary format**: Custom, undocumented beyond code comments; subject to change.
- **Dependencies**: Standard library only means no GPU/BLAS acceleration; integration with third-party libraries could be added.

## Building

```bash
# GCC (default)
make            # build df_demo + samples
make run        # run df_demo + samples sequentially

# Clang
make -f Makefile.clang
make -f Makefile.clang run

# MSVC (cl.exe in PATH)
make -f Makefile.msvc
make -f Makefile.msvc run

./df_demo       # run the main demo manually
```

Ensure the CSV inputs (e.g., `prices_2000_on.csv`, `SPY_intraday.csv`) are in the working directory.

## Contributing

Issues and pull requests are welcome. Ideas for future additions:
- Group-by/aggregation framework.
- Join/merge operations.
- Additional file formats (Parquet via Arrow bindings, JSON).
- In-place operations or expression templates for performance.

## License

(Choose a license, e.g., MIT/BSD, and state it here.)
