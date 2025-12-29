# C++ DataFrame Library

A header-only (with small cpp helpers) C++17 implementation of a DataFrame that mimics some features familiar from Python's pandas—CSV/binary I/O, typed indices (including dates/datetimes), arithmetic transformations, statistics, rolling windows, correlations, sampling, and more. This repository also contains multiple sample programs demonstrating common workflows (basic loading, arithmetic, statistics, indexing, I/O, custom construction, intraday data).

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

Issues and pull requests are welcome.

## License

MIT License.

## Sample Output

Here is the output of the sample programs compiled with g++.

```
--- running df_demo ---
loaded prices dataframe with 6536 rows and 10 columns

price data
        Date          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
  2000-01-03          nan    91.617100          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-04          nan    88.034200          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-05          nan    88.191800          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-06          nan    86.774400          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-07          nan    91.813900          nan          nan          nan          nan          nan          nan          nan          nan
...
  2025-12-19   335.269000   680.590000    95.460000    53.720000    96.270000    87.550000    96.240000    82.760000   110.130000    80.360000
  2025-12-22   337.600000   684.830000    95.700000    54.010000    96.310000    87.360000    96.140000    82.720000   110.110000    80.430000
  2025-12-23   338.720000   687.960000    96.290000    54.310000    96.260000    87.500000    96.100000    82.680000   110.220000    80.490000
  2025-12-24   339.880000   690.380000    96.410000    54.420000    96.550000    88.030000    96.350000    82.730000   110.650000    80.640000
  2025-12-26   339.670000   690.310000    96.570000    54.800000    96.540000    87.740000    96.440000    82.790000   110.640000    80.600000

return scaling factor: 100

computed simple returns (proportional changes)

returns
        Date          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
  2000-01-04          nan    -3.910733          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-05          nan     0.179021          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-06          nan    -1.607179          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-07          nan     5.807588          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-10          nan     0.343085          nan          nan          nan          nan          nan          nan          nan          nan
...
  2025-12-19     0.891223     0.906347     0.674963     0.977444    -0.048797    -0.373245    -0.224971    -0.018121    -0.257214    -0.048508
  2025-12-22     0.695263     0.622989     0.251414     0.539836     0.041550    -0.217019    -0.103907    -0.048333    -0.018160     0.087108
  2025-12-23     0.331754     0.457048     0.616510     0.555453    -0.051916     0.160256    -0.041606    -0.048356     0.099900     0.074599
  2025-12-24     0.342466     0.351765     0.124624     0.202541     0.301267     0.605714     0.260146     0.060474     0.390129     0.186359
  2025-12-26    -0.061787    -0.010139     0.165958     0.698273    -0.010357    -0.329433     0.093409     0.072525 -9.037506e-03    -0.049603

return statistics
   statistic          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
           n         6169         6535         6119         5713         4533         5891         5891         5891         5891         4709
      median       0.0815       0.0693       0.0734       0.0958       0.0356       0.0517       0.0238       0.0000       0.0411       0.0319
        mean       0.0437       0.0384       0.0327       0.0499       0.0206       0.0189       0.0154   7.9134e-03       0.0198       0.0217
          sd       1.2084       1.2219       1.3128       1.7051       0.6834       0.9060       0.4307       0.0960       0.5286       0.6936
        skew      -0.0994       0.0412      -0.0294       0.5021      -1.6047       0.0545       0.0818       0.2979       0.0122       0.7469
 ex_kurtosis      11.1870      12.0317      13.1951      18.4334      35.4483       3.4569       2.5309       7.1314      57.7707      42.2739
         min     -11.3808     -10.9424     -11.1630     -16.1662     -10.1104      -6.6683      -2.5073      -0.6567      -9.1110      -8.0974
         max      12.8299      14.5197      15.8874      22.7701       6.7298       7.5195       3.4263       0.9975       9.7678      12.2690

return summary with missing data
      column   first_idx    last_idx             n        median          mean            sd          skew       ex_kurt           min           max
         VTI  2001-06-18  2025-12-26          6169        0.0815        0.0437        1.2084       -0.0994       11.1870      -11.3808       12.8299
         SPY  2000-01-04  2025-12-26          6535        0.0693        0.0384        1.2219        0.0412       12.0317      -10.9424       14.5197
         EFA  2001-08-28  2025-12-26          6119        0.0734        0.0327        1.3128       -0.0294       13.1951      -11.1630       15.8874
         EEM  2003-04-15  2025-12-26          5713        0.0958        0.0499        1.7051        0.5021       18.4334      -16.1662       22.7701
         EMB  2007-12-20  2025-12-26          4533        0.0356        0.0206        0.6834       -1.6047       35.4483      -10.1104        6.7298
         TLT  2002-07-31  2025-12-26          5891        0.0517        0.0189        0.9060        0.0545        3.4569       -6.6683        7.5195
         IEF  2002-07-31  2025-12-26          5891        0.0238        0.0154        0.4307        0.0818        2.5309       -2.5073        3.4263
         SHY  2002-07-31  2025-12-26          5891        0.0000        0.0079        0.0960        0.2979        7.1314       -0.6567        0.9975
         LQD  2002-07-31  2025-12-26          5891        0.0411        0.0198        0.5286        0.0122       57.7707       -9.1110        9.7678
         HYG  2007-04-12  2025-12-26          4709        0.0319        0.0217        0.6936        0.7469       42.2739       -8.0974       12.2690

return percentiles
  percentile          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
           0     -11.3808     -10.9424     -11.1630     -16.1662     -10.1104      -6.6683      -2.5073      -0.6567      -9.1110      -8.0974
           1      -3.3743      -3.4067      -3.6176      -4.6095      -1.7526      -2.2932      -1.1116      -0.2538      -1.2139      -1.9958
           5      -1.8150      -1.9025      -1.9351      -2.3772      -0.8258      -1.4526      -0.6793      -0.1316      -0.7004      -0.8363
          25      -0.4428      -0.4596      -0.5514      -0.7309      -0.2157      -0.5124      -0.2433      -0.0358      -0.2153      -0.1913
          50       0.0815       0.0693       0.0734       0.0958       0.0356       0.0517       0.0238       0.0000       0.0411       0.0319
          75       0.5980       0.5992       0.6649       0.8551       0.2755       0.5461       0.2684       0.0484       0.2667       0.2503
          95       1.7093       1.7258       1.7947       2.2937       0.8211       1.4115       0.6765       0.1606       0.6702       0.8589
          99       3.2556       3.3479       3.4150       4.4955       1.8954       2.3580       1.1482       0.2846       1.2054       1.8140
         100      12.8299      14.5197      15.8874      22.7701       6.7298       7.5195       3.4263       0.9975       9.7678      12.2690

row completeness for returns
rows with complete data: 4533
first complete index: 2007-12-20
last complete index: 2025-12-26

return autocorrelations
         lag         VTI         SPY         EFA         EEM         EMB         TLT         IEF         SHY         LQD         HYG
           1      -0.085      -0.088      -0.099      -0.111       0.092      -0.027      -0.027      -0.070      -0.001       0.001
           2       0.001      -0.020       0.004      -0.036       0.058      -0.050      -0.042      -0.026      -0.052       0.013
           3       0.009      -0.004       0.006       0.030      -0.002      -0.028      -0.004      -0.018       0.005      -0.064
           4      -0.041      -0.026      -0.015      -0.042       0.021       0.005       0.015       0.012      -0.036       0.005
           5      -0.016      -0.013      -0.015      -0.010      -0.017      -0.004      -0.012       0.011       0.018       0.007

bootstrapped return autocorrelations
         lag         VTI         SPY         EFA         EEM         EMB         TLT         IEF         SHY         LQD         HYG
           1       0.005       0.001       0.002      -0.009       0.012       0.007       0.028       0.029       0.006      -0.008
           2      -0.021      -0.014      -0.021      -0.025      -0.004       0.006       0.006      -0.008       0.007      -0.005
           3      -0.015      -0.023      -0.019      -0.013       0.005       0.007       0.016       0.019       0.003       0.017
           4      -0.002       0.013       0.022       0.022       0.009       0.007      -0.007       0.006       0.009       0.000
           5       0.007      -0.002      -0.010      -0.013       0.019      -0.015      -0.009      -0.003       0.012       0.017

return correlation matrix
      column          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
         VTI        1.000        0.994        0.885        0.826        0.406       -0.314       -0.297       -0.224        0.204        0.689
         SPY        0.994        1.000        0.887        0.826        0.393       -0.316       -0.300       -0.227        0.211        0.691
         EFA        0.885        0.887        1.000        0.878        0.398       -0.300       -0.271       -0.187        0.234        0.671
         EEM        0.826        0.826        0.878        1.000        0.367       -0.281       -0.274       -0.212        0.193        0.621
         EMB        0.406        0.393        0.398        0.367        1.000        0.164        0.198        0.175        0.452        0.487
         TLT       -0.314       -0.316       -0.300       -0.281        0.164        1.000        0.912        0.567        0.538       -0.140
         IEF       -0.297       -0.300       -0.271       -0.274        0.198        0.912        1.000        0.758        0.556       -0.099
         SHY       -0.224       -0.227       -0.187       -0.212        0.175        0.567        0.758        1.000        0.363       -0.058
         LQD        0.204        0.211        0.234        0.193        0.452        0.538        0.556        0.363        1.000        0.456
         HYG        0.689        0.691        0.671        0.621        0.487       -0.140       -0.099       -0.058        0.456        1.000

return Spearman correlation
      column          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
         VTI        1.000        0.991        0.826        0.747        0.386       -0.270       -0.274       -0.192        0.072        0.679
         SPY        0.991        1.000        0.825        0.744        0.386       -0.272       -0.275       -0.194        0.071        0.676
         EFA        0.826        0.825        1.000        0.809        0.433       -0.233       -0.217       -0.116        0.111        0.647
         EEM        0.747        0.744        0.809        1.000        0.421       -0.232       -0.228       -0.144        0.080        0.591
         EMB        0.386        0.386        0.433        0.421        1.000        0.229        0.253        0.237        0.471        0.534
         TLT       -0.270       -0.272       -0.233       -0.232        0.229        1.000        0.920        0.597        0.733       -0.076
         IEF       -0.274       -0.275       -0.217       -0.228        0.253        0.920        1.000        0.762        0.744       -0.053
         SHY       -0.192       -0.194       -0.116       -0.144        0.237        0.597        0.762        1.000        0.562        0.024
         LQD        0.072        0.071        0.111        0.080        0.471        0.733        0.744        0.562        1.000        0.297
         HYG        0.679        0.676        0.647        0.591        0.534       -0.076       -0.053        0.024        0.297        1.000

return Kendall tau
      column          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
         VTI        1.000        0.909        0.627        0.563        0.272       -0.180       -0.188       -0.140        0.021        0.497
         SPY        0.909        1.000        0.626        0.557        0.271       -0.179       -0.186       -0.139        0.022        0.492
         EFA        0.627        0.626        1.000        0.616        0.306       -0.148       -0.143       -0.085        0.054        0.466
         EEM        0.563        0.557        0.616        1.000        0.297       -0.144       -0.146       -0.098        0.034        0.421
         EMB        0.272        0.271        0.306        0.297        1.000        0.164        0.181        0.166        0.341        0.389
         TLT       -0.180       -0.179       -0.148       -0.144        0.164        1.000        0.761        0.443        0.574       -0.051
         IEF       -0.188       -0.186       -0.143       -0.146        0.181        0.761        1.000        0.591        0.589       -0.035
         SHY       -0.140       -0.139       -0.085       -0.098        0.166        0.443        0.591        1.000        0.429        0.017
         LQD        0.021        0.022        0.054        0.034        0.341        0.574        0.589        0.429        1.000        0.201
         HYG        0.497        0.492        0.466        0.421        0.389       -0.051       -0.035        0.017        0.201        1.000

return covariance matrix
      column          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
         VTI        1.611        1.585        1.550        1.830        0.352       -0.387       -0.167       -0.027        0.147        0.611
         SPY        1.585        1.579        1.538        1.812        0.338       -0.386       -0.167       -0.027        0.150        0.606
         EFA        1.550        1.538        1.903        2.115        0.375       -0.402       -0.166       -0.024        0.183        0.646
         EEM        1.830        1.812        2.115        3.047        0.438       -0.477       -0.212       -0.035        0.191        0.757
         EMB        0.352        0.338        0.375        0.438        0.467        0.109        0.060        0.011        0.175        0.232
         TLT       -0.387       -0.386       -0.402       -0.477        0.109        0.945        0.393        0.052        0.297       -0.095
         IEF       -0.167       -0.167       -0.166       -0.212        0.060        0.393        0.196        0.032        0.140       -0.031
         SHY       -0.027       -0.027       -0.024       -0.035        0.011        0.052        0.032    8.902e-03        0.019   -3.850e-03
         LQD        0.147        0.150        0.183        0.191        0.175        0.297        0.140        0.019        0.321        0.180
         HYG        0.611        0.606        0.646        0.757        0.232       -0.095       -0.031   -3.850e-03        0.180        0.487

returns (%) first rows
        Date          SPY          EFA
  2000-01-04    -3.910733          nan
  2000-01-05     0.179021          nan
  2000-01-06    -1.607179          nan
  2000-01-07     5.807588          nan
  2000-01-10     0.343085          nan

returns sorted by SPY
        Date          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
  2020-03-16   -11.380825   -10.942361   -10.319491   -12.479065    -4.617264     6.476601     2.641720     0.162222    -1.422204    -5.496592
  2008-10-15    -9.352784    -9.844733   -10.669929   -16.166236    -0.578211     0.956207     0.819137     0.334834     0.000000    -4.363554
  2020-03-12    -9.723928    -9.567668   -10.990059   -10.010762    -4.924777     0.619703     0.050845    -0.023237    -4.771914    -3.995457
  2008-12-01    -8.938975    -8.857721    -8.171475    -9.629850    -2.990074     3.829675     1.353810     0.164502     1.563262    -2.062293
  2008-09-29    -6.588890    -7.836196   -11.163045   -11.679326    -1.800770     2.909952     1.394669     0.575469    -9.111039    -8.097432

returns columns sorted by first row
        Date          SPY          VTI          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
  2000-01-04    -3.910733          nan          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-05     0.179021          nan          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-06    -1.607179          nan          nan          nan          nan          nan          nan          nan          nan          nan

custom dataframe from vectors
  CustomDate        Alpha         Beta
  2025-01-01     1.000000     2.000000
  2025-01-02     3.000000     4.000000
  2025-01-03     5.000000     6.000000

standardized returns (z-scores)
        Date          SPY          EFA
  2000-01-04    -3.232033          nan
  2000-01-05     0.115109          nan
  2000-01-06    -1.346755          nan
  2000-01-07     4.721647          nan
  2000-01-10     0.249382          nan

normalized returns (last rows)
        Date          SPY          EFA
  2025-12-19     0.465347     0.437627
  2025-12-22     0.454219     0.421969
  2025-12-23     0.447702     0.435466
  2025-12-24     0.443567     0.417282
  2025-12-26     0.429353     0.418810

returns 2003-04-15..2003-04-22
        Date          SPY          EFA
  2003-04-15     0.933140     1.371317
  2003-04-16    -1.704200    -0.660585
  2003-04-17     1.484394     1.444811
  2003-04-21     0.100523    -0.348365
  2003-04-22     1.885125     1.449605

returns at endpoints
        Date          SPY          TLT
  2000-01-04    -3.910733          nan
  2025-12-26    -0.010139    -0.329433

log price preview
        Date          SPY          TLT
  2000-01-03     4.517618          nan
  2000-01-04     4.477725          nan
  2000-01-05     4.479514          nan

exp(log price) preview
        Date          SPY          TLT
  2000-01-03    91.617100          nan
  2000-01-04    88.034200          nan
  2000-01-05    88.191800          nan

first two price columns
        Date          VTI          SPY
  2000-01-03          nan    91.617100
  2000-01-04          nan    88.034200
  2000-01-05          nan    88.191800

last two price columns
        Date          LQD          HYG
  2000-01-03          nan          nan
  2000-01-04          nan          nan
  2000-01-05          nan          nan

SPY returns sample: first=-3.91073, last=-0.0101393, count=6535

SPY returns with squared column
        Date          SPY       SPY_sq
  2000-01-04    -3.910733    15.293831
  2000-01-05     0.179021     0.032049
  2000-01-06    -1.607179     2.583024

row-major buffer sample: [nan, -3.91073, nan, ...]

returns reloaded from binary
        Date          SPY          EFA
  2000-01-04    -3.910733          nan
  2000-01-05     0.179021          nan
  2000-01-06    -1.607179          nan

sample datetime-indexed returns
   timestamp   SPY_return
2000-01-04 00:00:00    -3.910730
2000-01-05 01:00:00     0.179021
2000-01-06 02:00:00    -1.607180
2000-01-07 03:00:00     5.807590
2000-01-10 04:00:00     0.343085
first row values: SPY=nan, EFA=-3.91073

5-day rolling mean
        Date          SPY          EFA
  2000-01-10     0.162357          nan
  2000-01-11     0.705186          nan
  2000-01-12     0.470429          nan

5-day rolling std
        Date          SPY          EFA
  2000-01-10     3.592454          nan
  2000-01-11     2.975157          nan
  2000-01-12     3.071794          nan

5-day rolling rms
        Date          SPY          EFA
  2000-01-10     3.217288          nan
  2000-01-11     2.752914          nan
  2000-01-12     2.787479          nan

EMA(alpha=0.1) first rows
        Date          SPY          EFA
  2000-01-04    -3.910733          nan
  2000-01-05    -3.501757          nan
  2000-01-06    -3.312300          nan
rows before NaN removal: 3, after: 0, columns after dropping NaNs: 0

random normal target correlation: 0.7

random normal stats
   statistic        Alpha         Beta        Gamma
           n         1000         1000         1000
      median    -0.028973     0.034076    -0.029050
        mean    -0.019795     0.043795 9.845398e-04
          sd     1.013260     1.007175     1.034801
        skew     0.055334     0.068212     0.148109
 ex_kurtosis 9.546673e-03     0.166948     0.061524
         min    -3.364096    -2.849686    -3.249447
         max     3.187033     3.316192     4.018276

random normal correlations
      column        Alpha         Beta        Gamma
       Alpha        1.000        0.837        0.837
        Beta        0.837        1.000        0.683
       Gamma        0.837        0.683        1.000

random normal covariances
      column        Alpha         Beta        Gamma
       Alpha        1.027        0.854        0.877
        Beta        0.854        1.014        0.712
       Gamma        0.877        0.712        1.071

random uniform sample
       index           U1           U2           U3
           0       0.7225       0.8319       0.7341
           1       0.7010       0.1933       0.9773
           2       0.5777       0.5138       0.8515
           3       0.3811       0.2739       0.6406
           4       0.4936       0.3368       0.6163

returns shape: (6535, 10)

SPY intraday sample (first 5 rows)
    Datetime         Open         High          Low        Close       Volume
2025-10-28 13:20:00   687.059998   687.299988   687.049988   687.265015     0.000000
2025-10-28 13:25:00   687.270020   687.270020   686.969971   687.010010 3.912220e+05
2025-10-28 13:30:00   687.020020   687.140015   686.875000   687.075012 3.632480e+05
2025-10-28 13:35:00   687.059998   687.280029   687.059998   687.125000 3.217930e+05
2025-10-28 13:40:00   687.130005   687.325500   687.114990   687.299988 4.112810e+05

--- running x_basic ---
prices shape: (6536, 10)
columns: VTI SPY EFA EEM EMB TLT IEF SHY LQD HYG

first rows

prices head
        Date          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
  2000-01-03          nan    91.617100          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-04          nan    88.034200          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-05          nan    88.191800          nan          nan          nan          nan          nan          nan          nan          nan

prices tail
        Date          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
  2025-12-23   338.720000   687.960000    96.290000    54.310000    96.260000    87.500000    96.100000    82.680000   110.220000    80.490000
  2025-12-24   339.880000   690.380000    96.410000    54.420000    96.550000    88.030000    96.350000    82.730000   110.650000    80.640000
  2025-12-26   339.670000   690.310000    96.570000    54.800000    96.540000    87.740000    96.440000    82.790000   110.640000    80.600000

--- running x_arithmetic ---

original subset
        Date          SPY          EFA
  2000-01-03    91.617100          nan
  2000-01-04    88.034200          nan
  2000-01-05    88.191800          nan
  2000-01-06    86.774400          nan
  2000-01-07    91.813900          nan

+2
        Date          SPY          EFA
  2000-01-03    93.617100          nan
  2000-01-04    90.034200          nan
  2000-01-05    90.191800          nan
  2000-01-06    88.774400          nan
  2000-01-07    93.813900          nan

-1
        Date          SPY          EFA
  2000-01-03    90.617100          nan
  2000-01-04    87.034200          nan
  2000-01-05    87.191800          nan
  2000-01-06    85.774400          nan
  2000-01-07    90.813900          nan

*1.05
        Date          SPY          EFA
  2000-01-03    96.197955          nan
  2000-01-04    92.435910          nan
  2000-01-05    92.601390          nan
  2000-01-06    91.113120          nan
  2000-01-07    96.404595          nan

/2
        Date          SPY          EFA
  2000-01-03    45.808550          nan
  2000-01-04    44.017100          nan
  2000-01-05    44.095900          nan
  2000-01-06    43.387200          nan
  2000-01-07    45.906950          nan

log subset
        Date          SPY          EFA
  2000-01-03     4.517618          nan
  2000-01-04     4.477725          nan
  2000-01-05     4.479514          nan
  2000-01-06     4.463312          nan
  2000-01-07     4.519764          nan

exp(log subset)
        Date          SPY          EFA
  2000-01-03    91.617100          nan
  2000-01-04    88.034200          nan
  2000-01-05    88.191800          nan
  2000-01-06    86.774400          nan
  2000-01-07    91.813900          nan

--- running x_stats ---

return scaling factor: 100

returns head
        Date          SPY          EFA
  2000-01-04    -3.910733          nan
  2000-01-05     0.179021          nan
  2000-01-06    -1.607179          nan
  2000-01-07     5.807588          nan
  2000-01-10     0.343085          nan

summary stats
   statistic          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
           n         6169         6535         6119         5713         4533         5891         5891         5891         5891         4709
      median       0.0815       0.0693       0.0734       0.0958       0.0356       0.0517       0.0238       0.0000       0.0411       0.0319
        mean       0.0437       0.0384       0.0327       0.0499       0.0206       0.0189       0.0154   7.9134e-03       0.0198       0.0217
          sd       1.2084       1.2219       1.3128       1.7051       0.6834       0.9060       0.4307       0.0960       0.5286       0.6936
        skew      -0.0994       0.0412      -0.0294       0.5021      -1.6047       0.0545       0.0818       0.2979       0.0122       0.7469

correlation matrix
      column          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
         VTI        1.000        0.994        0.885        0.826        0.406       -0.314       -0.297       -0.224        0.204        0.689
         SPY        0.994        1.000        0.887        0.826        0.393       -0.316       -0.300       -0.227        0.211        0.691
         EFA        0.885        0.887        1.000        0.878        0.398       -0.300       -0.271       -0.187        0.234        0.671
         EEM        0.826        0.826        0.878        1.000        0.367       -0.281       -0.274       -0.212        0.193        0.621
         EMB        0.406        0.393        0.398        0.367        1.000        0.164        0.198        0.175        0.452        0.487
         TLT       -0.314       -0.316       -0.300       -0.281        0.164        1.000        0.912        0.567        0.538       -0.140
         IEF       -0.297       -0.300       -0.271       -0.274        0.198        0.912        1.000        0.758        0.556       -0.099
         SHY       -0.224       -0.227       -0.187       -0.212        0.175        0.567        0.758        1.000        0.363       -0.058
         LQD        0.204        0.211        0.234        0.193        0.452        0.538        0.556        0.363        1.000        0.456
         HYG        0.689        0.691        0.671        0.621        0.487       -0.140       -0.099       -0.058        0.456        1.000

covariance matrix
      column          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
         VTI     1.611295     1.585318     1.550055     1.829655     0.351758    -0.386986    -0.167359    -0.026852     0.146677     0.611001
         SPY     1.585318     1.579159     1.538016     1.812257     0.337551    -0.385572    -0.166881    -0.026874     0.150494     0.606072
         EFA     1.550055     1.538016     1.903266     2.115369     0.375153    -0.401769    -0.165611    -0.024368     0.183189     0.646452
         EEM     1.829655     1.812257     2.115369     3.046807     0.438151    -0.477047    -0.211801    -0.034985     0.191193     0.756800
         EMB     0.351758     0.337551     0.375153     0.438151     0.467010     0.109193     0.060052     0.011253     0.175042     0.232344
         TLT    -0.386986    -0.385572    -0.401769    -0.477047     0.109193     0.945435     0.392953     0.051999     0.296596    -0.095113
         IEF    -0.167359    -0.166881    -0.165611    -0.211801     0.060052     0.392953     0.196425     0.031692     0.139667    -0.030547
         SHY    -0.026852    -0.026874    -0.024368    -0.034985     0.011253     0.051999     0.031692 8.902280e-03     0.019416 -3.850090e-03
         LQD     0.146677     0.150494     0.183189     0.191193     0.175042     0.296596     0.139667     0.019416     0.320950     0.180481
         HYG     0.611001     0.606072     0.646452     0.756800     0.232344    -0.095113    -0.030547 -3.850090e-03     0.180481     0.487371

5-day rolling mean
        Date          SPY          EFA
  2000-01-10     0.162357          nan
  2000-01-11     0.705186          nan
  2000-01-12     0.470429          nan

--- running x_indexing ---

return scaling factor: 100

slice 2002-01-02..2002-01-10
        Date          SPY          EFA
  2002-01-02     1.076130     0.553313
  2002-01-03     1.133950     0.900263
  2002-01-04     0.667484     0.660823
  2002-01-07    -0.705654    -1.329560
  2002-01-08    -0.231223    -1.139470
  2002-01-09    -0.815216    -1.052018
  2002-01-10     0.441290     0.238177

selected rows
        Date          SPY          TLT
  2000-01-04    -3.910733          nan
  2000-01-05     0.179021          nan
  2000-01-06    -1.607179          nan

sorted by SPY
        Date          SPY          EFA
  2020-03-16   -10.942361   -10.319491
  2008-10-15    -9.844733   -10.669929
  2020-03-12    -9.567668   -10.990059
  2008-12-01    -8.857721    -8.171475
  2008-09-29    -7.836196   -11.163045

columns sorted by first row
        Date          SPY          VTI          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
  2000-01-04    -3.910733          nan          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-05     0.179021          nan          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-06    -1.607179          nan          nan          nan          nan          nan          nan          nan          nan          nan

--- running x_io ---

binary reload
        Date          VTI          SPY          EFA          EEM          EMB          TLT          IEF          SHY          LQD          HYG
  2000-01-03          nan    91.617100          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-04          nan    88.034200          nan          nan          nan          nan          nan          nan          nan          nan
  2000-01-05          nan    88.191800          nan          nan          nan          nan          nan          nan          nan          nan
row-major dump: nan 91.6171 nan nan nan nan nan nan nan nan nan 88.0342 nan nan nan nan nan nan nan nan nan 88.1918 nan nan nan nan nan nan nan nan
column-major dump: nan nan nan 91.6171 88.0342 88.1918 nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan nan

--- running x_construct ---

from_vectors
  CustomDate        Alpha         Beta
  2024-01-01     1.000000     2.000000
  2024-01-02     3.000000     4.000000
  2024-01-03     5.000000     6.000000

after add_column
  CustomDate        Alpha         Beta        Gamma
  2024-01-01     1.000000     2.000000    10.000000
  2024-01-02     3.000000     4.000000    20.000000
  2024-01-03     5.000000     6.000000    30.000000

--- running x_intraday ---

intraday head
    Datetime         Open         High          Low        Close
2025-10-28 13:20:00   687.059998   687.299988   687.049988   687.265015
2025-10-28 13:25:00   687.270020   687.270020   686.969971   687.010010
2025-10-28 13:30:00   687.020020   687.140015   686.875000   687.075012
2025-10-28 13:35:00   687.059998   687.280029   687.059998   687.125000
2025-10-28 13:40:00   687.130005   687.325500   687.114990   687.299988

sorted by close
    Datetime        Close       Volume
2025-11-21 10:35:00   651.429993 2.344265e+06
2025-11-21 11:00:00   651.890015 8.309780e+05
2025-11-20 15:45:00   652.000000 3.757045e+06
2025-11-21 11:15:00   652.239990 7.683840e+05
2025-11-20 15:55:00   652.530029 6.592158e+06

3-period rolling mean
    Datetime        Close
2025-10-28 13:30:00   687.116679
2025-10-28 13:35:00   687.070007
2025-10-28 13:40:00   687.166667
```


