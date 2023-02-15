#include "config.h"

#include <glog/logging.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <fstream>

auto here = QFileInfo(__FILE__).dir().absolutePath();

static void copy_yaml(const YAML::Node& src, YAML::Node& dst) {
  // https://github.com/Humhu/argus_utils/blob/master/argus_utils/src/YamlUtils.cpp

  if (src.IsNull()) {
    return;
  }
  if (src.IsScalar()) {
    dst = src;
    return;
  };

  YAML::Node::const_iterator iter;
  for (iter = src.begin(); iter != src.end(); iter++) {
    dst[iter->first.as<std::string>()] = iter->second;
  }
}

static YAML::Node merge_yaml(const YAML::Node& a, const YAML::Node& b) {
  // https://github.com/Humhu/argus_utils/blob/master/argus_utils/src/YamlUtils.cpp

  // Short circuit cases
  if (a.IsNull()) {
    return b;
  } else if (b.IsNull()) {
    return a;
  }

  if (!a.IsMap() || !b.IsMap()) {
    throw std::runtime_error("Cannot merge non-map nodes.");
  }

  YAML::Node node;
  copy_yaml(a, node);
  YAML::Node::const_iterator iter;
  // Cycle through b and add all fields to node
  for (iter = b.begin(); iter != b.end(); iter++) {
    std::string key = iter->first.as<std::string>();

    // If both a and b have a key we have to merge them
    if (node[key]) {
      node[key] = merge_yaml(node[key], iter->second);
    }
    // Otherwise we just add it
    else {
      node[key] = iter->second;
    }
  }
  return node;
}

YAML::Node get_default_config() {
  auto config_file = QDir(here).absoluteFilePath("default_config.yaml");
  auto config = YAML::LoadFile(config_file.toStdString());

  // save default config to ~/.labelmerc
  auto home = QDir::homePath();
  auto user_config_file = QDir(home).absoluteFilePath(".labelmerc");
  if (!QFile::exists(user_config_file)) {
    auto flag = QFile::copy(config_file, user_config_file);
    if (flag == false) {
      LOG(WARNING) << "Failed to save config: "
                   << user_config_file.toStdString();
    }
  }

  return config;
}

static void validate_config_item(const YAML::Node& config) {
  if (auto validate_label = config["validate_label"]) {
    if (!(validate_label.IsNull() ||
          validate_label.as<std::string>() == "exact")) {
      LOG(ERROR) << "Unexpected value for config key `validate_label`: "
                 << validate_label.as<std::string>();
    }
  }

  if (auto shape_color = config["shape_color"]) {
    std::vector<std::string> color_type{"auto", "manual"};
    if (!(shape_color.IsNull() ||
          (std::find(color_type.begin(), color_type.end(),
                     shape_color.as<std::string>()) != color_type.end()))) {
      LOG(ERROR) << "Unexpected value for config key `shape_color`: "
                 << shape_color.as<std::string>();
    }
  }

  if (auto labels = config["labels"]) {
    if (!labels.IsNull()) {
      auto label_list = labels.as<std::vector<std::string>>();
      std::set<std::string> label_set(label_list.begin(), label_list.end());
      if (label_list.size() != label_set.size()) {
        LOG(ERROR) << "Unexpected value for config key `labels`";
      }
    }
  }
}

YAML::Node get_config(const YAML::Node& config_file_or_yaml,
                      const QString& config_from_args) {
  // 1. default config
  auto config = get_default_config();

  // 2. specified as file or yaml
  if (config_file_or_yaml) {
    merge_yaml(config, config_file_or_yaml);
  }

  // 3. command line argument or specified config file
  if (!config_from_args.isEmpty()) {
    auto args_config = YAML::Load(config_from_args.toStdString());
    merge_yaml(config, args_config);
  }

  validate_config_item(config);

  return config;
}
