#include "ipalgorithm.h"

#include <boost/program_options.hpp>
#include <cstdint>
#include <opencv2/highgui/highgui.hpp>
#include <vector>
#include <set>
#include <stack>
#include <string>
#include <utility>

class RLBP : public IPReduction<int64_t> {
public:
  RLBP(const std::string& N, const std::string& Desc)
     : IPReduction(N, Desc)
  { }

  bool isUniformPattern(uint8_t p)
  {
    int transitions = 0;
    unsigned bit = p & 1;
    for (unsigned i = 1; i < 8 && p != 0; ++i) {
      p >>= 1;
      if ((p & 1) != bit) {
        ++transitions;
        bit = p & 1;
      }
    }
    return transitions <= 2;
  }

  // Return all new values that can be created by replacing y3 and y6 3-bit
  // patterns by y3r and y6r.
  // y3 = (010); y3r = (000)
  // y6 = (101); y6r = (111)
  std::set<uint8_t> replaceY3AndY6(uint8_t n)
  {
    const uint8_t y3  = 2;
    const uint8_t y3r = 0;
    const uint8_t y6  = 5;
    const uint8_t y6r = 7;
    std::set<uint8_t> result;
    std::vector<bool> y3y6check(8);
    std::vector<uint8_t> y3y6replace(8);

    for (unsigned i = 0; i < 8; ++i) {
      if (i == y3) {
        y3y6check[i] = true;
        y3y6replace[i] = y3r;
        continue;
      } else if (i == y6) {
        y3y6check[i] = true;
        y3y6replace[i] = y6r;
        continue;
      }
      y3y6check[i] = false;
      y3y6replace[i] = i;
    }

    std::stack<std::pair<uint8_t, uint8_t>> work_queue;
    work_queue.push(std::make_pair(0, n));

    while (!work_queue.empty()) {
      std::pair<uint8_t, uint8_t> idx_number_pair = work_queue.top();
      work_queue.pop();
      for (uint8_t i = idx_number_pair.first; i < 8 - 2; ++i) {
        uint16_t nr = (idx_number_pair.second >> i) & 7;
        if (!y3y6check[nr])
          continue;
        // There's a change to be made so let's save the progress til now to
        // our working queue.
        work_queue.push(std::make_pair(i+1, idx_number_pair.second));
        // Replace the 3-bit sequence
        uint16_t new_seq = y3y6replace[nr];
        // Move the sequence to the right location
        new_seq <<= i;
        // Copy the bottom bits from the original sequence
        new_seq |= ((idx_number_pair.second << (8 - i)) & 0xff) >> (8 - i);
        // Copy the top bits from the original sequence
        new_seq |= ((idx_number_pair.second >> (i + 3))  & 0xff) << (i + 3);
        work_queue.push(std::make_pair(i+1, uint8_t(new_seq)));
      }
      if (idx_number_pair.second != n)
        result.insert(idx_number_pair.second);
    }

    return result;
  }

  virtual void Prologue() override
  {
    ReductionOutput.resize(256);
    LUTUniformHist.resize(256);
    // Initialise LUT that help us to easily create an uniform pattern histogram.
    // An uniform pattern is a sequence of binary digits that contains at most 2
    // 0-1 or 1-0 transitions (e.g. 11110011, 00110000, etc...).
    // More details can be found in
    // http://www.bmva.org/bmvc/2013/Papers/paper0122/paper0122.pdf
    for (int16_t i = 0, uniforms = 0; i < 256; ++i) {
      if (!isUniformPattern(i)) {
        LUTUniformHist[i] = 0;
        continue;
      }
      LUTUniformHist[i] = ++uniforms;
    }
  }


