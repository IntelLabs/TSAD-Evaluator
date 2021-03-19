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

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <climits>

#include "evaluator.h"

using namespace std;
using namespace anomaly;

//----------------------------------------------------------------------------
// Given an input data file consisting of 0/1 anomaly labels,
// read it into a vector of anomaly time intervals as well
// as counting the total number of label entries in the file.
//----------------------------------------------------------------------------
time_intervals read_file(ifstream &data, int &count)
{
  time_intervals anomalies;

  int i = 0;
  int label;
  time_range range;
  bool range_started = false;

  while (data >> label)
  {
    data.ignore(INT_MAX, '\n'); // Ignore everything else other than label.

    if (label == 1) // Anomaly.
    {
      if (!range_started) 
      {
        range_started = true;
        range.first = i; 
        range.second = i;
      }
      else range.second = i;
    }
    else if (label == 0) // Not anomaly.
    {
      if (range_started)
      {
        anomalies.push_back(range);
        range_started = false;
      }
      else
      {
        // Nothing to do, keep reading.
      }
    }
    else 
    {
      throw "Error: Invalid anomaly label!";
    }

    ++i;
  }

  if (range_started) // Last read label was an anomaly.
  {
    anomalies.push_back(range);
    range_started = false;
  }

  count = i;

  return anomalies;
}

//----------------------------------------------------------------------------
// Given an input data file consisting of 0/1 anomaly labels,
// read it into a vector of unit-size anomaly time intervals as well
// as counting the total number of label entries in the file.
//----------------------------------------------------------------------------
time_intervals read_file_unitsize(ifstream &data, int &count)
{
  time_intervals anomalies;

  int i = 0;
  int label;
  time_range range;

  while (data >> label)
  {
    data.ignore(INT_MAX, '\n'); // Ignore everything else other than label.

    if (label == 1) // Anomaly.
    {
      range.first = i;
      range.second = i;
      anomalies.push_back(range);
    }
    else if (label == 0) // Not anomaly.
    {
      // Nothing to do, keep reading.
    }
    else
    {
      throw "Error: Invalid anomaly label!";
    }

    ++i;
  }

  count = i;

  return anomalies;
}

//----------------------------------------------------------------------------
// Given a positional bias value as of type string, convert it into
// its corresponding value of enumerated type positional_bias.
//----------------------------------------------------------------------------
positional_bias convert_bias(string bias)
{
  if (bias == "flat") return e_flat;
  else if (bias == "front") return e_front;
  else if (bias == "middle") return e_middle;
  else if (bias == "back") return e_back;
  else if (bias == "udf_delta") return e_udf_delta;
  else if (bias == "x") return e_flat; // I don't care. Set to default.

  throw "Error: Invalid positional bias value!";
}

//----------------------------------------------------------------------------
// Given an overlap cardinality value as of type string, convert it into
// its corresponding value of enumerated type overlap_cardinality.
//----------------------------------------------------------------------------
overlap_cardinality convert_cardinality(string cardinality)
{
  if (cardinality == "one") return e_one;
  else if (cardinality == "reciprocal") return e_reciprocal;
  else if (cardinality == "udf_gamma") return e_udf_gamma;
  else if (cardinality == "x") return e_one; // I don't care. Set to default.

  throw "Error: Invalid overlap cardinality value!";
}

//----------------------------------------------------------------------------
void output_usage(char *argv[])
{
  cout << endl;
  cout << "Usage: " << endl;
  cout << argv[0] 
       << " {-v} [-c | -t | -n] <real_data_file> <predicted_data_file>"
       << endl;
  cout << argv[0] 
       << " {-v} [-c | -t | -n] <real_data_file> <predicted_data_file>"
       << " <beta> <alpha_r> <gamma> <delta_p> <delta_r>" 
       << endl; 
  cout << "    -v        : " 
       << "Produce verbose output." 
       << endl;
  cout << "    -c        : " 
       << "Compute classical metrics." 
       << endl;
  cout << "    -t        : " 
       << "Compute time series metrics." 
       << endl;
  cout << "    -n        : " 
       << "Compute numenta-like metrics." 
       << endl;
  cout << "    <beta>    : " 
       << "F-Score parameter (relative importance of Recall vs. Precision)." 
       << endl;
  cout << "                " 
       << "Positive real number, Default = 1, Most common = 1" 
       << endl;
  cout << "    <alpha_r> : " 
       << "Relative weight of existence reward for Recall." 
       << endl;
  cout << "                " 
       << "Real number in [0 .. 1], Default = 0, Most common = 0" 
       << endl;
  cout << "    <gamma>   : " 
       << "Customizable overlap cardinality function for Precision&Recall." 
       << endl;
  cout << "                " 
       << "Values = {one, reciprocal, udf_gamma}" 
       << endl;
  cout << "                " 
       << "Default = one, Most common = reciprocal" 
       << endl;
  cout << "    <delta_p> : " 
       << "Customizable positional bias function for Precision." 
       << endl;
  cout << "                " 
       << "Values = {flat, front, middle, back, udf_delta}" 
       << endl;
  cout << "                " 
       << "Default = flat, Most common = flat" 
       << endl;
  cout << "    <delta_r> : " 
       << "Customizable positional bias function for Recall." 
       << endl;
  cout << "                " 
       << "Values = {flat, front, middle, back, udf_delta}" 
       << endl;
  cout << "                " 
       << "Default = flat, Most common = {flat, front, back}" 
       << endl;
  cout << endl;
  cout << "New to TSAD-Evaluator? Try:" 
       << endl;
  cout << argv[0] 
       << " -v -t ../examples/simple/simple.real "
       << "../examples/simple/simple.pred 1 0 reciprocal flat front" 
       << endl;
  cout << endl;
}

