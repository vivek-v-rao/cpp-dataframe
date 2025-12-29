#ifndef DATAFRAME_DATAFRAME_H
#define DATAFRAME_DATAFRAME_H

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <istream>
#include <limits>
#include <numeric>
#include <random>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "date_utils.h"
#include "stats.h"

namespace df {

namespace detail {

inline std::string trim(const std::string& s) {
  std::size_t start = 0;
  std::size_t end = s.size();
  while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
  return s.substr(start, end - start);
}

inline std::vector<std::string> split_csv(const std::string& line) {
  std::vector<std::string> fields;
  std::string field;
  std::istringstream ss(line);
  while (std::getline(ss, field, ',')) {
    fields.push_back(trim(field));
  }
  if (!line.empty() && line.back() == ',') {
    fields.emplace_back();
  }
  return fields;
}

template <typename T>
T parse_token(const std::string& token) {
  if constexpr (std::is_same_v<T, std::string>) {
    return token;
  } else if constexpr (std::is_same_v<T, Date>) {
    return io::parse_iso_date(token);
  } else if constexpr (std::is_same_v<T, DateTime>) {
    return io::parse_iso_datetime(token);
  } else {
    std::istringstream ss(token);
    T value{};
    ss >> value;
    if (!ss || (ss >> std::ws && !ss.eof())) {
      throw std::runtime_error("failed to parse token");
    }
    return value;
  }
}

template <typename>
struct dependent_false : std::false_type {};

template <typename T>
void write_pod(std::ostream& os, const T& value) {
  os.write(reinterpret_cast<const char*>(&value), sizeof(T));
  if (!os) {
    throw std::runtime_error("dataframe::binary_write: failed to write data");
  }
}

template <typename T>
T read_pod(std::istream& is) {
  T value{};
  is.read(reinterpret_cast<char*>(&value), sizeof(T));
  if (!is) {
    throw std::runtime_error("dataframe::binary_read: failed to read data");
  }
  return value;
}

inline void write_string(std::ostream& os, const std::string& value) {
  std::uint64_t length = static_cast<std::uint64_t>(value.size());
  write_pod(os, length);
  if (length > 0) {
    os.write(value.data(), static_cast<std::streamsize>(length));
    if (!os) {
      throw std::runtime_error("dataframe::binary_write: failed to write string");
    }
  }
}

inline std::string read_string(std::istream& is) {
  std::uint64_t length = read_pod<std::uint64_t>(is);
  if (length > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    throw std::runtime_error("dataframe::binary_read: string too large");
  }
  std::string value(static_cast<std::size_t>(length), '\0');
  if (length > 0) {
    is.read(value.data(), static_cast<std::streamsize>(length));
    if (!is) {
      throw std::runtime_error("dataframe::binary_read: failed to read string");
    }
  }
  return value;
}

template <typename T>
void write_index_value(std::ostream& os, const T& value) {
  if constexpr (std::is_arithmetic_v<T>) {
    write_pod(os, value);
  } else if constexpr (std::is_same_v<T, std::string>) {
    write_string(os, value);
  } else if constexpr (std::is_same_v<T, Date>) {
    write_pod(os, value.year);
    write_pod(os, value.month);
    write_pod(os, value.day);
  } else if constexpr (std::is_same_v<T, DateTime>) {
    write_pod(os, value.year);
    write_pod(os, value.month);
    write_pod(os, value.day);
    write_pod(os, value.hour);
    write_pod(os, value.minute);
    write_pod(os, value.second);
  } else {
    static_assert(dependent_false<T>::value,
                  "dataframe::to_binary: unsupported index type");
  }
}

template <typename T>
T read_index_value(std::istream& is) {
  if constexpr (std::is_arithmetic_v<T>) {
    return read_pod<T>(is);
  } else if constexpr (std::is_same_v<T, std::string>) {
    return read_string(is);
  } else if constexpr (std::is_same_v<T, Date>) {
    Date value;
    value.year = read_pod<int>(is);
    value.month = read_pod<unsigned>(is);
    value.day = read_pod<unsigned>(is);
    return value;
  } else if constexpr (std::is_same_v<T, DateTime>) {
    DateTime value;
    value.year = read_pod<int>(is);
    value.month = read_pod<unsigned>(is);
    value.day = read_pod<unsigned>(is);
    value.hour = read_pod<unsigned>(is);
    value.minute = read_pod<unsigned>(is);
    value.second = read_pod<unsigned>(is);
    return value;
  } else {
    static_assert(dependent_false<T>::value,
                  "dataframe::from_binary: unsupported index type");
  }
}

template <typename T, typename = void>
struct is_orderable_index : std::false_type {};

template <typename T>
struct is_orderable_index<
    T,
    std::void_t<decltype(std::declval<const T&>() < std::declval<const T&>()),
                decltype(std::declval<const T&>() > std::declval<const T&>()),
                decltype(std::declval<const T&>() <= std::declval<const T&>()),
                decltype(std::declval<const T&>() >= std::declval<const T&>())>> : std::true_type {};

template <typename T>
std::string index_to_string(const T& value) {
  if constexpr (std::is_same_v<T, Date>) {
    return io::format_iso_date(value);
  } else if constexpr (std::is_same_v<T, DateTime>) {
    return io::format_iso_datetime(value);
  } else {
    std::ostringstream oss;
    oss << value;
    return oss.str();
  }
}

inline double compute_median(std::vector<double> values) {
  if (values.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  std::size_t mid = values.size() / 2;
  std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid), values.end());
  double median = values[mid];
  if (values.size() % 2 == 0) {
    double max_low = *std::max_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid));
    median = 0.5 * (median + max_low);
  }
  return median;
}

}  // namespace detail

template <typename IndexT>
class DataFrame {
 public:
  template <typename> friend class DataFrame;
  static DataFrame from_csv(std::istream& input, bool has_index);
  static DataFrame from_vectors(const std::vector<IndexT>& indices,
                                const std::vector<std::string>& columns,
                                const std::vector<std::vector<double>>& data);
  static DataFrame from_binary(std::istream& input);
  static DataFrame from_binary_file(const std::string& path);
  static DataFrame random_normal(std::size_t rows,
                                 const std::vector<std::string>& columns,
                                 double mean = 0.0,
                                 double stddev = 1.0,
                                 std::uint32_t seed = 0,
                                 double target_corr = 0.0);
  static DataFrame random_uniform(std::size_t rows,
                                  const std::vector<std::string>& columns,
                                  double min = 0.0,
                                  double max = 1.0,
                                  std::uint32_t seed = 0);

  void to_csv(std::ostream& output,
              bool include_header = true,
              bool include_index = true) const;
  void to_csv_file(const std::string& path,
                   bool include_header = true,
                   bool include_index = true) const;
  void to_binary(std::ostream& output) const;
  void to_binary_file(const std::string& path) const;

