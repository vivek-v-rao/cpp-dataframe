#include "print_utils.h"
#include "date_utils.h"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>

int main(int argc, char** argv) {
  std::string path = "prices_2000_on.csv";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--file" && i + 1 < argc) {
      path = argv[++i];
    } else if (arg == "--help") {
      std::cout << "Usage: df_demo [--file FILE]\n";
      return 0;
    }
  }

  std::ifstream input(path);
  if (!input) {
    std::cerr << "failed to open " << path << "\n";
    return 1;
  }

  std::string header;
  if (!std::getline(input, header)) {
    std::cerr << "empty file\n";
    return 1;
  }

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty()) lines.push_back(line);
  }
  if (lines.empty()) {
    std::cerr << "no data rows\n";
    return 1;
  }

  std::stringstream data_stream;
  if (header.find(',') == std::string::npos) {
    std::cerr << "header missing data columns\n";
    return 1;
  }
  data_stream << header << '\n';
  for (const auto& row : lines) {
    data_stream << row << '\n';
  }

  using DF = df::DataFrame<df::Date>;
  DF prices = DF::from_csv(data_stream, true);
  prices.set_index_name("Date");
  prices.to_csv_file("temp.csv");
  prices.to_csv_file("temp_no_indices.csv", true, false);
  prices.to_csv_file("temp_no_headings.csv", false, true);
  std::cout << "loaded prices dataframe with " << prices.rows() << " rows and "
            << prices.cols() << " columns\n";
  df::print::print_frame(prices, "price data", false);

  const double return_scale = 100.0;
  std::cout << "\nreturn scaling factor: " << return_scale << "\n";
  DF returns = prices.proportional_changes().multiply(return_scale);
  std::cout << "\ncomputed simple returns (proportional changes)\n";
  df::print::print_frame(returns, "returns", false);

  auto return_stats = returns.column_stats_dataframe();
  const int stats_precision = 4;
  df::print::print_frame(return_stats, "return statistics", false, stats_precision);
  df::print::print_column_summary_with_missing(returns,
                                               "return summary with missing data",
                                               stats_precision);
  std::vector<double> default_percentiles = {0, 1, 5, 25, 50, 75, 95, 99, 100};
  df::print::print_column_percentiles(returns,
                                      default_percentiles,
                                      "return percentiles",
                                      stats_precision);
  df::print::print_row_validity_summary(returns,
                                        "row completeness for returns");
  df::print::print_column_autocorrelations(returns,
                                           /*max_lag=*/5,
                                           "return autocorrelations",
                                           3);

  auto boot = returns.resample_rows();
  df::print::print_column_autocorrelations(boot,
                                           /*max_lag=*/5,
                                           "bootstrapped return autocorrelations",
                                           3);

  auto return_corr = returns.correlation_matrix();
  df::print::print_frame(return_corr, "return correlation matrix", false, 3);
  auto spearman_corr = returns.spearman_correlation_matrix();
  df::print::print_frame(spearman_corr, "return Spearman correlation", false, 3);
  auto kendall_tau = returns.kendall_tau_matrix();
  df::print::print_frame(kendall_tau, "return Kendall tau", false, 3);
  auto return_cov = returns.covariance_matrix();
  df::print::print_frame(return_cov, "return covariance matrix", false, 3);

  auto percent_returns = returns.head_rows(5).select_columns({"SPY", "EFA"});
  percent_returns = percent_returns.add(1.0).subtract(1.0);
  percent_returns = percent_returns.multiply(2.0).divide(2.0);
  df::print::print_frame(percent_returns, "returns (%) first rows", false);

  const auto& return_index = returns.index();
  auto sorted_by_spy = returns.sort_rows_by_column("SPY");
  df::print::print_frame(sorted_by_spy.head_rows(5),
                         "returns sorted by SPY",
                         false);

  if (!return_index.empty()) {
    auto columns_sorted = returns.sort_columns_by_row(return_index.front());
    df::print::print_frame(columns_sorted.head_rows(3),
                           "returns columns sorted by first row",
                           false);
  }

  {
    std::vector<df::Date> custom_indices = {
        df::Date(2025, 1, 1), df::Date(2025, 1, 2), df::Date(2025, 1, 3)};
    std::vector<std::string> custom_columns = {"Alpha", "Beta"};
    std::vector<std::vector<double>> custom_data = {{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};
    auto custom_frame = df::DataFrame<df::Date>::from_vectors(custom_indices,
                                                              custom_columns,
                                                              custom_data);
    custom_frame.set_index_name("CustomDate");
    df::print::print_frame(custom_frame, "custom dataframe from vectors", false);
  }

  auto standardized = returns.standardize().head_rows(5).select_columns({"SPY", "EFA"});
  df::print::print_frame(standardized, "standardized returns (z-scores)", false);

  auto normalized_tail = returns.normalize().tail_rows(5).select_columns({"SPY", "EFA"});
  df::print::print_frame(normalized_tail, "normalized returns (last rows)", false);

  auto range_slice = returns.slice_rows_range(df::Date(2003, 4, 15),
                                              df::Date(2003, 4, 22))
                        .select_columns({"SPY", "EFA"});
  df::print::print_frame(range_slice, "returns 2003-04-15..2003-04-22", false);

  if (!return_index.empty()) {
    std::vector<df::Date> endpoints = {return_index.front(), return_index.back()};
    auto endpoint_slice = returns.select_rows(endpoints).select_columns({"SPY", "TLT"});
    df::print::print_frame(endpoint_slice, "returns at endpoints", false);
  }

  auto log_price_preview = prices.head_rows(3).select_columns({"SPY", "TLT"}).log_elements();
  df::print::print_frame(log_price_preview, "log price preview", false);

  auto exp_preview = log_price_preview.exp_elements();
  df::print::print_frame(exp_preview, "exp(log price) preview", false);

  auto first_price_cols = prices.head_columns(2).head_rows(3);
  df::print::print_frame(first_price_cols, "first two price columns", false);

  auto last_price_cols = prices.tail_columns(2).head_rows(3);
  df::print::print_frame(last_price_cols, "last two price columns", false);

  auto spy_returns = returns.column_data("SPY");
  if (!spy_returns.empty()) {
    std::cout << "\nSPY returns sample: first=" << spy_returns.front()
              << ", last=" << spy_returns.back()
              << ", count=" << spy_returns.size() << "\n";
    std::vector<double> spy_squared = spy_returns;
    for (double& value : spy_squared) {
      value *= value;
    }
    DF returns_with_square = returns;
    returns_with_square.add_column("SPY_sq", spy_squared);
    auto spy_square_preview =
        returns_with_square.head_rows(3).select_columns({"SPY", "SPY_sq"});
    df::print::print_frame(spy_square_preview,
                           "SPY returns with squared column",
                           false,
                           6);

    std::vector<double> contiguous(returns.rows() * returns.cols(), 0.0);
    returns.to_row_major(contiguous.data());
    std::cout << "\nrow-major buffer sample: [" << contiguous[0] << ", "
              << contiguous[1] << ", " << contiguous[2] << ", ...]" << '\n';

    const std::string binary_path = "returns.bin";
    returns.to_binary_file(binary_path);
    DF returns_from_bin = DF::from_binary_file(binary_path);
    auto bin_preview = returns_from_bin.head_rows(3).select_columns({"SPY", "EFA"});
    df::print::print_frame(bin_preview,
                           "returns reloaded from binary",
                           false,
                           6);
  }

  if (!spy_returns.empty() && !return_index.empty()) {
    std::stringstream datetime_stream;
    datetime_stream << "timestamp,SPY_return\n";
    const std::size_t sample_count =
        std::min({spy_returns.size(), return_index.size(), static_cast<std::size_t>(5)});
    for (std::size_t i = 0; i < sample_count; ++i) {
      const df::Date& date = return_index[i];
      df::DateTime stamp(date.year, date.month, date.day, static_cast<unsigned>(i % 24), 0, 0);
      datetime_stream << df::io::format_iso_datetime(stamp) << ',' << spy_returns[i] << '\n';
    }
    auto datetime_frame = df::DataFrame<df::DateTime>::from_csv(datetime_stream, true);
    df::print::print_frame(datetime_frame,
                           "sample datetime-indexed returns",
                           false,
                           6);
  }

  if (!return_index.empty()) {
    auto first_row_data = returns.row_data(return_index.front());
    if (!first_row_data.empty()) {
      std::cout << "first row values: SPY=" << first_row_data[0];
      if (returns.cols() > 1) {
        std::cout << ", EFA=" << first_row_data[1];
      }
      std::cout << "\n";
    }
  }

  constexpr std::size_t window = 5;
  auto rolling_mean5 = returns.rolling_mean(window).head_rows(3).select_columns({"SPY", "EFA"});
  df::print::print_frame(rolling_mean5, "5-day rolling mean", false);

  auto rolling_std5 = returns.rolling_std(window).head_rows(3).select_columns({"SPY", "EFA"});
  df::print::print_frame(rolling_std5, "5-day rolling std", false);

  auto rolling_rms5 = returns.rolling_rms(window).head_rows(3).select_columns({"SPY", "EFA"});
  df::print::print_frame(rolling_rms5, "5-day rolling rms", false);

  auto ema = returns.exponential_moving_average(0.1).head_rows(3).select_columns({"SPY", "EFA"});
  df::print::print_frame(ema, "EMA(alpha=0.1) first rows", false);

  auto nan_subset = returns.head_rows(3).select_columns({"SPY", "EFA"});
  double nan_value = std::numeric_limits<double>::quiet_NaN();
  auto nan_data = nan_subset.add(nan_value);
  auto rows_clean = nan_data.remove_rows_with_nan();
  auto cols_clean = nan_data.remove_columns_with_nan();
  std::cout << "rows before NaN removal: " << nan_data.rows()
            << ", after: " << rows_clean.rows()
            << ", columns after dropping NaNs: " << cols_clean.cols() << "\n";

  const double target_corr = 0.7;
  std::cout << "\nrandom normal target correlation: " << target_corr << "\n";
  auto random_data = df::DataFrame<int>::random_normal(1000,
                                                       {"Alpha", "Beta", "Gamma"},
                                                       0.0,
                                                       1.0,
                                                       42,
                                                       target_corr);
  auto random_stats = random_data.column_stats_dataframe();
  df::print::print_frame(random_stats, "random normal stats", false);
  auto random_corr = random_data.correlation_matrix();
  df::print::print_frame(random_corr, "random normal correlations", false, 3);
  auto random_cov = random_data.covariance_matrix();
  df::print::print_frame(random_cov, "random normal covariances", false, 3);

  auto uniform_data = df::DataFrame<int>::random_uniform(5,
                                                         {"U1", "U2", "U3"},
                                                         0.0,
                                                         1.0,
                                                         99);
  df::print::print_frame(uniform_data, "random uniform sample", false, 4);

  auto shape = returns.shape();
  if (shape.size() == 2) {
    std::cout << "\nreturns shape: (" << shape[0] << ", " << shape[1] << ")\n";
  }

  {
    std::ifstream intraday_file("SPY_intraday.csv");
    if (!intraday_file) {
      std::cerr << "warning: could not open SPY_intraday.csv for intraday test\n";
    } else {
      std::string intraday_header;
      if (!std::getline(intraday_file, intraday_header)) {
        std::cerr << "warning: SPY_intraday.csv is empty\n";
      } else {
        std::vector<std::string> intraday_lines;
        std::string intraday_line;
        while (std::getline(intraday_file, intraday_line)) {
          if (!intraday_line.empty()) intraday_lines.push_back(intraday_line);
        }
        if (intraday_lines.empty()) {
          std::cerr << "warning: SPY_intraday.csv has no data rows\n";
        } else {
          std::stringstream intraday_stream;
          intraday_stream << intraday_header << '\n';
          for (const auto& row : intraday_lines) {
            intraday_stream << row << '\n';
          }
          auto intraday_data = df::DataFrame<df::DateTime>::from_csv(intraday_stream, true);
          intraday_data.set_index_name("Datetime");
          auto intraday_sample = intraday_data.head_rows(5).select_columns(
              {"Open", "High", "Low", "Close", "Volume"});
          df::print::print_frame(intraday_sample,
                                 "SPY intraday sample (first 5 rows)",
                                 false,
                                 6);
        }
      }
    }
  }

  return 0;
}
