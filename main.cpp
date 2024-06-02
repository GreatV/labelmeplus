#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <argparse/argparse.hpp>

#include "app.h"
#include "version.h"

int main(int argc, char *argv[]) {
  argparse::ArgumentParser parser("labelme++", "0.0",
                                  argparse::default_arguments::none);
  parser.add_argument("--version", "-v")
      .default_value(false)
      .implicit_value(true)
      .help("show version");
  parser.add_argument("--reset-config")
      .default_value(false)
      .implicit_value(true)
      .help("reset qt config");
  parser.add_argument("--logger-level")
      .default_value(std::string{"debug"})
      .choices("debug", "info", "warning", "fatal", "error")
      .help("logger level");

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
