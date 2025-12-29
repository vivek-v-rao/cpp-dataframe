#ifndef DATAFRAME_DATE_UTILS_H
#define DATAFRAME_DATE_UTILS_H

#include <ostream>
#include <string>

namespace df {

struct Date {
  int year = 0;
  unsigned month = 0;
  unsigned day = 0;

  Date() = default;
  Date(int y, unsigned m, unsigned d) : year(y), month(m), day(d) {}
};

struct DateTime {
  int year = 0;
  unsigned month = 0;
  unsigned day = 0;
  unsigned hour = 0;
  unsigned minute = 0;
  unsigned second = 0;

  DateTime() = default;
  DateTime(int y,
           unsigned m,
           unsigned d,
           unsigned h,
           unsigned min,
           unsigned s)
      : year(y), month(m), day(d), hour(h), minute(min), second(s) {}
};

bool operator==(const Date& lhs, const Date& rhs);
bool operator!=(const Date& lhs, const Date& rhs);
bool operator<(const Date& lhs, const Date& rhs);
bool operator>(const Date& lhs, const Date& rhs);
bool operator<=(const Date& lhs, const Date& rhs);
bool operator>=(const Date& lhs, const Date& rhs);

bool operator==(const DateTime& lhs, const DateTime& rhs);
bool operator!=(const DateTime& lhs, const DateTime& rhs);
bool operator<(const DateTime& lhs, const DateTime& rhs);
bool operator>(const DateTime& lhs, const DateTime& rhs);
bool operator<=(const DateTime& lhs, const DateTime& rhs);
bool operator>=(const DateTime& lhs, const DateTime& rhs);

std::ostream& operator<<(std::ostream& os, const Date& value);
std::ostream& operator<<(std::ostream& os, const DateTime& value);

namespace io {

Date parse_iso_date(const std::string& iso_date);
DateTime parse_iso_datetime(const std::string& iso_datetime);
std::string format_iso_date(const Date& date);
std::string format_iso_datetime(const DateTime& datetime);
int parse_iso_date_to_int(const std::string& iso_date);
std::string format_int_date(int yyyymmdd);

}  // namespace io
}  // namespace df

#endif
