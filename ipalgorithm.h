#ifndef IPALGORITHM_H
#define IPALGORITHM_H

#include <chrono>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <string>
#include <vector>

// Very simple base class for image processing algorithms.
// An algorithm must subclass this class and implement the methods:
// Prologue: Function called before any processing is done (single-threaded).
// Algorithms can use this function to set up lookup-tables, etc...
// Process: Possibly multi-threaded function and forms the basis of an algorithm.
// Epilogue: Function called after the processing terminates (single-threaded).
// Algorithms can use this function to clean up temporary data structures, etc...
class IPAlgorithm {
protected:
  IPAlgorithm(const std::string& N, const std::string& D)
    : Name(N)
    , Desc(D) { }

public:
  void addInput(const cv::Mat& Mat) {
    this->inputs.push_back(Mat);
  }

  void addOutput(cv::Mat& Mat) {
    this->output = Mat;
  }

  virtual void Prologue() = 0;

  virtual void Process(int RowStart, int RowEnd) = 0;

  virtual void Epilogue() = 0;

  virtual void Run() {
    Prologue();
    auto start = std::chrono::system_clock::now();
    Process(0, inputs[0].rows - 1);
    auto end = std::chrono::system_clock::now();
    Epilogue();
    if (Verbose)
      std::cout << Name << " execution time: " <<
                   std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms\n";
  }

  void setVerbose(bool V) {
    Verbose = V;
  }

  virtual ~IPAlgorithm() { }
protected:
  // Name of the algorithm. Used in verbose mode to report usage statistics like
  // running time, etc...
  std::string Name;
  // Description of the algorithm
  std::string Desc;
  // Dump extra information like execution time
  bool Verbose;
  // List of inputs to the algorithm
  std::vector<cv::Mat> inputs;
  cv::Mat output;
};

// Special case of an image processing algorithm (reduction).
template <typename T>
class IPReduction : public IPAlgorithm {
public:
  std::vector<T> GetReductionData() const {
    return ReductionOutput;
  }

protected:
  IPReduction(const std::string& N, const std::string& Desc)
    : IPAlgorithm(N, Desc) { }
  std::vector<T> ReductionOutput;
};

#endif
