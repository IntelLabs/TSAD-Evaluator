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

#include "evaluator.h"

#include <assert.h>

using namespace anomaly;

//-----------------------------------------------------------------------------
void evaluator::print_real_anomalies()
{
  std::cout << "Real Anomalies:" << std::endl;
  for (auto i = real_anomalies_.begin(); i != real_anomalies_.end(); ++i) 
  {
      std::cout << "[" << i->first << ", " << i->second << "]" << std::endl;
  }
}

//-----------------------------------------------------------------------------
void evaluator::print_predicted_anomalies()
{
  std::cout << "Predicted Anomalies:" << std::endl;
  for (auto i = predicted_anomalies_.begin(); 
    i != predicted_anomalies_.end(); ++i) 
  {
      std::cout << "[" << i->first << ", " << i->second << "]" << std::endl;
  }
}

//-----------------------------------------------------------------------------
// User-defined gamma function to be implemented by the application developer.
// This function must be a single-variable polynomial which returns a value
// >= 1. The single variable corresponds to the "overlap_count", i.e., the
// number of distinct overlap ranges between a given range and a sequence of
// ranges.
//-----------------------------------------------------------------------------
double evaluator::udf_gamma_def(int overlap_count, e_metric m) const
{
  double return_val = 1.0; // Default return value is 1. 

  // Please define here, the function that sets the value of return_val.
  // Note: We are currently assuming that the gamma function must be the
  // same for all metrics. Otherwise, define multiple metric-specific
  // functions here instead based on the value of m. 
  /*
  ... 
  */

  assert(return_val >= 1.0); // This function must not return anything < 1.
  return return_val;
}

//-----------------------------------------------------------------------------
// User-defined delta function to be implemented by the application developer.
// This function should be one that returns a positive value (typically between
// 1 and anomaly length), for a given position t in the anomaly range.
// This value is expected to monotonically increase/decrease based on
// distance of t to a well-defined reference point in the anomaly range
// (e.g., start point, end point, mid point, etc).
//-----------------------------------------------------------------------------
double evaluator::udf_delta_def(timestamp t, timestamp anomaly_length,
  e_metric m) const
{
  double return_val = 1.0; // Default return value is 1. 

  if (m == e_precision)
  {
    // Please define here, the function that sets the value of return_val
    // for precision.
    /*
    ... 
    */
  }
  else if (m == e_recall)
  {
    // Please define here, the function that sets the value of return_val
    // for recall.
    /*
    ... 
    */
  }

  assert(return_val > 0); // This function must not return anything <= 0.
  return return_val;
}

//-----------------------------------------------------------------------------
double evaluator::gamma_select(overlap_cardinality const &gamma, int overlap,
  e_metric m, std::string const &prec_or_recall) const
{
  switch (gamma)
  {
    case e_one:
      return 1.0;
    case e_reciprocal:
      return ((overlap > 1) ? 1.0/overlap : 1.0);
    case e_udf_gamma:
      return ((overlap > 1) ? 1.0/udf_gamma_def(overlap, m) : 1.0);
    default:
      std::cout << "Warning: Invalid overlap cardinality function for " 
                << prec_or_recall.c_str() << " = " << gamma << std::endl;
      std::cout << "... using default value \"one\" instead..." << std::endl;
      return 1.0;
  }
}

//-----------------------------------------------------------------------------
double evaluator::gamma_function(int overlap, e_metric m) const
{
  switch (m)
  {
    case e_precision:
      return gamma_select(gamma_p_, overlap, m, "precision");
    case e_recall:
      return gamma_select(gamma_r_, overlap, m, "recall");
    default:
      std::cout << "Warning: Invalid metric \"" << m << "\" in gamma_function" 
                << std::endl << "...ignoring..." << std::endl;
      return 1.0;
  }
}

//-----------------------------------------------------------------------------
double evaluator::delta_select(positional_bias const &delta, timestamp t, 
  timestamp anomaly_length, e_metric m, std::string const &prec_or_recall) const
{
  switch (delta)
  {
    case e_flat:
      return 1.0;
    case e_front:
      return (double)(anomaly_length - t + 1);
    case e_middle:
      return ((t <= anomaly_length/2) ? (double)t
                                      : (double)(anomaly_length - t + 1));
    case e_back:
      return (double)t;
    case e_udf_delta:
      return udf_delta_def(t, anomaly_length, m);
    default:
      std::cout << "Warning: Invalid positional bias for " 
                << prec_or_recall.c_str() << " = " << delta << std::endl
                << "...ignoring and using default value \"flat\" instead..." 
                << std::endl;
      return 1.0;
  }
}

