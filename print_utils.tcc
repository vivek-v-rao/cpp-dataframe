#ifndef DATAFRAME_PRINT_UTILS_TCC
#define DATAFRAME_PRINT_UTILS_TCC

#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>

namespace df {
namespace print {

template <typename IndexT>
void print_column_summary(const DataFrame<IndexT>& frame) {
  const int label_width = 10;
  const int value_width = 16;
  static const std::vector<std::string> headers = {"n", "mean", "sd", "skew",
                                                   "ex_kurtosis", "min", "max"};
  std::cout << "\ncolumn summary statistics\n";
  std::cout << std::setw(label_width) << "column";
  for (const auto& h : headers) {
    std::cout << std::setw(value_width) << h;
  }
  std::cout << '\n';

  auto old_flags = std::cout.flags();
  auto old_precision = std::cout.precision();
  std::cout << std::fixed << std::setprecision(6);

  for (std::size_t c = 0; c < frame.cols(); ++c) {
    std::vector<double> values;
    values.reserve(frame.rows());
    for (std::size_t r = 0; r < frame.rows(); ++r) {
      values.push_back(frame.value(r, c));
    }
    stats::SummaryStats summary = stats::summary_stats(values);
    std::cout << std::setw(label_width) << frame.columns()[c];
    std::cout << std::setw(value_width) << summary.n;
    std::cout << std::setw(value_width) << summary.mean;
    std::cout << std::setw(value_width) << summary.sd;
    std::cout << std::setw(value_width) << summary.skew;
    std::cout << std::setw(value_width) << summary.ex_kurtosis;
    std::cout << std::setw(value_width) << summary.min;
    std::cout << std::setw(value_width) << summary.max;
    std::cout << '\n';
  }

  std::cout.flags(old_flags);
  std::cout.precision(old_precision);
}

template <typename IndexT>
void print_column_summary_with_missing(const DataFrame<IndexT>& frame,
                                       const std::string& title,
                                       int precision) {
  const int label_width = 12;
  const int value_width = 14;
  std::cout << "\n" << title << "\n";
  std::cout << std::setw(label_width) << "column" << std::setw(label_width)
            << "first_idx" << std::setw(label_width) << "last_idx"
            << std::setw(value_width) << "n" << std::setw(value_width)
            << "median" << std::setw(value_width) << "mean" << std::setw(value_width) << "sd" << std::setw(value_width)
            << "skew" << std::setw(value_width) << "ex_kurt"
            << std::setw(value_width) << "min" << std::setw(value_width)
            << "max" << '\n';

  auto index_to_string = [](const IndexT& idx) {
    std::ostringstream oss;
    oss << idx;
    return oss.str();
  };

  auto old_flags = std::cout.flags();
  auto old_precision = std::cout.precision();
  std::cout << std::fixed << std::setprecision(precision);

  for (std::size_t c = 0; c < frame.cols(); ++c) {
    std::vector<double> values;
    values.reserve(frame.rows());
    std::string first_idx = "NA";
    std::string last_idx = "NA";
    for (std::size_t r = 0; r < frame.rows(); ++r) {
      double v = frame.value(r, c);
      if (v == v) {
        if (first_idx == "NA") first_idx = index_to_string(frame.index()[r]);
        last_idx = index_to_string(frame.index()[r]);
        values.push_back(v);
      }
    }
    stats::SummaryStats summary = stats::summary_stats(values);
    double median = detail::compute_median(values);
    if (summary.n <= 0) {
      first_idx = "NA";
      last_idx = "NA";
    }
    std::cout << std::setw(label_width) << frame.columns()[c]
              << std::setw(label_width) << first_idx << std::setw(label_width)
              << last_idx << std::setw(value_width) << summary.n
              << std::setw(value_width) << median << std::setw(value_width)
              << summary.mean << std::setw(value_width) << summary.sd
              << std::setw(value_width) << summary.skew << std::setw(value_width)
              << summary.ex_kurtosis << std::setw(value_width) << summary.min
              << std::setw(value_width) << summary.max << '\n';
  }

  std::cout.flags(old_flags);
  std::cout.precision(old_precision);
}

template <typename IndexT>
void print_row_validity_summary(const DataFrame<IndexT>& frame,
                                const std::string& title) {
  std::size_t valid_rows = 0;
  std::optional<IndexT> first_idx;
  std::optional<IndexT> last_idx;
  for (std::size_t r = 0; r < frame.rows(); ++r) {
    bool has_nan = false;
    for (std::size_t c = 0; c < frame.cols(); ++c) {
      double v = frame.value(r, c);
      if (!(v == v)) {
        has_nan = true;
        break;
      }
    }
    if (!has_nan) {
      if (!first_idx.has_value()) first_idx = frame.index()[r];
      last_idx = frame.index()[r];
      ++valid_rows;
    }
  }

  std::cout << '\n' << title << "\n";
  std::cout << "rows with complete data: " << valid_rows << '\n';
  std::cout << "first complete index: ";
  if (first_idx.has_value()) {
    std::cout << *first_idx;
  } else {
    std::cout << "NA";
  }
  std::cout << "\nlast complete index: ";
  if (last_idx.has_value()) {
    std::cout << *last_idx;
  } else {
    std::cout << "NA";
  }
  std::cout << '\n';
}

template <typename IndexT>
void print_column_autocorrelations(const DataFrame<IndexT>& frame,
                                   int max_lag,
                                   const std::string& title,
                                   int precision) {
  if (max_lag <= 0) {
    std::cout << "\n" << title << " (no lags requested)\n";
    return;
  }

  std::cout << "\n" << title << "\n";
  const int label_width = 12;
  const int value_width = 12;
  std::cout << std::setw(label_width) << "lag";
  for (const auto& name : frame.columns()) {
    std::cout << std::setw(value_width) << name;
  }
  std::cout << '\n';

  auto old_flags = std::cout.flags();
  auto old_precision = std::cout.precision();
  std::cout << std::fixed << std::setprecision(precision);

  for (int lag = 1; lag <= max_lag; ++lag) {
    std::cout << std::setw(label_width) << lag;
    for (std::size_t c = 0; c < frame.cols(); ++c) {
      std::vector<double> values;
      values.reserve(frame.rows());
      for (std::size_t r = 0; r < frame.rows(); ++r) {
        double v = frame.value(r, c);
        if (v == v) values.push_back(v);
      }
      if (static_cast<int>(values.size()) <= lag) {
        std::cout << std::setw(value_width) << 0.0;
        continue;
      }
      auto acfs = stats::autocorrelations(values, lag);
      double ac = (lag - 1 < static_cast<int>(acfs.size())) ? acfs[lag - 1] : 0.0;
      std::cout << std::setw(value_width) << ac;
    }
    std::cout << '\n';
  }

  std::cout.flags(old_flags);
  std::cout.precision(old_precision);
}

template <typename IndexT>
void print_column_percentiles(const DataFrame<IndexT>& frame,
                              const std::vector<double>& percentiles,
                              const std::string& title,
                              int precision) {
  if (percentiles.empty()) {
    std::cout << "\n" << title << " (no percentiles)\n";
    return;
  }
  auto percentile_df = frame.column_percentiles(percentiles);
  df::print::print_frame(percentile_df, title, false, precision);
}

template <typename IndexT>
void print_columns_header(const DataFrame<IndexT>& frame) {
  std::cout << std::setw(12) << frame.index_name();
  for (const auto& name : frame.columns()) {
    std::cout << ' ' << std::setw(12) << name;
  }
  std::cout << '\n';
}

template <typename IndexT>
void print_frame(const DataFrame<IndexT>& frame,
                 const std::string& title,
                 bool include_summary,
                 int precision) {
  std::cout << "\n" << title << '\n';
  print_columns_header(frame);
  const std::size_t total = frame.rows();
  const std::size_t max_print = 5;
  const bool use_window = total > 2 * max_print;
  auto old_flags = std::cout.flags();
  auto old_precision = std::cout.precision();
  std::cout << std::fixed << std::setprecision(precision);

  auto print_row = [&](std::size_t r) {
    std::cout << std::setw(12) << frame.index()[r];
    bool force_int = false;
    if constexpr (std::is_same_v<IndexT, std::string>) {
      force_int = (frame.index()[r] == "n");
    }
    for (std::size_t c = 0; c < frame.cols(); ++c) {
      std::cout << ' ' << std::setw(12);
      double value = frame.value(r, c);
      if (force_int) {
        std::cout << static_cast<long long>(std::llround(value));
      } else {
        if (std::fabs(value) >= 10000.0 || std::fabs(value) < 0.01) {
          if (value == 0.0) {
            std::cout << 0.0;
          } else {
            std::cout.setf(std::ios::scientific, std::ios::floatfield);
            std::cout << value;
            std::cout.setf(std::ios::fixed, std::ios::floatfield);
          }
        } else {
          std::cout << value;
        }
      }
    }
    std::cout << '\n';
  };

  if (!use_window) {
    for (std::size_t r = 0; r < total; ++r) print_row(r);
  } else {
    for (std::size_t r = 0; r < max_print; ++r) print_row(r);
    std::cout << "..." << '\n';
    for (std::size_t r = total - max_print; r < total; ++r) print_row(r);
  }

  std::cout.flags(old_flags);
  std::cout.precision(old_precision);

  if (include_summary) {
    print_column_summary(frame);
  }
}

}  // namespace print
}  // namespace df

#endif
