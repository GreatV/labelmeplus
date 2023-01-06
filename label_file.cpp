#include "label_file.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include <fstream>
#include <filesystem>
#include <algorithm>

#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <string_view>

#include "init.h"
#include "utils/utils.h"
#include "base64.h"


std::vector<std::string_view> split(std::string_view strv,
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

LabelFile::LabelFile(const QString& filename) {
  if (!filename.isEmpty()) {
    load(filename);
  }
  filename_ = filename;
}
cv::Mat LabelFile::load_image_file(const std::string& filename) {
  cv::Mat raw_image;
  try {
    raw_image = cv::imread(filename, cv::IMREAD_UNCHANGED);
  } catch (...) {
    LOG(ERROR) << fmt::format("Failed opening image file: {1}",
                              filename);
    return raw_image;
  }
  // apply orientation to image according to exif
  cv::Mat image = apply_exif_orientation(filename, raw_image);

  return image;
}
void LabelFile::load(const QString& filename) {
  std::vector<QString> keys{"version", "imageData",   "imagePath", "shapes",
                            "flags",   "imageHeight", "imageWidth"};

  std::vector<QString> shape_keys{"label", "points", "group_id", "flags"};

  nlohmann::json data;
  std::fstream label_json(filename.toStdString(), std::ios::in);
  if (label_json.is_open()) {
    try {
      data = nlohmann::json::parse(label_json);
    } catch (...) {
      label_json.close();
      return;
    }
  }
  label_json.close();

  if (data["version"].is_null()) {
    LOG(WARNING) << fmt::format("Loading JSON file ({1}) of unknown version",
                                filename.toStdString());
  }
  auto version = data["version"].get<std::string>();

  auto version_number = split(version, ".");
  auto current_version_number = split(__version__, ".");
  if (version_number[0] != current_version_number[0]) {
    LOG(WARNING) << fmt::format(
        "This json file ({1}) may be incompatible with current labelme. "
        "version in file: {2}, current version: {3}",
        filename.toStdString(), version, __version__);
  }

  cv::Mat image_data;
  if (!data["imageData"].is_null())
  {
    auto data_encoded = data["imageData"].get<std::string>();
    auto data_decoded = base64_decode(data_encoded);
    std::vector<uchar> raw_data(data_decoded.begin(), data_decoded.end());
    image_data = cv::imdecode(raw_data, cv::IMREAD_UNCHANGED);
  }
  else {
    // relative path from label file to relative path from cwd
    auto image_path = data["imagePath"].get<std::string>();
    auto dirname = std::filesystem::path(filename.toStdString()).parent_path();
    image_path = (dirname / std::filesystem::path(image_path)).generic_string();
    image_data = load_image_file(image_path);
  }

  auto flags = data["flags"].get<std::string>();
  auto image_path = data["imagePath"].get<std::string>();

  // check image height and width
  int image_height = -1;
  int image_width = -1;

  if (!data["imageHeight"].is_null()) {
    image_height = data["imageHeight"].get<int>();
  }

  if (!data["imageWidth"].is_null()) {
    image_width = data["imageWidth"].get<int>();
  }

  std::tie(image_height, image_width) = check_image_height_and_width(image_data, image_height, image_width);

  std::vector<std::map<std::string, std::string>> shapes;
  for (auto &s : data["shape"])
  {
    std::map<std::string, std::string> shape{
        {"label", s["label"].get<std::string>()},
        {"points", s["points"].get<std::string>()},
        {"shape_type", s["shape_type"].get<std::string>()},
        {"flags", s["flags"].get<std::string>()},
        {"group_id", s["group_id"].get<std::string>()},
        {"other_data", ""}
    };
    shapes.emplace_back(shape);
  }

//  other_data;
  flags_ = flags;
}
std::tuple<int, int> LabelFile::check_image_height_and_width(const cv::Mat& image_data,
                                             const int image_height, const int image_width) {
  bool check_flag = false;
  int h = image_height, w = image_width;
    if (image_height != image_data.rows)
    {
      check_flag |= true;
    }

  if (check_flag)
  {
    LOG(ERROR) << "imageHeight does not match with imageData or imagePath, "
                  "so getting imageHeight from actual image.";
    h = image_data.rows;
  }

  check_flag = false;
    if (image_width != image_data.cols)
    {
      check_flag |= true;
    }

  if (check_flag)
  {
    LOG(ERROR) << "imageWidth does not match with imageData or imagePath, "
                  "so getting imageWidth from actual image.";
    w = image_data.cols;
  }

  return std::tuple<int, int>{h, w};
}

void LabelFile::save(
    const std::string& filename,
    const std::vector<std::map<std::string, std::any>>& shapes,
    const std::string& imagePath, const int imageHeight, const int imageWidth,
    const std::string& imageData,
    const std::vector<std::map<std::string, std::any>>& otherData,
    const std::vector<std::map<std::string, bool>>& flags) {

  cv::Mat image_data;
  if (!imageData.empty()) {
    auto data_encoded = imageData;
    auto data_decoded = base64_decode(data_encoded);
    std::vector<uchar> raw_data(data_decoded.begin(), data_decoded.end());
    image_data = cv::imdecode(raw_data, cv::IMREAD_UNCHANGED);
  }
    check_image_height_and_width(image_data, imageHeight, imageWidth);

    nlohmann::json data;
    auto content = data.dump(2, ' ', false);

    std::fstream output_json(filename);
    output_json << content;
}

bool LabelFile::is_label_file(const std::string& filename) {
    const std::string suffix = ".json";

    std::filesystem::path path(filename);
    std::string ext = path.extension();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return ext == suffix;
}