//-----------------------------------------------------------------------------
double evaluator::delta_function(timestamp t, timestamp anomaly_length, 
  e_metric m) const
{
  switch (m)
  {
    case e_precision: 
      return delta_select(delta_p_, t, anomaly_length, m, "precision");
    case e_recall: 
      return delta_select(delta_r_, t, anomaly_length, m, "recall");
    default:
      std::cout << "Warning: Invalid metric \"" << m << "\" in delta_function" 
                << std::endl << "...ignoring..." << std::endl;
      return 1.0;
  }
}

//-----------------------------------------------------------------------------
double evaluator::omega_function(time_range range, time_range overlap, 
  e_metric m) const
{ 
  timestamp anomaly_length = range.second - range.first + 1;
  double my_positional_bias = 0, max_positional_bias = 0, temp_bias = 0;
  timestamp i, j;

  for (i = 1; i <= anomaly_length; ++i)
  {
    temp_bias = delta_function(i, anomaly_length, m);
    max_positional_bias += temp_bias;

    j = range.first + i - 1;
    if ((j >= overlap.first) && (j <= overlap.second))
    {
      my_positional_bias = my_positional_bias + temp_bias;
    }
  }

  if (max_positional_bias > 0)
    return my_positional_bias / max_positional_bias;
  else
    return 0;
}    

//-----------------------------------------------------------------------------
double evaluator::compute_omega_reward(time_range r1, time_range r2,
  int& overlap_count, e_metric m) const
{
  if ((r1.second < r2.first) || (r1.first > r2.second)) return 0; // No overlap
  else 
  {
    ++overlap_count;

    time_range overlap;
    overlap.first = std::max(r1.first, r2.first);
    overlap.second = std::min(r1.second, r2.second); 

    return omega_function(r1, overlap, m); 
  }
}

//-----------------------------------------------------------------------------
double evaluator::compute_precision() const
{
  time_range range_p, range_r;
  double existence_reward, omega_reward, overlap_reward;
  int overlap_count;
  double precision = 0.0;

  if (predicted_anomalies_.size() == 0) return 0.0;

  for (auto i = predicted_anomalies_.begin();
            i != predicted_anomalies_.end(); ++i) 
  {
    range_p.first = i->first;
    range_p.second = i->second;        
    omega_reward = 0;
    overlap_count = 0;

    for (auto j = real_anomalies_.begin(); j != real_anomalies_.end(); ++j) 
    {
      range_r.first = j->first;
      range_r.second = j->second;

      omega_reward += compute_omega_reward(range_p, range_r, overlap_count,
                                           e_precision);
    }

    overlap_reward = gamma_function(overlap_count, e_precision) * omega_reward;
    existence_reward = (overlap_count > 0) ? 1 : 0;
    precision += alpha_p_ * existence_reward + 
                 (1.0 - alpha_p_) * overlap_reward;
  }

  return precision / predicted_anomalies_.size();
}

//-----------------------------------------------------------------------------
double evaluator::compute_recall() const
{
  time_range range_p, range_r;
  double existence_reward, omega_reward, overlap_reward;
  int overlap_count;
  double recall = 0.0;

  if (real_anomalies_.size() == 0) return 0.0;

  for (auto i = real_anomalies_.begin(); i != real_anomalies_.end(); ++i) 
  {
    range_r.first = i->first;
    range_r.second = i->second;
    omega_reward = 0;
    overlap_count = 0;

    for (auto j = predicted_anomalies_.begin();
              j != predicted_anomalies_.end(); ++j) 
    {
      range_p.first = j->first;
      range_p.second = j->second;

      omega_reward += compute_omega_reward(range_r, range_p, overlap_count,
                                           e_recall);
    }

    overlap_reward = gamma_function(overlap_count, e_recall) * omega_reward;
    existence_reward = (overlap_count > 0) ? 1 : 0;
    recall += alpha_r_ * existence_reward + (1.0 - alpha_r_) * overlap_reward;
  }

  return recall / real_anomalies_.size();
}

