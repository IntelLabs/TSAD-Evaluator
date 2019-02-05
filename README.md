
# TSAD-Evaluator

This software package, publicly released under the MIT License, implements the customizable evaluation model for time series anomaly detection presented in the following paper:

**"Precision and Recall for Time Series"**, Nesime Tatbul, Tae Jun Lee, Stan Zdonik, Mejbah Alam, Justin Gottschlich, 32nd Annual Conference on Neural Information Processing Systems (NeurIPS'18), Montreal, Canada, December 2018. (https://arxiv.org/abs/1803.03639/)

## Building

```
cd src
make clean
make
```

## Running

There are two alternative ways to run TSAD-Evaluator:

```
./evaluate {-v} [-c | -t | -n] <real_data_file> <predicted_data_file>
```

```
./evaluate {-v} [-c | -t | -n] <real_data_file> <predicted_data_file> <beta> <alpha_r> <gamma> <delta_p>
<delta_r>
```

Here is a description of all command line options, inputs, and parameters:

```
-v : Produce verbose output.
-c : Compute classical metrics.
-t : Compute time series metrics.
-n : Compute numenta-like metrics.
<real_data_file> : File with real data labels.
<predicted_data_file> : File with predicted data labels. 
<beta> : F-Score parameter (relative importance of Recall vs. Precision).
         Positive real number, Default = 1, Most common = 1.
<alpha_r> : Relative weight of existence reward for Recall.
            Real number in [0 .. 1], Default = 0, Most common = 0.
<gamma> : Customizable overlap cardinality function for Precision&Recall.
          Values = {one, reciprocal, udf_gamma}.
          Default = one, Most common = reciprocal.
<delta_p> : Customizable positional bias function for Precision.
            Values = {flat, front, middle, back, udf_delta}.
            Default = flat, Most common = flat.
<delta_r> : Customizable positional bias function for Recall.
            Values = {flat, front, middle, back, udf_delta}.
            Default = flat, Most common = {flat, front, back}.
```

When no parameters are specified (like in the first usage above), then the following default values are used:

```
./evaluate {-v} [-c | -t | -n] <real_data_file> <predicted_data_file> 1 0 one flat flat 
```

To produce verbose output (i.e., to print the list of all real and predicted anomaly ranges), please use the `-v` option.

It is important to note that the use of `-v` is optional, whereas the metric option (`-c` or `-t` or `-n`) must always be specified. 

When the `-c` option is used, then both anomaly intervals are represented as unit-size intervals (i.e, as points), and one of the following parameter settings are expected:

```
./evaluate -c <real_data_file> <predicted_data_file> <beta> 0 one flat flat 
```

```
./evaluate -c <real_data_file> <predicted_data_file> <beta> 1 x x x 
```

In the second usage above, `x` means that the value of this parameter doesn't really matter.

When the `-t` option is used, then both anomaly intervals are represented as ranges, and parameters can be customly set as required by the application. For example:

```
./evaluate -t examples/simple/simple.real examples/simple/simple.pred 1 0 reciprocal flat front
```

When the `-n` option is used, then the predicted anomaly intervals are represented as unit-size intervals (i.e, as points) and the real anomaly intervals are represented as ranges, and one of the following parameter settings are expected:

```
./evaluate -n <real_data_file> <predicted_data_file> 1 0 one flat front
```

```
./evaluate -n <real_data_file> <predicted_data_file> 0.5 0 one flat front
```

```
./evaluate -n <real_data_file> <predicted_data_file> 2 0 one flat front
```

The first mimics Numenta-Standard, the second mimics Numenta-Reward-Low-FP, and
the third mimics Numenta-Reward-Low-FN, respectively.

## Additional Usage Notes

+ `<real_data_file>` and `<predicted_data_file>` are CSV files with 0/1 anomaly labels that correspond to each timestamp. In v1.0, these files contain only the labels, simply assuming label=1 at line=t indicates the presence of an anomaly at timestamp=t. More sophisticated input file formats can be supported in future releases. For example input files, please see: `examples/*/*.real` and `examples/*/*.pred`.  

+ An anomaly range is represented as a pair of timestamps `<start,end>`. In v1.0, we simply assume timestamps are integers `>= 0`. For example, `<1,5>` denotes the time period from 1 to 5, inclusive on both ends.

+ A collection of anomaly ranges is represented as a vector of anomaly ranges, ordered by ascending timestamps. For example, `[<1,5>,<10,12>]` denotes a vector of two anomalous ranges.

+ The application developer can provide user-defined functions for `<gamma>`, `<delta_p>`, and `<delta_r>`, if the pre-defined choices are not sufficient for his/her purpose. In order to do this, please select `<udf_gamma>` and/or `<udf_delta>` for the corresponding command line arguments above. Furthermore, please make sure to define these custom functions in `evaluator.cpp` by filling in the following templates provided in this file and rebuilding afterwards:

```
double evaluator::udf_gamma_def(int overlap_count, e_metric m)
double evaluator::udf_delta_def(timestamp t, timestamp anomaly_length, e_metric m)
```

## References

+ Paper: https://arxiv.org/abs/1803.03639/
+ Poster: https://people.csail.mit.edu/tatbul/talks/NeurIPS18_poster.pdf
+ Slides: https://people.csail.mit.edu/tatbul/talks/NeurIPS18_spotlight.pdf
+ Talk: https://youtu.be/1D8mC89k8e8?t=4978
+ Preview: https://www.youtube.com/watch?v=K5f-dUBiQP4
+ Blog post: https://ai.intel.com/precision-and-recall-for-time-series/

## Contact

+ Nesime Tatbul, Intel Labs, nesime.tatbul@intel.com
+ Justin Gottschlich, Intel Labs, justin.gottschlich@intel.com

