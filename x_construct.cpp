#include "print_utils.h"
#include "sample_utils.h"

#include <iostream>

int main() {
  try {
    std::vector<df::Date> indices = {df::Date(2024, 1, 1),
                                     df::Date(2024, 1, 2),
                                     df::Date(2024, 1, 3)};
    std::vector<std::string> columns = {"Alpha", "Beta"};
    std::vector<std::vector<double>> data = {{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};

    auto frame = df::DataFrame<df::Date>::from_vectors(indices, columns, data);
    frame.set_index_name("CustomDate");
    df::print::print_frame(frame, "from_vectors", false);

    std::vector<double> gamma = {10.0, 20.0, 30.0};
    frame.add_column("Gamma", gamma);
    df::print::print_frame(frame, "after add_column", false);
  } catch (const std::exception& ex) {
    std::cerr << "x_construct error: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}
