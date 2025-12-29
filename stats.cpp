// stats.cpp
// doc: implementations for stats.h

#include "stats.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <ostream>
#include <stdexcept>

namespace stats {

double mean(const std::vector<double>& x) {
  // doc: arithmetic mean.
	const long long n = (long long)x.size();
	if (n <= 0) return std::numeric_limits<double>::quiet_NaN();
	double s = 0.0;
	for (long long i = 0; i < n; ++i) s += x[(size_t)i];
	return s / (double)n;
}

double stdev(const std::vector<double>& x) {
  // doc: sample stdev with denominator n-1.
	const long long n = (long long)x.size();
	if (n <= 1) return std::numeric_limits<double>::quiet_NaN();
	const double m = mean(x);
	double ss = 0.0;
	for (long long i = 0; i < n; ++i) {
		const double d = x[(size_t)i] - m;
		ss += d * d;
	}
	const double v = ss / (double)(n - 1);
	if (!(v >= 0.0)) return std::numeric_limits<double>::quiet_NaN();
	return std::sqrt(v);
}

double skew(const std::vector<double>& x) {
  // doc: population-moment skewness.
	const long long n = (long long)x.size();
	if (n <= 2) return std::numeric_limits<double>::quiet_NaN();
	const double m = mean(x);

	double m2 = 0.0, m3 = 0.0;
	for (long long i = 0; i < n; ++i) {
		const double d = x[(size_t)i] - m;
		const double d2 = d * d;
		m2 += d2;
		m3 += d2 * d;
	}
	m2 /= (double)n;
	m3 /= (double)n;

	if (!(m2 > 0.0)) return std::numeric_limits<double>::quiet_NaN();
	return m3 / std::pow(m2, 1.5);
}

double excess_kurtosis(const std::vector<double>& x) {
  // doc: population-moment excess kurtosis.
	const long long n = (long long)x.size();
	if (n <= 3) return std::numeric_limits<double>::quiet_NaN();
	const double m = mean(x);

	double m2 = 0.0, m4 = 0.0;
	for (long long i = 0; i < n; ++i) {
		const double d = x[(size_t)i] - m;
		const double d2 = d * d;
		m2 += d2;
		m4 += d2 * d2;
	}
	m2 /= (double)n;
	m4 /= (double)n;

	if (!(m2 > 0.0)) return std::numeric_limits<double>::quiet_NaN();
	return m4 / (m2 * m2) - 3.0;
}

std::vector<double> autocorrelations(const std::vector<double>& x, int k) {
  // doc: sample ACF for lags 1..k, mean-centered and normalized by sum (x_t-m)^2.
	const long long n = (long long)x.size();
	std::vector<double> r;

	if (k <= 0 || n <= 1) return r;
	if (k > (int)(n - 1)) k = (int)(n - 1);

	r.assign((size_t)k, std::numeric_limits<double>::quiet_NaN());

	const double m = mean(x);

	double denom = 0.0;
	for (long long t = 0; t < n; ++t) {
		const double d = x[(size_t)t] - m;
		denom += d * d;
	}
	if (!(denom > 0.0)) return r;

	for (int lag = 1; lag <= k; ++lag) {
		double num = 0.0;
		for (long long t = lag; t < n; ++t) {
			const double a = x[(size_t)t] - m;
			const double b = x[(size_t)(t - lag)] - m;
			num += a * b;
		}
		r[(size_t)(lag - 1)] = num / denom;
	}

	return r;
}

std::vector<double> simulate_ar1(long long n,
				 double phi,
				 double sigma_eps,
				 double mu,
				 long long burnin,
				 std::mt19937_64& rng) {
  // doc: simulate AR(1) using provided rng.
	if (n <= 0) throw std::runtime_error("simulate_ar1: n must be positive");
	if (burnin < 0) throw std::runtime_error("simulate_ar1: burnin must be >= 0");
	if (!(sigma_eps >= 0.0)) throw std::runtime_error("simulate_ar1: sigma must be >= 0");

	std::normal_distribution<double> ndist(0.0, 1.0);

	std::vector<double> out;
	out.reserve((size_t)n);

	double x0 = mu;
	const long long total = burnin + n;

	for (long long t = 0; t < total; ++t) {
		const double e = ndist(rng);
		x0 = mu + phi * (x0 - mu) + sigma_eps * e;
		if (t >= burnin) out.push_back(x0);
	}

	return out;
}

std::vector<double> simulate_ar1(long long n,
				 double phi,
				 double sigma_eps,
				 double mu,
				 long long burnin,
				 std::uint64_t seed) {
  // doc: simulate AR(1) using seed-initialized rng.
	std::mt19937_64 rng(seed);
	return simulate_ar1(n, phi, sigma_eps, mu, burnin, rng);
}

SummaryStats summary_stats(const std::vector<double>& x) {
  // doc: compute n, mean, sd, skew, excess kurtosis, min, max.
	SummaryStats s;
	std::vector<double> filtered;
	filtered.reserve(x.size());
	for (double v : x) {
		if (v == v) filtered.push_back(v);
	}
	s.n = (long long)filtered.size();

	if (s.n <= 0) {
		const double nan = std::numeric_limits<double>::quiet_NaN();
		s.mean = nan;
		s.sd = nan;
		s.skew = nan;
		s.ex_kurtosis = nan;
		s.min = nan;
		s.max = nan;
		return s;
	}

	s.mean = mean(filtered);
	s.sd = stdev(filtered);
	s.skew = skew(filtered);
	s.ex_kurtosis = excess_kurtosis(filtered);

	double mn = filtered[0];
	double mx = filtered[0];
	for (long long i = 1; i < s.n; ++i) {
		const double v = filtered[(size_t)i];
		if (v < mn) mn = v;
		if (v > mx) mx = v;
	}
	s.min = mn;
	s.max = mx;

	return s;
}

void print_summary(const std::vector<double>& x,
		   std::ostream& os,
		   int width,
		   int precision,
		   bool fixed,
		   bool print_header) {
  // doc: print aligned, space-delimited summary stats, with optional formatting controls.
	const SummaryStats s = summary_stats(x);

	std::ios::fmtflags old_flags = os.flags();
	std::streamsize old_prec = os.precision();

	if (fixed) {
		os.setf(std::ios::fixed, std::ios::floatfield);
	} else {
		os.setf(std::ios::scientific, std::ios::floatfield);
	}
	os << std::setprecision(precision);

	const int w_n = 10;
	const int w = (width < 8) ? 8 : width;

	if (print_header) {
		os << std::setw(w_n) << "n" << " "
				<< std::setw(w)   << "mean" << " "
				<< std::setw(w)   << "sd" << " "
				<< std::setw(w)   << "skew" << " "
				<< std::setw(w)   << "ex_kurtosis" << " "
				<< std::setw(w)   << "min" << " "
				<< std::setw(w)   << "max"
				<< "\n";
	}

	os << std::setw(w_n) << s.n << " "
			<< std::setw(w)   << s.mean << " "
			<< std::setw(w)   << s.sd << " "
			<< std::setw(w)   << s.skew << " "
			<< std::setw(w)   << s.ex_kurtosis << " "
			<< std::setw(w)   << s.min << " "
			<< std::setw(w)   << s.max
			<< "\n";

	os.flags(old_flags);
	os.precision(old_prec);
}


// doc: return elementwise returns[i]/cond_sd[i]; uses fill_value when cond_sd[i] is nonpositive or non-finite.
std::vector<double> standardize_returns(const std::vector<double>& returns,
                                        const std::vector<double>& cond_sd,
                                        double fill_value) {
	if (cond_sd.size() != returns.size()) {
		throw std::runtime_error("standardize_returns: size mismatch");
	}

	std::vector<double> out;
	out.reserve(returns.size());

	for (size_t i = 0; i < returns.size(); ++i) {
		const double s = cond_sd[i];
		if (s > 0.0 && std::isfinite(s)) {
			out.push_back(returns[i] / s);
		} else {
			out.push_back(fill_value);
		}
	}

	return out;
}

void print_autocorr_table(const std::vector<double>& x,
                          int max_lag,
                          std::ostream& os,
                          int width,
                          int precision,
                          bool print_header) {
    if (max_lag <= 0) return;

    std::vector<double> abs_vals;
    abs_vals.reserve(x.size());
    std::vector<double> sq_vals;
    sq_vals.reserve(x.size());
    for (double v : x) {
        abs_vals.push_back(std::fabs(v));
        sq_vals.push_back(v * v);
    }

    auto acf_returns = autocorrelations(x, max_lag);
    auto acf_abs = autocorrelations(abs_vals, max_lag);
    auto acf_sq = autocorrelations(sq_vals, max_lag);

    std::ios::fmtflags old_flags = os.flags();
    std::streamsize old_prec = os.precision();
    os.setf(std::ios::fixed, std::ios::floatfield);
    os << std::setprecision(precision);

    const int lag_width = 6;
    const int col_width = (width < 8) ? 8 : width;

    if (print_header) {
        os << std::setw(lag_width) << "lag" << " "
           << std::setw(col_width) << "returns" << " "
           << std::setw(col_width) << "|returns|" << " "
           << std::setw(col_width) << "returns^2" << "\n";
    }

    auto print_value = [&](double value) {
        if (std::isfinite(value)) {
            os << std::setw(col_width) << value;
        } else {
            os << std::setw(col_width) << "NA";
        }
    };

    for (int lag = 1; lag <= max_lag; ++lag) {
        os << std::setw(lag_width) << lag << " ";
        print_value(acf_returns[(size_t)(lag - 1)]);
        os << " ";
        print_value(acf_abs[(size_t)(lag - 1)]);
        os << " ";
        print_value(acf_sq[(size_t)(lag - 1)]);
        os << "\n";
    }

    os << std::setprecision(old_prec);
    os.flags(old_flags);
}

}  // namespace stats

