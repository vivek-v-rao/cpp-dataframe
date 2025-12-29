#include "print_utils.h"
#include "sample_utils.h"

#include <iostream>

int main() {
  try {
    auto intraday = samples::load_intraday_dataframe();
    df::print::print_frame(intraday.head_rows(5).select_columns({"Open", "High", "Low", "Close"}),
                           "intraday head",
                           false,
                           6);

    auto sorted = intraday.sort_rows_by_column("Close").head_rows(5);
    df::print::print_frame(sorted.select_columns({"Close", "Volume"}),
                           "sorted by close",
                           false,
                           6);

    auto rolling = intraday.select_columns({"Close"}).rolling_mean(3).head_rows(3);
    df::print::print_frame(rolling, "3-period rolling mean", false, 6);
  } catch (const std::exception& ex) {
    std::cerr << "x_intraday warning: " << ex.what() << "\n";
    return 0;
  }
  return 0;
}
