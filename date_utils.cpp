#include "date_utils.h"

#include <cctype>
#include <cstdio>
#include <stdexcept>

namespace {

bool is_leap_year(int year) {
  if (year % 400 == 0) return true;
  if (year % 100 == 0) return false;
  return (year % 4 == 0);
}

unsigned days_in_month(int year, unsigned month) {
  static const unsigned days_per_month[] = {31, 28, 31, 30, 31, 30,
                                            31, 31, 30, 31, 30, 31};
  if (month == 0 || month > 12) return 0;
  if (month == 2 && is_leap_year(year)) return 29;
  return days_per_month[month - 1];
}

bool is_valid_date(int year, unsigned month, unsigned day) {
  if (month == 0 || month > 12) return false;
  unsigned dim = days_in_month(year, month);
  return day >= 1 && day <= dim;
}

bool is_valid_time(unsigned hour, unsigned minute, unsigned second) {
  if (hour > 23) return false;
  if (minute > 59) return false;
  if (second > 59) return false;
  return true;
}

void ensure_digit(char ch, const std::string& context) {
  if (!std::isdigit(static_cast<unsigned char>(ch))) {
    throw std::runtime_error("invalid character in date/time: " + context);
  }
}

int parse_number(const std::string& text, std::size_t offset, std::size_t count) {
  return std::stoi(text.substr(offset, count));
}

}  // namespace

namespace df {

bool operator==(const Date& lhs, const Date& rhs) {
  return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day;
}

bool operator!=(const Date& lhs, const Date& rhs) {
  return !(lhs == rhs);
}

bool operator<(const Date& lhs, const Date& rhs) {
  if (lhs.year != rhs.year) return lhs.year < rhs.year;
  if (lhs.month != rhs.month) return lhs.month < rhs.month;
  return lhs.day < rhs.day;
}

bool operator>(const Date& lhs, const Date& rhs) {
  return rhs < lhs;
}

bool operator<=(const Date& lhs, const Date& rhs) {
  return !(rhs < lhs);
}

bool operator>=(const Date& lhs, const Date& rhs) {
  return !(lhs < rhs);
}

bool operator==(const DateTime& lhs, const DateTime& rhs) {
  return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day &&
         lhs.hour == rhs.hour && lhs.minute == rhs.minute && lhs.second == rhs.second;
}

bool operator!=(const DateTime& lhs, const DateTime& rhs) {
  return !(lhs == rhs);
}

bool operator<(const DateTime& lhs, const DateTime& rhs) {
  if (lhs.year != rhs.year) return lhs.year < rhs.year;
  if (lhs.month != rhs.month) return lhs.month < rhs.month;
  if (lhs.day != rhs.day) return lhs.day < rhs.day;
  if (lhs.hour != rhs.hour) return lhs.hour < rhs.hour;
  if (lhs.minute != rhs.minute) return lhs.minute < rhs.minute;
  return lhs.second < rhs.second;
}

bool operator>(const DateTime& lhs, const DateTime& rhs) {
  return rhs < lhs;
}

bool operator<=(const DateTime& lhs, const DateTime& rhs) {
  return !(rhs < lhs);
}

bool operator>=(const DateTime& lhs, const DateTime& rhs) {
  return !(lhs < rhs);
}

std::ostream& operator<<(std::ostream& os, const Date& value) {
  os << io::format_iso_date(value);
  return os;
}

std::ostream& operator<<(std::ostream& os, const DateTime& value) {
  os << io::format_iso_datetime(value);
  return os;
}

namespace io {

Date parse_iso_date(const std::string& iso_date) {
  if (iso_date.size() != 10 || iso_date[4] != '-' || iso_date[7] != '-') {
    throw std::runtime_error("invalid date format: " + iso_date);
  }
  for (std::size_t i = 0; i < iso_date.size(); ++i) {
    if (i == 4 || i == 7) continue;
    ensure_digit(iso_date[i], iso_date);
  }
  int year = parse_number(iso_date, 0, 4);
  unsigned month = static_cast<unsigned>(parse_number(iso_date, 5, 2));
  unsigned day = static_cast<unsigned>(parse_number(iso_date, 8, 2));
  if (!is_valid_date(year, month, day)) {
    throw std::runtime_error("invalid calendar date: " + iso_date);
  }
  return Date(year, month, day);
}

DateTime parse_iso_datetime(const std::string& iso_datetime) {
  if (iso_datetime.size() < 19) {
    throw std::runtime_error("invalid datetime format: " + iso_datetime);
  }
  if (iso_datetime[4] != '-' || iso_datetime[7] != '-') {
    throw std::runtime_error("invalid datetime delimiters: " + iso_datetime);
  }
  if (iso_datetime[10] != ' ' && iso_datetime[10] != 'T') {
    throw std::runtime_error("invalid datetime separator: " + iso_datetime);
  }
  if (iso_datetime[13] != ':' || iso_datetime[16] != ':') {
    throw std::runtime_error("invalid time delimiters: " + iso_datetime);
  }
  for (std::size_t i = 0; i < 19; ++i) {
    if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) continue;
    ensure_digit(iso_datetime[i], iso_datetime);
  }
  int year = parse_number(iso_datetime, 0, 4);
  unsigned month = static_cast<unsigned>(parse_number(iso_datetime, 5, 2));
  unsigned day = static_cast<unsigned>(parse_number(iso_datetime, 8, 2));
  unsigned hour = static_cast<unsigned>(parse_number(iso_datetime, 11, 2));
  unsigned minute = static_cast<unsigned>(parse_number(iso_datetime, 14, 2));
  unsigned second = static_cast<unsigned>(parse_number(iso_datetime, 17, 2));

