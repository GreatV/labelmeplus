#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>

#include <QDir>

typedef struct configuration {
  QString here = QDir::current().absolutePath();

  void get_default_config() {
    QString config_file =
        QDir::cleanPath(here + QDir::separator() + "config" +
                        QDir::separator() + "default_config.yaml");

    YAML::Node config = YAML::Load(config_file.toStdString());
  };

} configuration;

#endif  // CONFIG_H
