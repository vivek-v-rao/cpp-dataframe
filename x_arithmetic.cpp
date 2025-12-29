#include "print_utils.h"
#include "sample_utils.h"

#include <iostream>

int main() {
  try {
    auto prices = samples::load_prices_dataframe();
    auto subset = prices.select_columns({"SPY", "EFA"}).head_rows(5);

    auto plus = subset.add(2.0);
    auto minus = subset.subtract(1.0);
    auto scaled = subset.multiply(1.05);
    auto divided = subset.divide(2.0);

    df::print::print_frame(subset, "original subset", false);
    df::print::print_frame(plus, "+2", false);
    df::print::print_frame(minus, "-1", false);
    df::print::print_frame(scaled, "*1.05", false);
    df::print::print_frame(divided, "/2", false);

    auto logs = subset.log_elements();
    auto exp_back = logs.exp_elements();
    df::print::print_frame(logs, "log subset", false);
    df::print::print_frame(exp_back, "exp(log subset)", false);
  } catch (const std::exception& ex) {
    std::cerr << "x_arithmetic error: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}