  DataFrame differences() const;
  DataFrame log_changes() const;
  DataFrame proportional_changes() const;
  DataFrame add(double value) const;
  DataFrame subtract(double value) const;
  DataFrame multiply(double value) const;
  DataFrame divide(double value) const;
  DataFrame add(const DataFrame& other) const;
  DataFrame subtract(const DataFrame& other) const;
  DataFrame multiply(const DataFrame& other) const;
  DataFrame divide(const DataFrame& other) const;
  DataFrame log_elements() const;
  DataFrame exp_elements() const;
  DataFrame power(double exponent) const;
  DataFrame power_int(int exponent) const;
  DataFrame standardize() const;
  DataFrame normalize() const;
  DataFrame select_rows(const std::vector<IndexT>& values) const;
  DataFrame select_columns(const std::vector<std::string>& names) const;
  void add_column(const std::string& name, const std::vector<double>& values);
  template <typename T = IndexT,
            typename = std::enable_if_t<detail::is_orderable_index<T>::value>>
  DataFrame slice_rows_range(IndexT start,
                             IndexT end,
                             bool inclusive_end = true) const;
  DataFrame head_rows(std::size_t count) const;
  DataFrame tail_rows(std::size_t count) const;
  DataFrame head_columns(std::size_t count) const;
  DataFrame tail_columns(std::size_t count) const;
  std::vector<double> column_data(const std::string& name) const;
  std::vector<double> row_data(const IndexT& index_value) const;
  void to_row_major(double* out, std::size_t row_stride = 0) const;
  void to_column_major(double* out, std::size_t column_stride = 0) const;
  DataFrame sort_rows_by_column(const std::string& column_name,
                                bool ascending = true) const;
  DataFrame sort_columns_by_row(const IndexT& index_value,
                                bool ascending = true) const;
  DataFrame rolling_mean(std::size_t window) const;
  DataFrame rolling_std(std::size_t window) const;
  DataFrame rolling_rms(std::size_t window) const;
  DataFrame exponential_moving_average(double alpha) const;
  DataFrame resample_rows(std::size_t sample_size = 0,
                          bool reset_index = true) const;
  DataFrame remove_rows_with_nan() const;
  DataFrame remove_columns_with_nan() const;
  DataFrame<std::string> column_stats_dataframe() const;
  DataFrame<std::string> correlation_matrix() const;
  DataFrame<std::string> spearman_correlation_matrix() const;
  DataFrame<std::string> kendall_tau_matrix() const;
  DataFrame<std::string> column_percentiles(const std::vector<double>& percentiles) const;
  DataFrame<std::string> covariance_matrix() const;

  std::size_t rows() const { return data_.size(); }
  std::size_t cols() const { return columns_.size(); }
  const std::vector<std::string>& columns() const { return columns_; }
  const std::vector<IndexT>& index() const { return index_; }
  const std::string& index_name() const { return index_name_; }
  void set_index_name(const std::string& name) { index_name_ = name; }
  std::vector<std::size_t> shape() const { return {rows(), cols()}; }

  double value(std::size_t row, std::size_t col) const;

 private:
  std::vector<std::string> columns_;
  std::vector<IndexT> index_;
  std::vector<std::vector<double>> data_;
  std::string index_name_ = "index";

  template <typename Func>
  DataFrame apply_scalar(Func func) const;

  template <typename Func>
  DataFrame apply_unary(Func func, const char* name) const;

  template <typename Func>
  DataFrame apply_binary(const DataFrame& other, Func func, const char* name) const;

  DataFrame select_rows_by_positions(const std::vector<std::size_t>& positions) const;

  DataFrame select_columns_by_positions(const std::vector<std::size_t>& positions) const;

  std::vector<std::size_t> find_row_positions_in_range(IndexT start,
                                                       IndexT end,
                                                       bool inclusive_end) const;

  std::size_t find_column_index(const std::string& name) const;

