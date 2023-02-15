#include "label_file.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include <QDir>
#include <QFileInfo>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

#include "base64.h"
#include "init.h"
#include "utils/utils.h"

static std::vector<std::string_view> split(std::string_view strv,
                                           std::string_view delims = " ") {
  std::vector<std::string_view> output;
  size_t first = 0;

  while (first < strv.size()) {
    const auto second = strv.find_first_of(delims, first);

    if (first != second)
      output.emplace_back(strv.substr(first, second - first));

    if (second == std::string_view::npos) break;

    first = second + 1;
  }

  return output;
}

LabelFile::LabelFile(const QString& filepath) : filename(filepath) {
  if (!filepath.isEmpty()) {
    load(filepath);
  }
}
cv::Mat LabelFile::load_image_file(const QString& filepath) {
  cv::Mat raw_image;
  try {
    raw_image = cv::imread(filepath.toStdString(), cv::IMREAD_UNCHANGED);
  } catch (...) {
    LOG(ERROR) << fmt::format("Failed opening image file: {1}",
                              filepath.toStdString());
    return raw_image;
  }
  // apply orientation to image according to exif
  cv::Mat image = apply_exif_orientation(filepath.toStdString(), raw_image);

  return image;
}
void LabelFile::load(const QString& filepath) {
  std::vector<std::string> keys{
      "version",     "imageData",  "imagePath",
      "shapes",  // polygonal annotations
      "flags",   // image level flags
      "imageHeight", "imageWidth",
  };

  std::vector<std::string> shape_keys{
      "label", "points", "group_id", "shape_type", "flags",
  };

  nlohmann::json data{};
  try {
    std::fstream f(filepath.toStdString());
    data = nlohmann::json::parse(f);
    f.close();
    std::string version{};
    if (data["version"].is_null()) {
      LOG(WARNING) << fmt::format("Loading JSON file {} of unknown version.",
                                  filepath.toStdString());
    } else {
      version = data["version"].get<std::string>();
      auto version_number = split(version, ".");
      auto current_version_number = split(__version__, ".");
      if (version_number[0] != current_version_number[0]) {
        LOG(WARNING) << fmt::format(
            "This JSON file {0} may be incompatible with "
            "current labelmeplus. version in file: {1}, "
            "current version: {2}",
            filepath.toStdString(), version, __version__);
      }
    }

    if (!data["imageData"].is_null()) {
      auto encode = data["imageData"].get<std::string>();
      auto decode = base64_decode(encode);
      std::vector<uchar> raw_data(decode.begin(), decode.end());
      imageData = cv::imdecode(raw_data, cv::IMREAD_UNCHANGED);
    } else {
      // relative path from label file to relative path from cwd.
      auto dir = QFileInfo(filepath).dir();
      auto image_path = data["imagePath"].get<std::string>();
      imagePath = dir.relativeFilePath(QString::fromStdString(image_path));
      imageData = load_image_file(imagePath);
    }

    flags = {};
    if (!data["flags"].is_null()) {
      for (auto& [key, value] : data["flags"].items()) {
        flags[key] = value.get<bool>();
      }
    }
    imagePath = QString::fromStdString(data["imagePath"].get<std::string>());
    auto image_height = data["imageHeight"].get<int>();
    auto image_width = data["imageWidth"].get<int>();
    check_image_height_and_width(imageData, image_height, image_width);
    //    std::vector<std::map<std::string, std::any>> shapes {};
    for (auto& s : data["shapes"]) {
      std::map<std::string, std::any> shape{};
      shape["label"] = s["label"].get<std::string>();
      std::vector<std::vector<double>> points{};
      for (auto& point : s["points"]) {
        std::vector<double> p{};
        for (auto& item : point) {
          p.emplace_back(item.get<double>());
        }
        points.emplace_back(p);
      }
      shape["points"] = points;
      shape["shape_type"] = std::string("polygon");
      if (!s["shape_type"].is_null()) {
        shape["shape_type"] = s["shape_type"].get<std::string>();
      }

      shape["flags"] = {};

      if (!s["flags"].is_null()) {
        shape["flags"] = s["flags"].get<std::map<std::string, bool>>();
      }
      shape["group_id"] = -1;
      if (!s["group_id"].is_null()) {
        shape["group_id"] = s["group_id"].get<int>();
      }
      std::map<std::string, std::any> other_data = {};
      for (auto& [k, v] : s.items()) {
        if (std::find(shape_keys.begin(), shape_keys.end(), k) ==
            shape_keys.end()) {
          other_data[k] = v;
        }
      }
      shape["other_data"] = other_data;

      shapes.emplace_back(shape);
    }
  } catch (std::exception& e) {
    throw e;
  }

  otherData = {};

  filename = filepath;

  otherData = {};
  for (auto& [key, value] : data.items()) {
    if (std::find(keys.begin(), keys.end(), key) != keys.end()) {
      otherData[key] = value;
    }
  }
}

std::tuple<int, int> LabelFile::check_image_height_and_width(
    const cv::Mat& image, const int image_height, const int image_width) {
  int height = image_height, width = image_width;
  if (image_height <= 0 && image.rows != image_height) {
    LOG(ERROR) << "imageHeight does not match with imageData or imagePath, "
                  "so getting imageHeight from actual image.";
    height = image.rows;
  }

  if (image_width <= 0 && image.cols != image_width) {
    LOG(ERROR) << "imageWidth does not match with imageData or imagePath, "
                  "so getting imageWidth from actual image.";
    width = image.cols;
  }
  return {height, width};
}
void LabelFile::save(
    const QString& filepath,
    const std::vector<std::map<std::string, std::any>>& shape_info_list,
    const QString& image_path, const int image_height, const int image_width,
    const cv::Mat& image_data,
    const std::map<std::string, std::any>& other_data,
    const std::map<std::string, bool>& flag_list) {
  std::map<std::string, std::any> data{};
  data["version"] = __version__;
  data["flags"] = flag_list;
  data["shapes"] = shape_info_list;
  data["imagePath"] = image_path.toStdString();

  if (!image_data.empty()) {
    std::string image_format = ".png";
    std::vector<uchar> buf;
    cv::imencode(image_format, image_data, buf);
    std::string raw_data = std::string((char*)buf.data());
    data["imageData"] = raw_data;
    int height, width;
    std::tie(height, width) =
        check_image_height_and_width(image_data, image_height, image_width);
    data["imageHeight"] = height;
    data["imageWidth"] = width;
  }

  for (auto& [key, value] : other_data) {
    data[key] = value;
  }

  try {
    std::fstream f(filepath.toStdString(), std::ios::out);
    f.close();

  } catch (...) {
  }
}
bool LabelFile::is_label_file(const QString& filepath) {
  auto file_suffix = QFileInfo(filepath).suffix();
  return file_suffix.toLower() == suffix;
}