  if (!is_valid_date(year, month, day)) {
    throw std::runtime_error("invalid calendar date: " + iso_datetime);
  }
  if (!is_valid_time(hour, minute, second)) {
    throw std::runtime_error("invalid time of day: " + iso_datetime);
  }

  // Accept optional timezone designators (e.g., Z, +HH:MM, -HH:MM) but do not
  // currently adjust the stored local time.
  if (iso_datetime.size() > 19) {
    std::size_t pos = 19;
    const char tz_marker = iso_datetime[pos];
    if (tz_marker == 'Z') {
      ++pos;
    } else if (tz_marker == '+' || tz_marker == '-') {
      if (iso_datetime.size() < pos + 6) {
        throw std::runtime_error("invalid timezone specifier: " + iso_datetime);
      }
      ensure_digit(iso_datetime[pos + 1], iso_datetime);
      ensure_digit(iso_datetime[pos + 2], iso_datetime);
      if (iso_datetime[pos + 3] != ':') {
        throw std::runtime_error("invalid timezone separator: " + iso_datetime);
      }
      ensure_digit(iso_datetime[pos + 4], iso_datetime);
      ensure_digit(iso_datetime[pos + 5], iso_datetime);
      pos += 6;
    } else {
      throw std::runtime_error("invalid timezone marker: " + iso_datetime);
    }
    if (pos != iso_datetime.size()) {
      throw std::runtime_error("unexpected characters after timezone: " + iso_datetime);
    }
  }

  return DateTime(year, month, day, hour, minute, second);
}

std::string format_iso_date(const Date& date) {
  if (!is_valid_date(date.year, date.month, date.day)) {
    throw std::runtime_error("cannot format invalid date");
  }
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d-%02u-%02u", date.year, date.month, date.day);
  return std::string(buf);
}

std::string format_iso_datetime(const DateTime& datetime) {
  if (!is_valid_date(datetime.year, datetime.month, datetime.day) ||
      !is_valid_time(datetime.hour, datetime.minute, datetime.second)) {
    throw std::runtime_error("cannot format invalid datetime");
  }
  char buf[32];
  std::snprintf(buf,
                sizeof(buf),
                "%04d-%02u-%02u %02u:%02u:%02u",
                datetime.year,
                datetime.month,
                datetime.day,
                datetime.hour,
                datetime.minute,
                datetime.second);
  return std::string(buf);
}

int parse_iso_date_to_int(const std::string& iso_date) {
  Date parsed = parse_iso_date(iso_date);
  return parsed.year * 10000 + static_cast<int>(parsed.month) * 100 +
         static_cast<int>(parsed.day);
}

std::string format_int_date(int yyyymmdd) {
  if (yyyymmdd <= 0) return std::to_string(yyyymmdd);
  int year = yyyymmdd / 10000;
  unsigned month = static_cast<unsigned>((yyyymmdd / 100) % 100);
  unsigned day = static_cast<unsigned>(yyyymmdd % 100);
  if (!is_valid_date(year, month, day)) {
    return std::to_string(yyyymmdd);
  }
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d-%02u-%02u", year, month, day);
  return std::string(buf);
}

}  // namespace io
}  // namespace df