//----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  if ((argc != 4) && (argc != 9) && (argc !=5) && (argc != 10)) 
  {
    output_usage(argv);
    return 1;
  }

  bool verbose = false;
  int offset = 0;
  if ((argc == 5) || (argc == 10))
  {
    offset = 1;
    string verbose_option = argv[1];
    if (verbose_option == "-v")
    {
      verbose = true; 
    }
    else
    {
      cerr << "Error: Invalid verbose option!" << endl;
      return 1;
    }
  }

  ifstream real_data(argv[2+offset]);
  ifstream predicted_data(argv[3+offset]);

  if (!real_data.is_open() || !predicted_data.is_open())
  {
    cerr << "Error: Could not open file!" << endl;
    return 1;
  }

  int real_count = 0, predicted_count = 0;
  time_intervals real_anomalies, predicted_anomalies;

  string metric_option = argv[1+offset];  
  if (metric_option == "-c") // Classical metrics
  {
    try
    {
      real_anomalies = read_file_unitsize(real_data, real_count);
    }
    catch (const char* msg)
    {
      cerr << msg << endl;
      return 1;
    }

    try
    {
      predicted_anomalies = read_file_unitsize(predicted_data, predicted_count);
    }
    catch (const char* msg) 
    {
      cerr << msg << endl;
      return 1;
    }
  }
  else if (metric_option == "-t") // Time series metrics
  {
    try
    {
      real_anomalies = read_file(real_data, real_count);
    }
    catch (const char* msg) 
    {
      cerr << msg << endl;
      return 1;
    }

    try
    {
      predicted_anomalies = read_file(predicted_data, predicted_count);
    }
    catch (const char* msg) 
    {
      cerr << msg << endl;
      return 1;
    }
  }
  else if (metric_option == "-n") // Numenta-like metrics 
  {
    try
    {
      real_anomalies = read_file(real_data, real_count);
    }
    catch (const char* msg) 
    {
      cerr << msg << endl;
      return 1;
    }

    try
    {
      predicted_anomalies = read_file_unitsize(predicted_data, predicted_count);
    }
    catch (const char* msg) 
    {
      cerr << msg << endl;
      return 1;
    }
  }
  else 
  {
    cerr << "Error: Invalid metric option!" << endl;
    return 1;
  }
  real_data.close();
  predicted_data.close();

  if (real_count != predicted_count)
  {
    cerr << "Error: Number of data items are different!" << endl;
    return 1;
  }
  if (real_count == 0)
  {
    cerr << "Error: No data items!" << endl;
    return 1;
  }

  evaluator e;

  if ((argc == 4) || (argc == 5))
  {
    e = evaluator(real_anomalies, predicted_anomalies);
  }
  else
  {
    double beta = atof(argv[4+offset]);
    if (beta < 0)
    {
      cerr << "Error: Invalid beta value!" << endl;
      return 1;
    }

    double alpha_r = atof(argv[5+offset]);
    if ((alpha_r < 0) || (alpha_r > 1.0))
    {
      cerr << "Error: Invalid alpha_r value!" << endl;
      return 1;
    }

    overlap_cardinality gamma;
    try
    {
      gamma = convert_cardinality(argv[6+offset]);
    }
    catch (const char* msg)
    {
      cerr << msg << endl;
      return 1;
    }

    positional_bias delta_p;
    try
    {
      delta_p = convert_bias(argv[7+offset]);
    }
    catch (const char* msg)
    {
      cerr << msg << endl;
      return 1;
    }

    positional_bias delta_r;
    try
    {
      delta_r = convert_bias(argv[8+offset]);
    }
    catch (const char* msg)
    {
      cerr << msg << endl;
      return 1;
    }

    e = evaluator(real_anomalies, predicted_anomalies, 
                  beta, alpha_r, gamma, delta_p, delta_r);
  }

  if (verbose) // Print anomaly ranges.
  {
    e.print_real_anomalies();
    e.print_predicted_anomalies();
  }

  e.update_precision();
  e.update_recall();
  e.update_fscore();

  cout << "Precision = " << e.get_precision() << endl;
  cout << "Recall = " << e.get_recall() << endl;
  cout << "F-Score = " << e.get_fscore() << endl;

  return 0;
}

