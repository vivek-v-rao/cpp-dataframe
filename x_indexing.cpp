#include "print_utils.h"
#include "sample_utils.h"

#include <iostream>

int main() {
  try {
    auto prices = samples::load_prices_dataframe();
    const double return_scale = 100.0;
    std::cout << "\nreturn scaling factor: " << return_scale << "\n";
    auto returns = prices.proportional_changes().multiply(return_scale);

    auto range = returns.slice_rows_range(df::Date(2002, 1, 2), df::Date(2002, 1, 10));
    df::print::print_frame(range.select_columns({"SPY", "EFA"}),
                           "slice 2002-01-02..2002-01-10",
                           false,
                           6);

    auto first_rows = returns.head_rows(3);
    std::vector<df::Date> indices;
    for (const auto& idx : first_rows.index()) {
      indices.push_back(idx);
    }
    auto selected = returns.select_rows(indices).select_columns({"SPY", "TLT"});
    df::print::print_frame(selected, "selected rows", false, 6);

    auto sorted_by_spy = returns.sort_rows_by_column("SPY").head_rows(5);
    df::print::print_frame(sorted_by_spy.select_columns({"SPY", "EFA"}),
                           "sorted by SPY",
                           false,
                           6);

    if (!returns.index().empty()) {
      auto sorted_columns = returns.sort_columns_by_row(returns.index().front());
      df::print::print_frame(sorted_columns.head_rows(3),
                             "columns sorted by first row",
                             false,
                             6);
    }
  } catch (const std::exception& ex) {
    std::cerr << "x_indexing error: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}