  std::size_t find_row_position(const IndexT& value) const;
};

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::from_csv(std::istream& input, bool has_index) {
  std::string header;
  if (!std::getline(input, header)) {
    throw std::runtime_error("dataframe::from_csv: missing header row");
  }
  auto header_fields = detail::split_csv(header);
  if (header_fields.empty()) {
    throw std::runtime_error("dataframe::from_csv: header has no columns");
  }

  std::size_t start_col = has_index ? 1 : 0;
  if (has_index && header_fields.size() < 2) {
    throw std::runtime_error("dataframe::from_csv: need at least one data column when reading indices");
  }

  DataFrame<IndexT> df;
  df.columns_.assign(header_fields.begin() + static_cast<std::ptrdiff_t>(start_col), header_fields.end());
  df.index_name_ = has_index ? header_fields[0] : "index";
  if (df.columns_.empty()) {
    throw std::runtime_error("dataframe::from_csv: no data columns found");
  }

  std::string line;
  while (std::getline(input, line)) {
    if (detail::trim(line).empty()) continue;
    auto fields = detail::split_csv(line);
    const std::size_t expected = df.columns_.size() + (has_index ? 1 : 0);
    if (fields.size() != expected) {
      throw std::runtime_error("dataframe::from_csv: row has unexpected number of columns");
    }

    IndexT idx{};
    std::size_t offset = 0;
    if (has_index) {
      try {
        idx = detail::parse_token<IndexT>(fields[0]);
      } catch (const std::exception&) {
        throw std::runtime_error("dataframe::from_csv: invalid index value");
      }
      offset = 1;
    } else {
      if constexpr (std::is_convertible_v<std::size_t, IndexT>) {
        idx = static_cast<IndexT>(df.index_.size());
      } else {
        throw std::runtime_error("dataframe::from_csv: index type cannot be auto-generated");
      }
    }

    std::vector<double> row;
    row.reserve(df.columns_.size());
    for (std::size_t c = 0; c < df.columns_.size(); ++c) {
      const std::string& token = fields[c + offset];
      if (token.empty()) {
        row.push_back(std::numeric_limits<double>::quiet_NaN());
        continue;
      }
      try {
        row.push_back(std::stod(token));
      } catch (const std::exception&) {
        throw std::runtime_error("dataframe::from_csv: invalid numeric value");
      }
    }

    df.index_.push_back(idx);
    df.data_.push_back(std::move(row));
  }

  return df;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::from_vectors(const std::vector<IndexT>& indices,
                                                  const std::vector<std::string>& columns,
                                                  const std::vector<std::vector<double>>& data) {
  if (columns.empty()) {
    throw std::runtime_error("dataframe::from_vectors: no columns provided");
  }
  if (indices.size() != data.size()) {
    throw std::runtime_error(
        "dataframe::from_vectors: index count must match row count");
  }
  std::size_t expected_cols = columns.size();
  for (const auto& name : columns) {
    if (name.empty()) {
      throw std::runtime_error("dataframe::from_vectors: column name cannot be empty");
    }
  }

  DataFrame<IndexT> df;
  df.columns_ = columns;
  df.index_ = indices;
  df.index_name_ = "index";
  df.data_.reserve(data.size());
  for (const auto& row : data) {
    if (row.size() != expected_cols) {
      throw std::runtime_error(
          "dataframe::from_vectors: row size does not match column count");
    }
    df.data_.push_back(row);
  }
  return df;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::from_binary(std::istream& input) {
  const char expected_magic[] = {'D', 'F', 'B', 'I', 'N', '1'};
  char magic[sizeof(expected_magic)];
  input.read(magic, sizeof(magic));
  if (!input || std::memcmp(magic, expected_magic, sizeof(expected_magic)) != 0) {
    throw std::runtime_error("dataframe::from_binary: invalid file header");
  }

  auto row_count = detail::read_pod<std::uint64_t>(input);
  auto col_count = detail::read_pod<std::uint64_t>(input);

  if (row_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
      col_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    throw std::runtime_error("dataframe::from_binary: dimensions too large");
  }

  DataFrame<IndexT> df;
  df.index_name_ = detail::read_string(input);

  std::uint64_t column_names = detail::read_pod<std::uint64_t>(input);
  if (column_names != col_count) {
    throw std::runtime_error("dataframe::from_binary: column metadata mismatch");
  }
  df.columns_.resize(static_cast<std::size_t>(col_count));
  for (std::size_t i = 0; i < df.columns_.size(); ++i) {
    df.columns_[i] = detail::read_string(input);
  }

  df.index_.resize(static_cast<std::size_t>(row_count));
  for (std::size_t i = 0; i < df.index_.size(); ++i) {
    df.index_[i] = detail::read_index_value<IndexT>(input);
  }

  df.data_.assign(static_cast<std::size_t>(row_count),
                  std::vector<double>(static_cast<std::size_t>(col_count), 0.0));
  for (std::size_t r = 0; r < df.data_.size(); ++r) {
    for (std::size_t c = 0; c < df.columns_.size(); ++c) {
      df.data_[r][c] = detail::read_pod<double>(input);
    }
  }

  return df;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::from_binary_file(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("dataframe::from_binary_file: unable to open file");
  }
  return from_binary(file);
}

template <typename IndexT>
void DataFrame<IndexT>::to_csv(std::ostream& output,
                               bool include_header,
                               bool include_index) const {
  if (!output.good()) {
    throw std::runtime_error("dataframe::to_csv: output stream is not writable");
  }

  if (include_header) {
    bool wrote_header_field = false;
    if (include_index) {
      output << index_name_;
      wrote_header_field = true;
    }
    for (const auto& column_name : columns_) {
      if (wrote_header_field) output << ',';
      output << column_name;
      wrote_header_field = true;
    }
    output << '\n';
  }

  const std::size_t column_count = columns_.size();
  const std::size_t row_count = data_.size();
  if (include_index && index_.size() != row_count) {
    throw std::runtime_error("dataframe::to_csv: index size mismatch");
  }

  for (std::size_t r = 0; r < row_count; ++r) {
    if (data_[r].size() != column_count) {
      throw std::runtime_error("dataframe::to_csv: row has unexpected number of columns");
    }
    bool wrote_field = false;
    if (include_index) {
      output << detail::index_to_string(index_[r]);
      wrote_field = true;
    }
    for (std::size_t c = 0; c < column_count; ++c) {
      if (wrote_field) output << ',';
      const double value = data_[r][c];
      if (value == value) {
        output << value;
      }  // leave NaN fields empty to match from_csv semantics
      wrote_field = true;
    }
    output << '\n';
  }

  if (!output.good()) {
    throw std::runtime_error("dataframe::to_csv: failed while writing output");
  }
}

template <typename IndexT>
void DataFrame<IndexT>::to_csv_file(const std::string& path,
                                     bool include_header,
                                     bool include_index) const {
  std::ofstream file(path, std::ios::out | std::ios::trunc);
  if (!file.is_open()) {
    throw std::runtime_error("dataframe::to_csv_file: unable to open output file");
  }
  to_csv(file, include_header, include_index);
  if (!file.good()) {
    throw std::runtime_error("dataframe::to_csv_file: failed while writing output file");
  }
}

template <typename IndexT>
void DataFrame<IndexT>::to_binary(std::ostream& output) const {
  if (!output.good()) {
    throw std::runtime_error("dataframe::to_binary: output stream is not writable");
  }
  const char magic[] = {'D', 'F', 'B', 'I', 'N', '1'};
  output.write(magic, sizeof(magic));
  if (!output) {
    throw std::runtime_error("dataframe::to_binary: failed to write header");
  }

  detail::write_pod<std::uint64_t>(output, static_cast<std::uint64_t>(rows()));
  detail::write_pod<std::uint64_t>(output, static_cast<std::uint64_t>(cols()));
  detail::write_string(output, index_name_);
  detail::write_pod<std::uint64_t>(output, static_cast<std::uint64_t>(columns_.size()));
  for (const auto& name : columns_) {
    detail::write_string(output, name);
  }
  for (const auto& idx : index_) {
    detail::write_index_value(output, idx);
  }
  for (const auto& row : data_) {
    for (double value : row) {
      detail::write_pod(output, value);
    }
  }
  if (!output.good()) {
    throw std::runtime_error("dataframe::to_binary: failed while writing data");
  }
}

template <typename IndexT>
void DataFrame<IndexT>::to_binary_file(const std::string& path) const {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file.is_open()) {
    throw std::runtime_error("dataframe::to_binary_file: unable to open output file");
  }
  to_binary(file);
  if (!file.good()) {
    throw std::runtime_error("dataframe::to_binary_file: failed while writing file");
  }
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::random_normal(std::size_t rows,
                                                   const std::vector<std::string>& columns,
                                                   double mean,
                                                   double stddev,
                                                   std::uint32_t seed,
                                                   double target_corr) {
  static_assert(std::is_integral_v<IndexT>, "random_normal requires integral indices");
  if (columns.empty()) {
    throw std::runtime_error("random_normal: at least one column is required");
  }
  if (stddev <= 0.0) {
    throw std::runtime_error("random_normal: standard deviation must be positive");
  }
  if (target_corr < 0.0 || target_corr > 1.0) {
    throw std::runtime_error("random_normal: target correlation must be in [0, 1]");
  }

  if (rows > static_cast<std::size_t>(std::numeric_limits<IndexT>::max())) {
    throw std::runtime_error("random_normal: row count exceeds index capacity");
  }

  DataFrame<IndexT> df;
  df.columns_ = columns;
  df.index_name_ = "index";
  df.index_.reserve(rows);
  df.data_.reserve(rows);

  std::mt19937 rng(seed == 0 ? std::mt19937::result_type(std::random_device{}()) : seed);
  std::normal_distribution<double> dist(mean, stddev);

  if (df.columns_.size() <= 1 || target_corr == 0.0) {
    for (std::size_t row = 0; row < rows; ++row) {
      df.index_.push_back(static_cast<IndexT>(row));
      std::vector<double> row_values;
      row_values.reserve(df.columns_.size());
      for (std::size_t col = 0; col < df.columns_.size(); ++col) {
        row_values.push_back(dist(rng));
      }
      df.data_.push_back(std::move(row_values));
    }
    return df;
  }

  const double corr = target_corr;
  const double coeff1 = std::sqrt(corr);
  const double coeff2 = std::sqrt(1.0 - corr);

  for (std::size_t row = 0; row < rows; ++row) {
    df.index_.push_back(static_cast<IndexT>(row));
    std::vector<double> row_values(df.columns_.size(), 0.0);
    double common = dist(rng);
    row_values[0] = common;
    for (std::size_t col = 1; col < df.columns_.size(); ++col) {
      double independent = dist(rng);
      row_values[col] = coeff1 * common + coeff2 * independent;
    }
    df.data_.push_back(std::move(row_values));
  }

  return df;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::random_uniform(std::size_t rows,
                                                    const std::vector<std::string>& columns,
                                                    double min,
                                                    double max,
                                                    std::uint32_t seed) {
  static_assert(std::is_integral_v<IndexT>, "random_uniform requires integral indices");
  if (columns.empty()) {
    throw std::runtime_error("random_uniform: at least one column is required");
  }
  if (min >= max) {
    throw std::runtime_error("random_uniform: min must be less than max");
  }
  if (rows > static_cast<std::size_t>(std::numeric_limits<IndexT>::max())) {
    throw std::runtime_error("random_uniform: row count exceeds index capacity");
  }

  DataFrame<IndexT> df;
  df.columns_ = columns;
  df.index_name_ = "index";
  df.index_.reserve(rows);
  df.data_.reserve(rows);

  std::mt19937 rng(seed == 0 ? std::mt19937::result_type(std::random_device{}()) : seed);
  std::uniform_real_distribution<double> dist(min, max);

  for (std::size_t row = 0; row < rows; ++row) {
    df.index_.push_back(static_cast<IndexT>(row));
    std::vector<double> row_values;
    row_values.reserve(df.columns_.size());
    for (std::size_t col = 0; col < df.columns_.size(); ++col) {
      row_values.push_back(dist(rng));
    }
    df.data_.push_back(std::move(row_values));
  }

  return df;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::differences() const {
  if (data_.size() < 2) {
    throw std::runtime_error("dataframe::differences: need at least two rows");
  }
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_.assign(index_.begin() + 1, index_.end());
  out.index_name_ = index_name_;
  out.data_.resize(data_.size() - 1, std::vector<double>(columns_.size(), 0.0));
  for (std::size_t r = 1; r < data_.size(); ++r) {
    for (std::size_t c = 0; c < columns_.size(); ++c) {
      out.data_[r - 1][c] = data_[r][c] - data_[r - 1][c];
    }
  }
  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::log_changes() const {
  if (data_.size() < 2) {
    throw std::runtime_error("dataframe::log_changes: need at least two rows");
  }
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_.assign(index_.begin() + 1, index_.end());
  out.index_name_ = index_name_;
  out.data_.resize(data_.size() - 1, std::vector<double>(columns_.size(), 0.0));
  for (std::size_t r = 1; r < data_.size(); ++r) {
    for (std::size_t c = 0; c < columns_.size(); ++c) {
      double prev = data_[r - 1][c];
      double curr = data_[r][c];
      if (!(prev > 0.0) || !(curr > 0.0)) {
        throw std::runtime_error("dataframe::log_changes: non-positive value encountered");
      }
      out.data_[r - 1][c] = std::log(curr) - std::log(prev);
    }
  }
  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::proportional_changes() const {
  if (data_.size() < 2) {
    throw std::runtime_error("dataframe::proportional_changes: need at least two rows");
  }
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_.assign(index_.begin() + 1, index_.end());
  out.index_name_ = index_name_;
  out.data_.resize(data_.size() - 1, std::vector<double>(columns_.size(), 0.0));
  for (std::size_t r = 1; r < data_.size(); ++r) {
    for (std::size_t c = 0; c < columns_.size(); ++c) {
      double prev = data_[r - 1][c];
      double curr = data_[r][c];
      if (prev == 0.0) {
        throw std::runtime_error("dataframe::proportional_changes: zero value encountered");
      }
      out.data_[r - 1][c] = (curr - prev) / prev;
    }
  }
  return out;
}

template <typename IndexT>
DataFrame<std::string> DataFrame<IndexT>::column_stats_dataframe() const {
  static const std::vector<std::string> labels = {"n",       "median", "mean",
                                                  "sd",      "skew",   "ex_kurtosis",
                                                  "min",     "max"};
  DataFrame<std::string> out;
  out.columns_ = columns_;
  out.index_ = labels;
  out.index_name_ = "statistic";
  out.data_.assign(labels.size(), std::vector<double>(columns_.size(), 0.0));

  for (std::size_t c = 0; c < columns_.size(); ++c) {
    std::vector<double> values;
    values.reserve(rows());
    for (std::size_t r = 0; r < rows(); ++r) {
        double v = data_[r][c];
        if (!(v == v)) continue;
        values.push_back(v);
    }
    stats::SummaryStats summary = stats::summary_stats(values);
    double median = detail::compute_median(values);
    out.data_[0][c] = static_cast<double>(summary.n);
    out.data_[1][c] = median;
    out.data_[2][c] = summary.mean;
    out.data_[3][c] = summary.sd;
    out.data_[4][c] = summary.skew;
    out.data_[5][c] = summary.ex_kurtosis;
    out.data_[6][c] = summary.min;
    out.data_[7][c] = summary.max;
  }

  return out;
}

template <typename IndexT>
DataFrame<std::string> DataFrame<IndexT>::correlation_matrix() const {
  if (columns_.empty()) {
    throw std::runtime_error("dataframe::correlation_matrix: no columns");
  }
  if (rows() < 2) {
    throw std::runtime_error("dataframe::correlation_matrix: need at least two rows");
  }
  std::vector<std::size_t> valid_rows;
  valid_rows.reserve(rows());
  for (std::size_t r = 0; r < rows(); ++r) {
    bool has_nan = false;
    for (std::size_t c = 0; c < cols(); ++c) {
      const double v = data_[r][c];
      if (!(v == v)) {
        has_nan = true;
        break;
      }
    }
    if (!has_nan) valid_rows.push_back(r);
  }
  if (valid_rows.size() < 2) {
    throw std::runtime_error("dataframe::correlation_matrix: need at least two non-NaN rows");
  }
  DataFrame<std::string> out;
  out.columns_ = columns_;
  out.index_ = columns_;
  out.index_name_ = "column";
  out.data_.assign(columns_.size(), std::vector<double>(columns_.size(), 0.0));

  std::vector<double> means(columns_.size(), 0.0);
  for (std::size_t c = 0; c < columns_.size(); ++c) {
    for (std::size_t r_index : valid_rows) {
      means[c] += data_[r_index][c];
    }
    means[c] /= static_cast<double>(valid_rows.size());
  }

  std::vector<double> sds(columns_.size(), 0.0);
  for (std::size_t c = 0; c < columns_.size(); ++c) {
    double accum = 0.0;
    for (std::size_t r_index : valid_rows) {
      double diff = data_[r_index][c] - means[c];
      accum += diff * diff;
    }
    const double var = accum / static_cast<double>(valid_rows.size() - 1);
    sds[c] = (var > 0.0) ? std::sqrt(var) : 0.0;
  }

  const double nan = std::numeric_limits<double>::quiet_NaN();
  for (std::size_t i = 0; i < columns_.size(); ++i) {
    for (std::size_t j = 0; j < columns_.size(); ++j) {
      if (i == j) {
        out.data_[i][j] = 1.0;
        continue;
      }
      double accum = 0.0;
      for (std::size_t r_index : valid_rows) {
        accum += (data_[r_index][i] - means[i]) * (data_[r_index][j] - means[j]);
      }
      const double cov = accum / static_cast<double>(valid_rows.size() - 1);
      if (sds[i] <= 0.0 || sds[j] <= 0.0) {
        out.data_[i][j] = nan;
      } else {
        out.data_[i][j] = cov / (sds[i] * sds[j]);
      }
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<std::string> DataFrame<IndexT>::spearman_correlation_matrix() const {
  if (columns_.empty()) {
    throw std::runtime_error("dataframe::spearman_correlation_matrix: no columns");
  }
  if (rows() < 2) {
    throw std::runtime_error("dataframe::spearman_correlation_matrix: need at least two rows");
  }
  DataFrame<IndexT> ranked = *this;
  for (std::size_t c = 0; c < cols(); ++c) {
    std::vector<std::pair<double, std::size_t>> values;
    values.reserve(rows());
    for (std::size_t r = 0; r < rows(); ++r) {
      double v = data_[r][c];
      if (v == v) values.emplace_back(v, r);
    }
    if (values.size() < 2) {
      throw std::runtime_error("dataframe::spearman_correlation_matrix: insufficient data in column " + columns_[c]);
    }
    std::sort(values.begin(), values.end(), [](const auto& a, const auto& b) {
      if (a.first == b.first) return a.second < b.second;
      return a.first < b.first;
    });
    std::size_t i = 0;
    while (i < values.size()) {
      std::size_t j = i;
      double rank_sum = 0.0;
      while (j < values.size() && values[j].first == values[i].first) {
        rank_sum += static_cast<double>(j + 1);
        ++j;
      }
      double avg_rank = rank_sum / static_cast<double>(j - i);
      for (std::size_t k = i; k < j; ++k) {
        ranked.data_[values[k].second][c] = avg_rank;
      }
      i = j;
    }
    for (std::size_t r = 0; r < rows(); ++r) {
      if (!(data_[r][c] == data_[r][c])) {
        ranked.data_[r][c] = std::numeric_limits<double>::quiet_NaN();
      }
    }
  }
  return ranked.correlation_matrix();
}

template <typename IndexT>
DataFrame<std::string> DataFrame<IndexT>::kendall_tau_matrix() const {
  if (columns_.empty()) {
    throw std::runtime_error("dataframe::kendall_tau_matrix: no columns");
  }
  if (rows() < 2) {
    throw std::runtime_error("dataframe::kendall_tau_matrix: need at least two rows");
  }

  DataFrame<std::string> out;
  out.columns_ = columns_;
  out.index_ = columns_;
  out.index_name_ = "column";
  out.data_.assign(cols(), std::vector<double>(cols(), 0.0));

  for (std::size_t i = 0; i < cols(); ++i) {
    out.data_[i][i] = 1.0;
    for (std::size_t j = i + 1; j < cols(); ++j) {
      std::vector<std::pair<double, double>> pairs;
      pairs.reserve(rows());
      for (std::size_t r = 0; r < rows(); ++r) {
        double xi = data_[r][i];
        double xj = data_[r][j];
        if ((xi == xi) && (xj == xj)) {
          pairs.emplace_back(xi, xj);
        }
      }
      if (pairs.size() < 2) {
        out.data_[i][j] = out.data_[j][i] = std::numeric_limits<double>::quiet_NaN();
        continue;
      }
      long long concordant = 0;
      long long discordant = 0;
      for (std::size_t a = 0; a < pairs.size(); ++a) {
        for (std::size_t b = a + 1; b < pairs.size(); ++b) {
          const auto& pa = pairs[a];
          const auto& pb = pairs[b];
          const double diff_i = pa.first - pb.first;
          const double diff_j = pa.second - pb.second;
          if (diff_i == 0.0 || diff_j == 0.0) continue;
          if ((diff_i > 0 && diff_j > 0) || (diff_i < 0 && diff_j < 0)) {
            ++concordant;
          } else {
            ++discordant;
          }
        }
      }
      const long long total = concordant + discordant;
      if (total == 0) {
        out.data_[i][j] = out.data_[j][i] = std::numeric_limits<double>::quiet_NaN();
      } else {
        double tau = static_cast<double>(concordant - discordant) / static_cast<double>(total);
        out.data_[i][j] = out.data_[j][i] = tau;
      }
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<std::string> DataFrame<IndexT>::column_percentiles(
    const std::vector<double>& percentiles) const {
  if (columns_.empty()) {
    throw std::runtime_error("dataframe::column_percentiles: no columns");
  }
  if (percentiles.empty()) {
    throw std::runtime_error("dataframe::column_percentiles: no percentiles specified");
  }
  for (double p : percentiles) {
    if (p < 0.0 || p > 100.0) {
      throw std::runtime_error("dataframe::column_percentiles: percentiles must be in [0, 100]");
    }
  }

  DataFrame<std::string> out;
  std::vector<std::string> labels;
  labels.reserve(percentiles.size());
  for (double p : percentiles) {
    std::ostringstream oss;
    oss << p;
    labels.push_back(oss.str());
  }
  out.index_ = labels;
  out.index_name_ = "percentile";
  out.columns_ = columns_;
  out.data_.assign(percentiles.size(), std::vector<double>(columns_.size(), 0.0));

  for (std::size_t c = 0; c < cols(); ++c) {
    std::vector<double> values;
    values.reserve(rows());
    for (std::size_t r = 0; r < rows(); ++r) {
      double v = data_[r][c];
      if (v == v) values.push_back(v);
    }
    if (values.empty()) {
      for (std::size_t p_idx = 0; p_idx < percentiles.size(); ++p_idx) {
        out.data_[p_idx][c] = std::numeric_limits<double>::quiet_NaN();
      }
      continue;
    }
    std::sort(values.begin(), values.end());
    for (std::size_t p_idx = 0; p_idx < percentiles.size(); ++p_idx) {
      double percentile = percentiles[p_idx];
      if (percentile <= 0.0) {
        out.data_[p_idx][c] = values.front();
        continue;
      }
      if (percentile >= 100.0) {
        out.data_[p_idx][c] = values.back();
        continue;
      }
      double rank = (percentile / 100.0) * static_cast<double>(values.size() - 1);
      std::size_t lower = static_cast<std::size_t>(std::floor(rank));
      std::size_t upper = static_cast<std::size_t>(std::ceil(rank));
      double fraction = rank - static_cast<double>(lower);
      const double lower_value = values[lower];
      const double upper_value = values[upper];
      out.data_[p_idx][c] = lower_value + fraction * (upper_value - lower_value);
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<std::string> DataFrame<IndexT>::covariance_matrix() const {
  if (columns_.empty()) {
    throw std::runtime_error("dataframe::covariance_matrix: no columns");
  }
  if (rows() < 2) {
    throw std::runtime_error("dataframe::covariance_matrix: need at least two rows");
  }
  std::vector<std::size_t> valid_rows;
  valid_rows.reserve(rows());
  for (std::size_t r = 0; r < rows(); ++r) {
    bool has_nan = false;
    for (std::size_t c = 0; c < cols(); ++c) {
      const double v = data_[r][c];
      if (!(v == v)) {
        has_nan = true;
        break;
      }
    }
    if (!has_nan) valid_rows.push_back(r);
  }
  if (valid_rows.size() < 2) {
    throw std::runtime_error("dataframe::covariance_matrix: need at least two non-NaN rows");
  }

  DataFrame<std::string> out;
  out.columns_ = columns_;
  out.index_ = columns_;
  out.index_name_ = "column";
  out.data_.assign(columns_.size(), std::vector<double>(columns_.size(), 0.0));

  std::vector<double> means(columns_.size(), 0.0);
  for (std::size_t c = 0; c < columns_.size(); ++c) {
    for (std::size_t r_index : valid_rows) {
      means[c] += data_[r_index][c];
    }
    means[c] /= static_cast<double>(valid_rows.size());
  }

  for (std::size_t i = 0; i < columns_.size(); ++i) {
    for (std::size_t j = 0; j < columns_.size(); ++j) {
      double accum = 0.0;
      for (std::size_t r_index : valid_rows) {
        accum += (data_[r_index][i] - means[i]) * (data_[r_index][j] - means[j]);
      }
      out.data_[i][j] = accum / static_cast<double>(valid_rows.size() - 1);
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::add(double value) const {
  return apply_scalar([&](double v) { return v + value; });
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::subtract(double value) const {
  return apply_scalar([&](double v) { return v - value; });
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::multiply(double value) const {
  return apply_scalar([&](double v) { return v * value; });
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::divide(double value) const {
  if (value == 0.0) {
    throw std::runtime_error("dataframe::divide: division by zero");
  }
  return apply_scalar([&](double v) { return v / value; });
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::add(const DataFrame& other) const {
  return apply_binary(other, [](double a, double b) { return a + b; }, "add");
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::subtract(const DataFrame& other) const {
  return apply_binary(other, [](double a, double b) { return a - b; }, "subtract");
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::multiply(const DataFrame& other) const {
  return apply_binary(other, [](double a, double b) { return a * b; }, "multiply");
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::divide(const DataFrame& other) const {
  return apply_binary(other,
                      [](double a, double b) {
                        if (b == 0.0) {
                          throw std::runtime_error("dataframe::divide: division by zero element");
                        }
                        return a / b;
                      },
                      "divide");
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::log_elements() const {
  return apply_unary([](double v) {
    if (std::isnan(v)) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    if (!(v > 0.0)) {
      throw std::runtime_error("dataframe::log_elements: non-positive value encountered");
    }
    return std::log(v);
  }, "log_elements");
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::exp_elements() const {
  return apply_unary([](double v) { return std::exp(v); }, "exp_elements");
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::power(double exponent) const {
  return apply_unary([&](double v) { return std::pow(v, exponent); }, "power");
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::power_int(int exponent) const {
  return apply_unary([&](double v) { return std::pow(v, exponent); }, "power_int");
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::standardize() const {
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_ = index_;
  out.index_name_ = index_name_;
  out.data_.assign(rows(), std::vector<double>(cols(), 0.0));
  if (rows() == 0 || cols() == 0) {
    return out;
  }

  std::vector<double> means(cols(), 0.0);
  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < cols(); ++c) {
      means[c] += data_[r][c];
    }
  }
  for (double& m : means) {
    m /= static_cast<double>(rows());
  }

  std::vector<double> sds(cols(), 0.0);
  std::vector<std::size_t> counts(cols(), 0);
  for (std::size_t c = 0; c < cols(); ++c) {
    means[c] = 0.0;
  }
  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < cols(); ++c) {
      double v = data_[r][c];
      if (!(v == v)) continue;
      means[c] += v;
      ++counts[c];
    }
  }
  for (std::size_t c = 0; c < cols(); ++c) {
    if (counts[c] > 0) {
      means[c] /= static_cast<double>(counts[c]);
    } else {
      means[c] = std::numeric_limits<double>::quiet_NaN();
    }
  }

  for (std::size_t c = 0; c < cols(); ++c) {
    double accum = 0.0;
    if (counts[c] > 1) {
      for (std::size_t r = 0; r < rows(); ++r) {
        double v = data_[r][c];
        if (!(v == v)) continue;
        double diff = v - means[c];
        accum += diff * diff;
      }
      const double var = accum / static_cast<double>(counts[c] - 1);
      sds[c] = (var > 0.0) ? std::sqrt(var) : std::numeric_limits<double>::quiet_NaN();
    } else {
      sds[c] = std::numeric_limits<double>::quiet_NaN();
    }
  }

  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < cols(); ++c) {
      double value = data_[r][c];
      if (!(value == value) || !(means[c] == means[c]) || !(sds[c] == sds[c]) || sds[c] == 0.0) {
        out.data_[r][c] = std::numeric_limits<double>::quiet_NaN();
      } else {
        out.data_[r][c] = (value - means[c]) / sds[c];
      }
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::normalize() const {
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_ = index_;
  out.index_name_ = index_name_;
  out.data_.assign(rows(), std::vector<double>(cols(), 0.0));
  if (rows() == 0 || cols() == 0) {
    return out;
  }

  std::vector<double> mins(cols(), std::numeric_limits<double>::infinity());
  std::vector<double> maxs(cols(), -std::numeric_limits<double>::infinity());

  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < cols(); ++c) {
      double v = data_[r][c];
      if (!(v == v)) continue;
      mins[c] = std::min(mins[c], v);
      maxs[c] = std::max(maxs[c], v);
    }
  }

  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < cols(); ++c) {
      double v = data_[r][c];
      if (!(v == v)) {
        out.data_[r][c] = std::numeric_limits<double>::quiet_NaN();
        continue;
      }
      if (!std::isfinite(mins[c]) || !std::isfinite(maxs[c])) {
        out.data_[r][c] = std::numeric_limits<double>::quiet_NaN();
        continue;
      }
      const double spread = maxs[c] - mins[c];
      if (spread > 0.0) {
        out.data_[r][c] = (v - mins[c]) / spread;
      } else {
        out.data_[r][c] = 0.0;
      }
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::select_rows(const std::vector<IndexT>& values) const {
  std::vector<std::size_t> positions;
  positions.reserve(values.size());
  for (const auto& v : values) {
    auto it = std::find(index_.begin(), index_.end(), v);
    if (it == index_.end()) {
      throw std::runtime_error("dataframe::select_rows: requested index not found");
    }
    positions.push_back(static_cast<std::size_t>(it - index_.begin()));
  }
  return select_rows_by_positions(positions);
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::select_columns(const std::vector<std::string>& names) const {
  std::vector<std::size_t> positions;
  positions.reserve(names.size());
  for (const auto& name : names) {
    positions.push_back(find_column_index(name));
  }
  return select_columns_by_positions(positions);
}

template <typename IndexT>
void DataFrame<IndexT>::add_column(const std::string& name,
                                   const std::vector<double>& values) {
  if (std::find(columns_.begin(), columns_.end(), name) != columns_.end()) {
    throw std::runtime_error("dataframe::add_column: column already exists");
  }
  const std::size_t row_count = rows();
  if (row_count == 0) {
    if (!values.empty()) {
      throw std::runtime_error(
          "dataframe::add_column: cannot add data to an empty dataframe");
    }
    columns_.push_back(name);
    return;
  }
  if (values.size() != row_count) {
    throw std::runtime_error("dataframe::add_column: value count mismatch");
  }
  columns_.push_back(name);
  for (std::size_t r = 0; r < row_count; ++r) {
    data_[r].push_back(values[r]);
  }
}

template <typename IndexT>
template <typename T, typename>
DataFrame<IndexT> DataFrame<IndexT>::slice_rows_range(IndexT start,
                                                      IndexT end,
                                                      bool inclusive_end) const {
  auto positions = find_row_positions_in_range(start, end, inclusive_end);
  return select_rows_by_positions(positions);
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::head_rows(std::size_t count) const {
  if (count == 0) {
    std::vector<std::size_t> empty;
    return select_rows_by_positions(empty);
  }
  if (count >= rows()) {
    return *this;
  }
  std::vector<std::size_t> positions(count);
  for (std::size_t i = 0; i < count; ++i) positions[i] = i;
  return select_rows_by_positions(positions);
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::tail_rows(std::size_t count) const {
  if (count == 0) {
    std::vector<std::size_t> empty;
    return select_rows_by_positions(empty);
  }
  if (count >= rows()) {
    return *this;
  }
  std::vector<std::size_t> positions(count);
  const std::size_t start = rows() - count;
  for (std::size_t i = 0; i < count; ++i) positions[i] = start + i;
  return select_rows_by_positions(positions);
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::head_columns(std::size_t count) const {
  if (count == 0) {
    std::vector<std::size_t> empty;
    return select_columns_by_positions(empty);
  }
  if (count >= cols()) {
    return *this;
  }
  std::vector<std::size_t> positions(count);
  for (std::size_t i = 0; i < count; ++i) positions[i] = i;
  return select_columns_by_positions(positions);
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::tail_columns(std::size_t count) const {
  if (count == 0) {
    std::vector<std::size_t> empty;
    return select_columns_by_positions(empty);
  }
  if (count >= cols()) {
    return *this;
  }
  std::vector<std::size_t> positions(count);
  const std::size_t start = cols() - count;
  for (std::size_t i = 0; i < count; ++i) positions[i] = start + i;
  return select_columns_by_positions(positions);
}

template <typename IndexT>
std::vector<double> DataFrame<IndexT>::column_data(const std::string& name) const {
  std::size_t col = find_column_index(name);
  std::vector<double> values;
  values.reserve(rows());
  for (std::size_t r = 0; r < rows(); ++r) {
    values.push_back(data_[r][col]);
  }
  return values;
}

template <typename IndexT>
std::vector<double> DataFrame<IndexT>::row_data(const IndexT& index_value) const {
  std::size_t pos = find_row_position(index_value);
  return data_[pos];
}

template <typename IndexT>
void DataFrame<IndexT>::to_row_major(double* out, std::size_t row_stride) const {
  if (!out) {
    throw std::runtime_error("dataframe::to_row_major: output buffer is null");
  }
  const std::size_t row_count = rows();
  const std::size_t col_count = cols();
  if (row_count == 0 || col_count == 0) return;
  const std::size_t stride = (row_stride == 0) ? col_count : row_stride;
  if (stride < col_count) {
    throw std::runtime_error("dataframe::to_row_major: row_stride is too small");
  }
  for (std::size_t r = 0; r < row_count; ++r) {
    double* row_ptr = out + r * stride;
    for (std::size_t c = 0; c < col_count; ++c) {
      row_ptr[c] = data_[r][c];
    }
  }
}

template <typename IndexT>
void DataFrame<IndexT>::to_column_major(double* out, std::size_t column_stride) const {
  if (!out) {
    throw std::runtime_error("dataframe::to_column_major: output buffer is null");
  }
  const std::size_t row_count = rows();
  const std::size_t col_count = cols();
  if (row_count == 0 || col_count == 0) return;
  const std::size_t stride = (column_stride == 0) ? row_count : column_stride;
  if (stride < row_count) {
    throw std::runtime_error(
        "dataframe::to_column_major: column_stride is too small");
  }
  for (std::size_t c = 0; c < col_count; ++c) {
    double* col_ptr = out + c * stride;
    for (std::size_t r = 0; r < row_count; ++r) {
      col_ptr[r] = data_[r][c];
    }
  }
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::sort_rows_by_column(const std::string& column_name,
                                                         bool ascending) const {
  if (cols() == 0) {
    throw std::runtime_error("dataframe::sort_rows_by_column: no columns to sort by");
  }
  const std::size_t column_index = find_column_index(column_name);
  std::vector<std::size_t> order(rows());
  std::iota(order.begin(), order.end(), 0);
  auto comparator = [&](std::size_t lhs, std::size_t rhs) {
    double left_value = data_[lhs][column_index];
    double right_value = data_[rhs][column_index];
    const bool left_nan = !(left_value == left_value);
    const bool right_nan = !(right_value == right_value);
    if (left_nan || right_nan) {
      if (left_nan && right_nan) return lhs < rhs;
      if (ascending) {
        return !left_nan && right_nan;
      } else {
        return left_nan && !right_nan;
      }
    }
    return ascending ? (left_value < right_value) : (left_value > right_value);
  };
  std::stable_sort(order.begin(), order.end(), comparator);

  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_name_ = index_name_;
  out.index_.reserve(order.size());
  out.data_.reserve(order.size());
  for (std::size_t position : order) {
    out.index_.push_back(index_[position]);
    out.data_.push_back(data_[position]);
  }
  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::sort_columns_by_row(const IndexT& index_value,
                                                         bool ascending) const {
  if (cols() == 0) {
    throw std::runtime_error("dataframe::sort_columns_by_row: no columns to sort");
  }
  if (rows() == 0) {
    throw std::runtime_error("dataframe::sort_columns_by_row: no rows available");
  }
  const std::size_t row_position = find_row_position(index_value);
  std::vector<std::size_t> order(cols());
  std::iota(order.begin(), order.end(), 0);
  auto comparator = [&](std::size_t lhs, std::size_t rhs) {
    double left_value = data_[row_position][lhs];
    double right_value = data_[row_position][rhs];
    const bool left_nan = !(left_value == left_value);
    const bool right_nan = !(right_value == right_value);
    if (left_nan || right_nan) {
      if (left_nan && right_nan) return lhs < rhs;
      if (ascending) {
        return !left_nan && right_nan;
      } else {
        return left_nan && !right_nan;
      }
    }
    return ascending ? (left_value < right_value) : (left_value > right_value);
  };
  std::stable_sort(order.begin(), order.end(), comparator);

  DataFrame<IndexT> out;
  out.index_ = index_;
  out.index_name_ = index_name_;
  out.columns_.resize(cols());
  for (std::size_t i = 0; i < order.size(); ++i) {
    out.columns_[i] = columns_[order[i]];
  }
  out.data_.assign(rows(), std::vector<double>(cols(), 0.0));
  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < order.size(); ++c) {
      out.data_[r][c] = data_[r][order[c]];
    }
  }
  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::rolling_mean(std::size_t window) const {
  if (window == 0) {
    throw std::runtime_error("dataframe::rolling_mean: window must be positive");
  }
  if (window > rows()) {
    throw std::runtime_error("dataframe::rolling_mean: window exceeds row count");
  }
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_name_ = index_name_;
  out.index_.assign(index_.begin() + static_cast<std::ptrdiff_t>(window - 1), index_.end());
  out.data_.assign(rows() - window + 1, std::vector<double>(cols(), 0.0));

  std::vector<double> sums(cols(), 0.0);
  std::vector<int> valid_counts(cols(), 0);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < cols(); ++c) {
      double value = data_[r][c];
      const bool is_nan = !(value == value);
      if (!is_nan) {
        sums[c] += value;
        ++valid_counts[c];
      }
      if (r >= window) {
        double old_value = data_[r - window][c];
        const bool old_is_nan = !(old_value == old_value);
        if (!old_is_nan) {
          sums[c] -= old_value;
          --valid_counts[c];
        }
      }
      if (r + 1 >= window) {
        if (valid_counts[c] == static_cast<int>(window)) {
          out.data_[r + 1 - window][c] = sums[c] / static_cast<double>(window);
        } else {
          out.data_[r + 1 - window][c] = nan;
        }
      }
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::rolling_std(std::size_t window) const {
  if (window == 0) {
    throw std::runtime_error("dataframe::rolling_std: window must be positive");
  }
  if (window > rows()) {
    throw std::runtime_error("dataframe::rolling_std: window exceeds row count");
  }
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_name_ = index_name_;
  out.index_.assign(index_.begin() + static_cast<std::ptrdiff_t>(window - 1), index_.end());
  out.data_.assign(rows() - window + 1, std::vector<double>(cols(), 0.0));

  std::vector<double> sums(cols(), 0.0);
  std::vector<double> sums_sq(cols(), 0.0);
  std::vector<int> valid_counts(cols(), 0);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < cols(); ++c) {
      double value = data_[r][c];
      const bool is_nan = !(value == value);
      if (!is_nan) {
        sums[c] += value;
        sums_sq[c] += value * value;
        ++valid_counts[c];
      }
      if (r >= window) {
        double old = data_[r - window][c];
        const bool old_is_nan = !(old == old);
        if (!old_is_nan) {
          sums[c] -= old;
          sums_sq[c] -= old * old;
          --valid_counts[c];
        }
      }
      if (r + 1 >= window) {
        double result = nan;
        if (valid_counts[c] == static_cast<int>(window)) {
          if (window == 1) {
            result = 0.0;
          } else {
            double mean = sums[c] / static_cast<double>(window);
            double numerator = sums_sq[c] - sums[c] * mean;
            double variance = numerator / static_cast<double>(window - 1);
            if (variance < 0.0 && variance > -1e-12) {
              variance = 0.0;
            }
            result = (variance > 0.0) ? std::sqrt(variance) : 0.0;
          }
        }
        out.data_[r + 1 - window][c] = result;
      }
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::rolling_rms(std::size_t window) const {
  if (window == 0) {
    throw std::runtime_error("dataframe::rolling_rms: window must be positive");
  }
  if (window > rows()) {
    throw std::runtime_error("dataframe::rolling_rms: window exceeds row count");
  }
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_name_ = index_name_;
  out.index_.assign(index_.begin() + static_cast<std::ptrdiff_t>(window - 1), index_.end());
  out.data_.assign(rows() - window + 1, std::vector<double>(cols(), 0.0));
  if (cols() == 0) return out;

  std::vector<double> sums_sq(cols(), 0.0);
  std::vector<int> valid_counts(cols(), 0);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < cols(); ++c) {
      double value = data_[r][c];
      if (value == value) {
        sums_sq[c] += value * value;
        ++valid_counts[c];
      }
      if (r >= window) {
        double old = data_[r - window][c];
        if (old == old) {
          sums_sq[c] -= old * old;
          --valid_counts[c];
        }
      }
      if (r + 1 >= window) {
        if (valid_counts[c] == static_cast<int>(window)) {
          out.data_[r + 1 - window][c] = std::sqrt(sums_sq[c] / static_cast<double>(window));
        } else {
          out.data_[r + 1 - window][c] = nan;
        }
      }
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::exponential_moving_average(double alpha) const {
  if (!(alpha > 0.0) || !(alpha < 1.0)) {
    throw std::runtime_error(
        "dataframe::exponential_moving_average: alpha must be in (0,1)");
  }
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_ = index_;
  out.index_name_ = index_name_;
  out.data_.assign(rows(), std::vector<double>(cols(), std::numeric_limits<double>::quiet_NaN()));
  if (rows() == 0 || cols() == 0) return out;

  for (std::size_t c = 0; c < cols(); ++c) {
    double ema = std::numeric_limits<double>::quiet_NaN();
    bool has_ema = false;
    for (std::size_t r = 0; r < rows(); ++r) {
      double value = data_[r][c];
      if (!(value == value)) {
        out.data_[r][c] = std::numeric_limits<double>::quiet_NaN();
        continue;
      }
      if (!has_ema) {
        ema = value;
        has_ema = true;
      } else {
        ema = alpha * value + (1.0 - alpha) * ema;
      }
      out.data_[r][c] = ema;
    }
  }

  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::resample_rows(std::size_t sample_size,
                                                   bool reset_index) const {
  if (rows() == 0) {
    throw std::runtime_error("dataframe::resample_rows: no rows to sample");
  }
  if (sample_size == 0) sample_size = rows();
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_name_ = reset_index ? "resample_index" : index_name_;
  out.data_.reserve(sample_size);
  out.index_.reserve(sample_size);

  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<std::size_t> dist(0, rows() - 1);

  for (std::size_t i = 0; i < sample_size; ++i) {
    std::size_t pick = dist(rng);
    out.data_.push_back(data_[pick]);
    if constexpr (std::is_integral_v<IndexT>) {
      if (reset_index) {
        out.index_.push_back(static_cast<IndexT>(i));
      } else {
        out.index_.push_back(index_[pick]);
      }
    } else {
      out.index_.push_back(index_[pick]);
    }
  }

  if (reset_index && !std::is_integral_v<IndexT>) {
    out.index_name_ = index_name_;
  }

  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::remove_rows_with_nan() const {
  std::vector<std::size_t> keep_positions;
  for (std::size_t r = 0; r < rows(); ++r) {
    bool has_nan = false;
    for (double value : data_[r]) {
      if (std::isnan(value)) {
        has_nan = true;
        break;
      }
    }
    if (!has_nan) {
      keep_positions.push_back(r);
    }
  }
  return select_rows_by_positions(keep_positions);
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::remove_columns_with_nan() const {
  std::vector<std::size_t> keep_positions;
  for (std::size_t c = 0; c < cols(); ++c) {
    bool has_nan = false;
    for (std::size_t r = 0; r < rows(); ++r) {
      if (std::isnan(data_[r][c])) {
        has_nan = true;
        break;
      }
    }
    if (!has_nan) {
      keep_positions.push_back(c);
    }
  }
  return select_columns_by_positions(keep_positions);
}

template <typename IndexT>
template <typename Func>
DataFrame<IndexT> DataFrame<IndexT>::apply_scalar(Func func) const {
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_ = index_;
  out.index_name_ = index_name_;
  out.data_ = data_;
  for (auto& row : out.data_) {
    for (double& value : row) {
      value = func(value);
    }
  }
  return out;
}

template <typename IndexT>
template <typename Func>
DataFrame<IndexT> DataFrame<IndexT>::apply_unary(Func func, const char*) const {
  return apply_scalar(func);
}

template <typename IndexT>
template <typename Func>
DataFrame<IndexT> DataFrame<IndexT>::apply_binary(const DataFrame& other,
                                                  Func func,
                                                  const char* name) const {
  if (rows() != other.rows() || cols() != other.cols()) {
    throw std::runtime_error(std::string("dataframe::") + name + ": shape mismatch");
  }
  if (columns_ != other.columns_) {
    throw std::runtime_error(std::string("dataframe::") + name + ": column mismatch");
  }
  if (index_ != other.index_) {
    throw std::runtime_error(std::string("dataframe::") + name + ": index mismatch");
  }
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_ = index_;
  out.index_name_ = index_name_;
  out.data_.assign(rows(), std::vector<double>(cols(), 0.0));
  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < cols(); ++c) {
      out.data_[r][c] = func(data_[r][c], other.data_[r][c]);
    }
  }
  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::select_rows_by_positions(
    const std::vector<std::size_t>& positions) const {
  DataFrame<IndexT> out;
  out.columns_ = columns_;
  out.index_name_ = index_name_;
  out.index_.reserve(positions.size());
  out.data_.reserve(positions.size());
  for (std::size_t pos : positions) {
    if (pos >= rows()) {
      throw std::runtime_error("dataframe::select_rows: position out of bounds");
    }
    out.index_.push_back(index_[pos]);
    out.data_.push_back(data_[pos]);
  }
  return out;
}

template <typename IndexT>
DataFrame<IndexT> DataFrame<IndexT>::select_columns_by_positions(
    const std::vector<std::size_t>& positions) const {
  DataFrame<IndexT> out;
  out.index_ = index_;
  out.index_name_ = index_name_;
  out.columns_.reserve(positions.size());
  for (std::size_t pos : positions) {
    if (pos >= cols()) {
      throw std::runtime_error("dataframe::select_columns: position out of bounds");
    }
    out.columns_.push_back(columns_[pos]);
  }
  out.data_.assign(rows(), std::vector<double>(positions.size(), 0.0));
  for (std::size_t r = 0; r < rows(); ++r) {
    for (std::size_t c = 0; c < positions.size(); ++c) {
      out.data_[r][c] = data_[r][positions[c]];
    }
  }
  return out;
}

template <typename IndexT>
std::vector<std::size_t> DataFrame<IndexT>::find_row_positions_in_range(
    IndexT start,
    IndexT end,
    bool inclusive_end) const {
  std::vector<std::size_t> positions;
  if (rows() == 0) return positions;
  IndexT lo = start;
  IndexT hi = end;
  if (hi < lo) std::swap(lo, hi);
  for (std::size_t i = 0; i < index_.size(); ++i) {
    const bool lower_ok = index_[i] >= lo;
    const bool upper_ok = inclusive_end ? (index_[i] <= hi) : (index_[i] < hi);
    if (lower_ok && upper_ok) {
      positions.push_back(i);
    }
  }
  return positions;
}

template <typename IndexT>
std::size_t DataFrame<IndexT>::find_column_index(const std::string& name) const {
  for (std::size_t i = 0; i < columns_.size(); ++i) {
    if (columns_[i] == name) {
      return i;
    }
  }
  throw std::runtime_error("dataframe::select_columns: column not found");
}

template <typename IndexT>
std::size_t DataFrame<IndexT>::find_row_position(const IndexT& value) const {
  for (std::size_t i = 0; i < index_.size(); ++i) {
    if (index_[i] == value) {
      return i;
    }
  }
  throw std::runtime_error("dataframe::select_rows: index not found");
}

template <typename IndexT>
double DataFrame<IndexT>::value(std::size_t row, std::size_t col) const {
  if (row >= data_.size() || col >= columns_.size()) {
    throw std::out_of_range("dataframe::value: index out of range");
  }
  return data_[row][col];
}

using IntDataFrame = DataFrame<int>;
using StringDataFrame = DataFrame<std::string>;

}  // namespace df

#endif


