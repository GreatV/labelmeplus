#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>

#include <QString>

YAML::Node get_default_config();

YAML::Node get_config(const YAML::Node& config_file_or_yaml = {},
                      const QString& config_from_args = {});

#endif  // CONFIG_H
