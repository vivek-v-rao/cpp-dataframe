#include "print_utils.h"
#include "sample_utils.h"

#include <iostream>

int main() {
  try {
    auto prices = samples::load_prices_dataframe();
    auto subset = prices.head_rows(3);
    subset.to_csv_file("x_io_prices.csv");
    subset.to_binary_file("x_io_prices.bin");

    auto reloaded = df::DataFrame<df::Date>::from_binary_file("x_io_prices.bin");
    df::print::print_frame(reloaded, "binary reload", false, 6);

    std::vector<double> row_major(reloaded.rows() * reloaded.cols(), 0.0);
    reloaded.to_row_major(row_major.data());
    std::cout << "row-major dump:";
    for (std::size_t i = 0; i < row_major.size(); ++i) {
      std::cout << ' ' << row_major[i];
    }
    std::cout << "\n";

    std::vector<double> column_major(reloaded.rows() * reloaded.cols(), 0.0);
    reloaded.to_column_major(column_major.data());
    std::cout << "column-major dump:";
    for (std::size_t i = 0; i < column_major.size(); ++i) {
      std::cout << ' ' << column_major[i];
    }
    std::cout << "\n";
  } catch (const std::exception& ex) {
    std::cerr << "x_io error: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}
