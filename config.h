#ifndef CONFIG_H
#define CONFIG_H

#include <glog/logging.h>
#include <yaml-cpp/yaml.h>

#include <QDir>
#include <set>
#include <stdexcept>

static const QString here = QDir::current().absolutePath();

static void update_dict(
    YAML::Node& target_dict, const YAML::Node& new_dict,
    std::function<void(std::string, YAML::Node)> validate_item = nullptr) {
  for (YAML::const_iterator it = new_dict.begin(); it != new_dict.end(); ++it) {
    if (validate_item) {
      validate_item(it->first.as<std::string>(), it->second);
    }
    if (!target_dict[it->first]) {
      LOG(WARNING) << "Skipping unexpected key in config: "
                   << it->first.as<std::string>();
      continue;
    }
    if (target_dict[it->first].IsMap() && it->second.IsMap()) {
      YAML::Node item{};
      update_dict(item, it->second, validate_item);
      target_dict[it->first.as<std::string>()] = item;
    } else {
      target_dict[it->first] = it->second;
    }
  }
}

static YAML::Node get_default_config() {
  QString config_file =
      QDir::cleanPath(here + QDir::separator() + "config" + QDir::separator() +
                      "default_config.yaml");

  YAML::Node config = YAML::LoadFile(config_file.toStdString());

  // save default config to ~/.labelmerc
  QString user_config_file = QDir::homePath() + "/.labelmerc";
  if (!QFile::exists(user_config_file)) {
    QFile::copy(config_file, user_config_file);
    if (!QFile::exists(user_config_file)) {
      LOG(WARNING) << "Failed to save config: "
                   << user_config_file.toStdString();
    }
  }

  return config;
};

static void validateConfigItem(const std::string& key,
                               const YAML::Node& value) {
  if (key == "validate_label" && value && value.as<std::string>() != "exact") {
    throw std::runtime_error(
        "Unexpected value for config key 'validate_label': " +
        value.as<std::string>());
  }
  if (key == "shape_color" && value && value.as<std::string>() != "auto" &&
      value.as<std::string>() != "manual") {
    throw std::runtime_error("Unexpected value for config key 'shape_color': " +
                             value.as<std::string>());
  }
  if (key == "labels" && value) {
    std::vector<std::string> labels = value.as<std::vector<std::string>>();
    std::set<std::string> unique_labels(labels.begin(), labels.end());
    if (labels.size() != unique_labels.size()) {
      throw std::runtime_error(
          "Duplicates are detected for config key 'labels'");
    }
  }
}

static YAML::Node get_config(const YAML::Node& config_file_or_yaml,
                             const YAML::Node& config_from_args) {
  // default config
  auto config = get_default_config();
  update_dict(config, config_file_or_yaml, validateConfigItem);
  update_dict(config, config_from_args, validateConfigItem);
  return config;
};

#endif  // CONFIG_H
