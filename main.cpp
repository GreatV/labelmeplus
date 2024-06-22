#include <glog/logging.h>
#include <yaml-cpp/yaml.h>

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QStandardPaths>
#include <QString>
#include <QTranslator>
#include <argparse/argparse.hpp>
#include <fstream>

#include "app.h"
#include "config.h"
#include "version.h"

static std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::stringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

int main(int argc, char *argv[]) {
  argparse::ArgumentParser parser("labelme++", "0.0",
                                  argparse::default_arguments::help);
  parser.add_argument("--version", "-v")
      .default_value(false)
      .implicit_value(true)
      .help("show version");
  parser.add_argument("--reset-config")
      .default_value(false)
      .implicit_value(true)
      .help("reset qt config");
  parser.add_argument("--logger-level")
      .default_value(std::string{"info"})
      // .choices("info", "warning", "fatal", "error")
      // .nargs(1)
      .help("logger level");
  parser.add_argument("filename")
      .help("image or label filename")
      .nargs(argparse::nargs_pattern::optional);
  parser.add_argument("--output", "-O", "-o")
      .help(
          "output file or directory (if it ends with .json it is recognized as "
          "file, else as directory)");

  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QDir dir(home);
  QString default_config_file = dir.filePath(".labelmerc");
  QString config_help_string =
      QString("config file or yaml-format string (default: %1)")
          .arg(default_config_file);
  parser.add_argument("--config")
      .default_value(default_config_file.toStdString())
      .help(config_help_string.toStdString());

  // config for the gui
  parser.add_argument("--nodata")
      .default_value(true)
      .implicit_value(false)
      .help("stop storing image data to JSON file");

  parser.add_argument("--autosave")
      .default_value(false)
      .implicit_value(true)
      .help("auto save");

  parser.add_argument("--nosortlabels")
      .default_value(false)
      .implicit_value(true)
      .help("stop sorting labels");

  parser.add_argument("--flags").help(
      "comma separated list of flags OR file containing flags");

  parser.add_argument("--labelflags")
      .help(
          R"(yaml string of label specific flags OR file containing json string of label specific flags (ex. {person-\d+: [male, tall], dog-\d+: [black, brown, white], .*: [occluded]}))");

  parser.add_argument("--labels")
      .help("comma separated list of labels OR file containing labels");

  parser.add_argument("--validatelabel")
      .default_value(std::string("exact"))
      .choices("exact")
      .help("label validation types");

  parser.add_argument("--keep-prev")
      .default_value(true)
      .implicit_value(false)
      .help("keep annotation of previous frame");

  parser.add_argument("--epsilon")
      .scan<'g', float>()
      .help("epsilon to find nearest vertex on canvas");

  try {
    parser.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << parser;
    std::exit(1);
  }

  if (parser["--version"] == true) {
    qDebug() << QString("%1 %2.%3.%4")
                    .arg(PROGRAM_NAME)
                    .arg(MAJOR_VERSION)
                    .arg(MINOR_VERSION)
                    .arg(PATCH_VERSION);
    return 0;
  }

  std::map<std::string, int> log_level_map{{"info", google::GLOG_INFO},
                                           {"warning", google::WARNING},
                                           {"fatal", google::FATAL},
                                           {"error", google::ERROR}};
  std::string logger_level = parser.get<std::string>("--logger-level");
  FLAGS_minloglevel = log_level_map[logger_level];

  std::vector<std::string> flags{};
  if (parser.is_used("--flags")) {
    std::string flags_string = parser.get<std::string>("--flags");
    QFileInfo file_info(QString::fromStdString(flags_string));
    if (file_info.exists() && file_info.isFile()) {
      std::ifstream in(flags_string);
      std::string line{};
      while (in >> line) {
        flags.emplace_back(line);
      }
    } else {
      flags = split(flags_string, ',');
    }
  }

  std::vector<std::string> labels{};
  if (parser.is_used("--labels")) {
    std::string labels_string = parser.get<std::string>("--labels");
    QFileInfo file_info(QString::fromStdString(labels_string));
    if (file_info.exists() && file_info.isFile()) {
      std::ifstream in(labels_string);
      std::string line{};
      while (in >> line) {
        labels.emplace_back(line);
      }
    } else {
      labels = split(labels_string, ',');
    }
  }

  YAML::Node label_flags{};
  if (parser.is_used("--labelflags")) {
    std::string label_flags_string = parser.get<std::string>("--labelflags");
    QFileInfo file_info(QString::fromStdString(label_flags_string));
    if (file_info.exists() && file_info.isFile()) {
      label_flags = YAML::LoadFile(label_flags_string);
    } else {
      label_flags = YAML::Load(label_flags_string);
    }
  }

  YAML::Node config_yaml{};
  if (parser.is_used("--config")) {
    std::string config_yaml_string = parser.get<std::string>("--config");
    QFileInfo file_info(QString::fromStdString(config_yaml_string));
    if (file_info.exists() && file_info.isFile()) {
      config_yaml = YAML::LoadFile(config_yaml_string);
    } else {
      config_yaml = YAML::Load(config_yaml_string);
    }
  }

  YAML::Node config = get_config(config_yaml, config_yaml);

  QApplication a(argc, argv);

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages) {
    const QString baseName = "labelmeplus_" + QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      a.installTranslator(&translator);
      break;
    }
  }
  app w;
  w.show();
  return a.exec();
}
