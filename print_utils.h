#ifndef DATAFRAME_PRINT_UTILS_H
#define DATAFRAME_PRINT_UTILS_H

#include "dataframe.h"

#include <string>

namespace df {
namespace print {

template <typename IndexT>
void print_frame(const DataFrame<IndexT>& frame,
                 const std::string& title,
                 bool include_summary = true,
                 int precision = 6);

template <typename IndexT>
void print_column_summary(const DataFrame<IndexT>& frame);

template <typename IndexT>
void print_column_summary_with_missing(const DataFrame<IndexT>& frame,
                                       const std::string& title =
                                           "column summary with missing data",
                                       int precision = 6);

template <typename IndexT>
void print_column_percentiles(const DataFrame<IndexT>& frame,
                              const std::vector<double>& percentiles,
                              const std::string& title = "column percentiles",
                              int precision = 6);

template <typename IndexT>
void print_row_validity_summary(const DataFrame<IndexT>& frame,
                                const std::string& title =
                                    "row completeness summary");

template <typename IndexT>
void print_column_autocorrelations(const DataFrame<IndexT>& frame,
                                   int max_lag,
                                   const std::string& title =
                                       "column autocorrelations",
                                   int precision = 3);

}  // namespace print
}  // namespace df

#include "print_utils.tcc"

#endif
