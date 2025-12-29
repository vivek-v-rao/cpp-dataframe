#include "print_utils.h"
#include "sample_utils.h"

#include <iostream>

int main() {
  try {
    auto prices = samples::load_prices_dataframe();
    const double return_scale = 100.0;
    std::cout << "\nreturn scaling factor: " << return_scale << "\n";
    auto returns = prices.proportional_changes().multiply(return_scale);
    df::print::print_frame(returns.head_rows(5).select_columns({"SPY", "EFA"}),
                           "returns head",
                           false,
                           6);

    auto stats_frame = returns.column_stats_dataframe();
    df::print::print_frame(stats_frame.head_rows(5), "summary stats", false, 4);

    auto corr = returns.correlation_matrix();
    df::print::print_frame(corr, "correlation matrix", false, 3);
    auto cov = returns.covariance_matrix();
    df::print::print_frame(cov, "covariance matrix", false, 6);

    auto rolling = returns.rolling_mean(5).head_rows(3).select_columns({"SPY", "EFA"});
    df::print::print_frame(rolling, "5-day rolling mean", false, 6);
  } catch (const std::exception& ex) {
    std::cerr << "x_stats error: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}
