#include <boost/program_options.hpp>
#include <string>

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
