// #ifdef _WIN32
// #define GLOG_NO_ABBREVIATED_SEVERITIES
// #endif
#include <fmt/core.h>
#include <glog/logging.h>

#include <QApplication>
#include <QTranslator>
#include <algorithm>
#include <argparse/argparse.hpp>
#include <filesystem>

#include "app.h"
#include "init.h"

static std::map<std::string, int> logger_level_map{{"INFO", google::INFO},
                                                   {"WARNING", google::WARNING},
                                                   {"ERROR", google::ERROR},
                                                   {"FATAL", google::FATAL}};

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = true;
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  auto parser = argparse::ArgumentParser();
  parser.add_argument("--version", "-V")
      .default_value(false)
      .implicit_value(true)
      .help("show version");

  parser.add_argument("--reset-config")
      .default_value(false)
      .implicit_value(true)
      .help("reset qt config");

  parser.add_argument("--logger-level")
      .default_value(std::string{"info"})
      .action([=](const std::string &value) {
        static const std::vector<std::string> choices = {"info", "warning",
                                                         "fatal", "error"};
        if (std::find(choices.begin(), choices.end(), value) != choices.end()) {
          return value;
        }
        return std::string{"info"};
      })
      .help("logger level");

  parser.add_argument("filename")
      .nargs(argparse::nargs_pattern::optional)
      .help("image or label filename");

  parser.add_argument("--output", "-O", "-o")
      .help(
          "output file or directory (if it ends with .json it is recognized as "
          "file, else as directory)");

  auto default_config_file_path =
      std::filesystem::path("~") / std::filesystem::path(".labelmerc");
  std::string default_config_file = default_config_file_path.generic_string();
  parser.add_argument("--config")
      .default_value(default_config_file)
      .help(fmt::format("config file or yaml-format string (default: {})",
                        default_config_file));

  // config for the gui
  parser.add_argument("--nodata");
  parser.add_argument("--autosave");
  parser.add_argument("--nosortlabels");
  parser.add_argument("--flags");
  parser.add_argument("--labelflags");
  parser.add_argument("--labels");
  parser.add_argument("--validatelabels");
  parser.add_argument("--keep-prev");
  parser.add_argument("--epsilon");

  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << parser;
    std::exit(1);
  }

  if (parser["--version"] == true) {
    fmt::print("{0} {1}\n", __appname__, __version__);
    return 0;
  }
  auto logger_level = parser.get<std::string>("--logger-level");
  LOG(INFO) << "current logger level: " << logger_level;

  std::transform(logger_level.begin(), logger_level.end(), logger_level.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  FLAGS_minloglevel = logger_level_map[logger_level];

  QApplication a(argc, argv);
  a.setApplicationName(QString::fromStdString(__appname__));

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages) {
    const QString baseName =
        QString::fromStdString(fmt::format("{}_", __appname__)) +
        QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      a.installTranslator(&translator);
      break;
    }
  }

  std::string config = "";
  std::string filename = "";
  std::string output = "";
  std::string output_file = "";
  std::string output_dir = "";
  app w = app(config, filename, output, output_file, output_dir);
  w.show();
  return a.exec();
}
