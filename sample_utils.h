#ifndef DATAFRAME_SAMPLE_UTILS_H
#define DATAFRAME_SAMPLE_UTILS_H

#include "date_utils.h"
#include "dataframe.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace samples {

inline df::DataFrame<df::Date> load_prices_dataframe(const std::string& path = "prices_2000_on.csv") {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("samples::load_prices_dataframe: failed to open " + path);
  }
  std::string header;
  if (!std::getline(input, header)) {
    throw std::runtime_error("samples::load_prices_dataframe: empty file");
  }
  std::stringstream buffer;
  buffer << header << '\n';
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty()) buffer << line << '\n';
  }
  auto df = df::DataFrame<df::Date>::from_csv(buffer, true);
  df.set_index_name("Date");
  return df;
}

inline df::DataFrame<df::DateTime> load_intraday_dataframe(const std::string& path = "SPY_intraday.csv") {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("samples::load_intraday_dataframe: failed to open " + path);
  }
  std::string header;
  if (!std::getline(input, header)) {
    throw std::runtime_error("samples::load_intraday_dataframe: empty file");
  }
  std::stringstream buffer;
  buffer << header << '\n';
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty()) buffer << line << '\n';
  }
  auto df = df::DataFrame<df::DateTime>::from_csv(buffer, true);
  df.set_index_name("Datetime");
  return df;
}

}  // namespace samples

#endif