  // There are many ways to improve the performance of this function.
  // 1) Compiler usually do a good job optimising ternary operators but it's
  //    definitely better to rewrite the expressions so that we can use the sign
  //    bit of the subtraction to set 1 or 0
  // 2) The same pixel is being read more times than required because I'm not using
  //    any location/temporal optimisation. The adjacent pixels should be in the cache
  //    due to prefetch, etc.. but it's a good idea to minimise memory accesses.
  virtual void Process(int RowStart, int RowEnd) override
  {
    uchar Weights[3][3] = { {1, 2, 4}, {128, 0, 8}, {64, 32, 16} };
    for (unsigned R = RowStart + 1; R < RowEnd; ++R) {
      uchar* pminus1 = inputs[0].ptr<uchar>(R-1);
      uchar* p = inputs[0].ptr<uchar>(R);
      uchar* pplus1 = inputs[0].ptr<uchar>(R+1);
      for (unsigned C = 1; C < inputs[0].cols - 1; ++C) {
        uchar Values[3][3] = {0};
        uchar Ic = p[C];
        Values[0][0] = int(pminus1[C-1]) - int(Ic) >= 0 ? 1 : 0;
        Values[0][1] = int(pminus1[C]) - int(Ic) >= 0 ? 1 : 0;
        Values[0][2] = int(pminus1[C+1]) - int(Ic) >= 0 ? 1 : 0;
        Values[1][0] = int(p[C-1]) - int(Ic) >= 0 ? 1 : 0;
        Values[1][2] = int(p[C+1]) - int(Ic) >= 0 ? 1 : 0;
        Values[2][0] = int(pplus1[C-1]) - int(Ic) >= 0 ? 1 : 0;
        Values[2][1] = int(pplus1[C]) - int(Ic) >= 0 ? 1 : 0;
        Values[2][2] = int(pplus1[C+1]) - int(Ic) >= 0 ? 1 : 0;

        int LBP = Values[0][0] * Weights[0][0] +
                  Values[0][1] * Weights[0][1] +
                  Values[0][2] * Weights[0][2] +
                  Values[1][0] * Weights[1][0] +
                  Values[1][2] * Weights[1][2] +
                  Values[2][0] * Weights[2][0] +
                  Values[2][1] * Weights[2][1] +
                  Values[2][2] * Weights[2][2];
        ReductionOutput[LBP]++;
      }
    }
  }

  virtual void Epilogue() override
  {
    std::vector<int64_t> new_histogram(59);

    for (uint16_t i = 0; i < 256; ++i) {
      std::set<uint8_t> bins = replaceY3AndY6(i);
      int Ti = bins.size();
      int Ti1 = 0; // Uniform patterns
      // Calculate the number of uniform patterns for the current bin.
      for (std::set<uint8_t>::iterator it = bins.begin();
           it != bins.end(); ++it) {
        if (isUniformPattern(*it))
          Ti1++;
      }
      int64_t bin_value = ReductionOutput[i];
      if (Ti == 0) {
        new_histogram[LUTUniformHist[i]] += bin_value;
        continue;
      }
      // TODO: Don't iterate two times over the set. replaceY3andY6 can return
      // the number of uniform and non-uniform patterns.
      for (std::set<uint8_t>::iterator it = bins.begin();
           it != bins.end(); ++it) {
        int64_t bin_contribution = 0;
        if (isUniformPattern(*it)) {
          bin_contribution = bin_value / Ti;
        } else {
          bin_contribution = bin_value * ((Ti - Ti1) / Ti);
        }
        new_histogram[LUTUniformHist[*it]] += bin_contribution;
      }
    }
    ReductionOutput = new_histogram;
  }

private:
  // LUT that maps all 8-bit patterns to non-uniform (bin 0) and 58 uniform
  // patterns [1-58].
  std::vector<uint8_t> LUTUniformHist;
};

class Application {
public:
  Application() = delete;
  Application(int argc, char** argv)
  {
    Descs.add_options()
      ("help,h", "Print this help")
      ("input,i", boost::program_options::value<std::string>(&InputPath),
       "Path to input file")
      ("output,o", boost::program_options::value<std::string>(&OutputPath),
       "Path to output file");
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(Descs).run(),
          ProgramOptions);
    boost::program_options::notify(ProgramOptions);
  }

  int Run()
  {
    if (ProgramOptions.count("help")) {
      std::cout << Descs << std::endl;
      return 0;
    }

    if (ProgramOptions.count("input") == 0) {
      std::cout << "Exiting: Input file not specified!\n";
      return 1;
    }

    if (ProgramOptions.count("output") == 0) {
      std::cout << "Exiting: Output file not specified!\n";
      return 1;
    }

    return 0;
  }

  Application(const Application& app) = delete;

  ~Application() { }

  Application operator=(Application a) = delete;
private:
  boost::program_options::variables_map ProgramOptions;
  boost::program_options::options_description Descs;
  std::string InputPath;
  std::string OutputPath;
};

int main(int argc, char** argv)
{
  Application app(argc, argv);
  return app.Run();
}
