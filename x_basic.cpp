#include "print_utils.h"
#include "sample_utils.h"

#include <iostream>

int main() {
  try {
    auto prices = samples::load_prices_dataframe();
    auto shape = prices.shape();
    std::cout << "prices shape: (" << shape[0] << ", " << shape[1] << ")\n";
    std::cout << "columns:";
    for (const auto& name : prices.columns()) {
      std::cout << ' ' << name;
    }
    std::cout << "\n\nfirst rows\n";
    df::print::print_frame(prices.head_rows(3), "prices head", false);
    df::print::print_frame(prices.tail_rows(3), "prices tail", false);
  } catch (const std::exception& ex) {
    std::cerr << "x_basic error: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}
