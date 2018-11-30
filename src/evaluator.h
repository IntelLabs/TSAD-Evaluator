/*

The MIT License

Copyright (c) 2018 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.

*/

#ifndef EVALUATOR_H_
#define EVALUATOR_H_

#include <cmath>
#include <string>
#include <vector>
#include <iostream>

//-----------------------------------------------------------------------------
// All header code goes within the anomaly namespace to avoid naming collisions
//-----------------------------------------------------------------------------
namespace anomaly
{

typedef int timestamp; // Starts from 0 and represents label position in input.
typedef std::pair<timestamp, timestamp> time_range;
typedef std::vector<time_range> time_intervals;
typedef enum {e_one, e_reciprocal, e_udf_gamma} overlap_cardinality;
typedef enum {e_flat, e_front, e_middle, e_back, e_udf_delta} positional_bias;
typedef enum {e_precision, e_recall, e_fscore} e_metric;

class evaluator
{
public:

  //---------------------------------------------------------------------------
  // Constructors
  //---------------------------------------------------------------------------
  evaluator()
  : beta_(1), alpha_p_(0), alpha_r_(0), gamma_p_(e_one), gamma_r_(e_one),
    delta_p_(e_flat), delta_r_(e_flat), precision_(0), recall_(0), fscore_(0)
  {}

  evaluator(time_intervals const &real, time_intervals const &predicted)
  : beta_(1), alpha_p_(0), alpha_r_(0), gamma_p_(e_one), gamma_r_(e_one),
    delta_p_(e_flat), delta_r_(e_flat), precision_(0), recall_(0), fscore_(0),
    real_anomalies_(real), predicted_anomalies_(predicted)
  {}

  evaluator(time_intervals const &real, time_intervals const &predicted, 
    double const beta, double const alpha_r, overlap_cardinality const &gamma, 
    positional_bias const &delta_p, positional_bias const &delta_r)
  : beta_(beta), alpha_p_(0), alpha_r_(alpha_r), gamma_p_(gamma),
    gamma_r_(gamma), delta_p_(delta_p), delta_r_(delta_r),
    precision_(0), recall_(0), fscore_(0),
    real_anomalies_(real), predicted_anomalies_(predicted)
  {}

  void print_real_anomalies();
  void print_predicted_anomalies();

  //---------------------------------------------------------------------------
  // Getters - placed inside header for inline opt. potential by compiler
  //---------------------------------------------------------------------------
  double const & get_beta() const { return beta_; }
  double const & get_alpha_p() const { return alpha_p_; }
  double const & get_alpha_r() const { return alpha_r_; }
  overlap_cardinality const & get_gamma_p() { return gamma_p_; }
  overlap_cardinality const & get_gamma_r() { return gamma_r_; }
  positional_bias const & get_delta_p() const { return delta_p_; }
  positional_bias const & get_delta_r() const { return delta_r_; }
  double const & get_precision() const { return precision_; }
  double const & get_recall() const { return recall_; }
  double const & get_fscore() const { return fscore_; }

  //---------------------------------------------------------------------------
  // Updates call computers and *change* object state
  //---------------------------------------------------------------------------
  void update_precision() { precision_ = compute_precision(); }
  void update_recall() { recall_ = compute_recall(); }
  void update_fscore() { fscore_ = compute_fscore(); }

  //---------------------------------------------------------------------------
  // Computers are all const and *do not change* object state
  //---------------------------------------------------------------------------
  double compute_precision() const;
  double compute_recall() const;
  double compute_fscore() const
  {
    double beta_sqr = pow(beta_, 2.0);
    return (1 + beta_sqr) * (precision_ * recall_) /
           (beta_sqr * precision_ + recall_); 
  }

  //---------------------------------------------------------------------------
  // Setters
  //---------------------------------------------------------------------------
  void set_beta(double beta)
  {
    if (beta < 0) throw "Error: Invalid beta value!";
    beta_ = beta;
  }

  //---------------------------------------------------------------------------
  void set_alpha_r(double alpha)
  {
    if ((alpha < 0) || (alpha > 1.0)) throw "Error: Invalid alpha value!";
    alpha_r_ = alpha;
  }

  //---------------------------------------------------------------------------
  void set_gamma(overlap_cardinality const &gamma)
  {
    if ((gamma == e_one) || (gamma == e_reciprocal) || 
        (gamma == e_udf_gamma)) gamma_p_ = gamma_r_ = gamma;
    else throw "Error: Invalid overlap cardinality value!";
  }

  //---------------------------------------------------------------------------
  bool is_valid_bias(positional_bias const &bias)
  {
    if (bias == e_flat || bias == e_front || bias == e_middle ||
        bias == e_back || bias == e_udf_delta) return true;
    return false;    
  }
  
  //---------------------------------------------------------------------------
  void set_delta_p(positional_bias const &bias)
  {
    if (!is_valid_bias(bias)) throw "Error: Invalid positional bias value!";
    delta_p_ = bias;
  }

  //---------------------------------------------------------------------------
  void set_delta_r(positional_bias const &bias)
  {
    if (!is_valid_bias(bias)) throw "Error: Invalid positional bias value!";
    delta_r_ = bias;
  }

private:

  // Fixed function for omega
  double compute_omega_reward(time_range r1, time_range r2, 
    int& overlap_count, e_metric m) const;
  double omega_function(time_range range, time_range overlap, e_metric m) const;

  // Optional user-defined function (udf) for gamma
  double udf_gamma_def(int overlap_count, e_metric m) const;
  double gamma_function(int overlap_count, e_metric m) const;
  double gamma_select(overlap_cardinality const &gamma, int overlap,
    e_metric m, std::string const &prec_or_recall) const;

  // Optional user-defined function (udf) for delta
  double udf_delta_def(timestamp, timestamp, e_metric) const;
  double delta_function(timestamp, timestamp, e_metric) const;
  double delta_select(positional_bias const &, timestamp, timestamp, 
    e_metric, std::string const &) const;

  //---------------------------------------------------------------------------
  // Members
  //---------------------------------------------------------------------------
  double beta_; // Customizable F-Score parameter
 
  double alpha_p_; // Restriction: alpha_p_ = 0
  double alpha_r_; // Customizable existence weight

  overlap_cardinality gamma_p_; // Customizable overlap cardinality
  overlap_cardinality gamma_r_; // Restriction: gamma_r_ = gamma_p_

  positional_bias delta_p_; // Customizable positional bias
  positional_bias delta_r_; // Customizable positional bias

  double precision_;
  double recall_;
  double fscore_;

  time_intervals real_anomalies_;
  time_intervals predicted_anomalies_;
};

}

#endif // EVALUATOR_H_

