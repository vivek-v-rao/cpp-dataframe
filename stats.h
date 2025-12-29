#ifndef STATS_H
#define STATS_H

// doc: reusable statistics + autocorrelation + AR(1) simulation utilities.

#include <cstdint>
#include <iosfwd>
#include <random>
#include <vector>

namespace stats {

// doc: container for basic summary statistics.
struct SummaryStats {
	long long n;
	double mean;
	double sd;
	double skew;
	double ex_kurtosis;
	double min;
	double max;
};

// doc: compute the arithmetic mean of x; returns NaN for empty.
double mean(const std::vector<double>& x);

// doc: compute the sample standard deviation of x (denominator n-1); returns NaN if n<=1.
double stdev(const std::vector<double>& x);

// doc: compute skewness using population central moments: m3 / m2^(3/2); returns NaN if n<=2 or var<=0.
double skew(const std::vector<double>& x);

// doc: compute excess kurtosis using population central moments: m4/m2^2 - 3; returns NaN if n<=3 or var<=0.
double excess_kurtosis(const std::vector<double>& x);

// doc: return sample autocorrelations for lags 1..k (mean-centered); empty if k<=0; NaN values if undefined.
std::vector<double> autocorrelations(const std::vector<double>& x, int k);

// doc: simulate n observations from AR(1): x_t = mu + phi*(x_{t-1}-mu) + sigma_eps*e_t, using provided RNG.
std::vector<double> simulate_ar1(long long n,
				 double phi,
				 double sigma_eps,
				 double mu,
				 long long burnin,
				 std::mt19937_64& rng);

// doc: simulate n observations from AR(1) using a 64-bit seed to initialize RNG.
std::vector<double> simulate_ar1(long long n,
				 double phi,
				 double sigma_eps,
				 double mu,
				 long long burnin,
				 std::uint64_t seed);

// doc: compute n, mean, sd, skew, excess kurtosis, min, max for x.
SummaryStats summary_stats(const std::vector<double>& x);

// doc: print labels + one aligned, space-delimited line of stats (first column is n).
void print_summary(const std::vector<double>& x,
                   std::ostream& os,
                   int width = 16,
                   int precision = 10,
                   bool fixed = true,
                   bool print_header = true);

void print_autocorr_table(const std::vector<double>& x,
                          int max_lag,
                          std::ostream& os,
                          int width = 12,
                          int precision = 3,
                          bool print_header = true);

// doc: return elementwise returns[i]/cond_sd[i]; uses fill_value when cond_sd[i] is nonpositive or non-finite.
std::vector<double> standardize_returns(const std::vector<double>& returns,
					const std::vector<double>& cond_sd,
					double fill_value = 0.0);

}  // namespace stats

#endif
