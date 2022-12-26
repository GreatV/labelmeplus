#include "label_file.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include <opencv2/opencv.hpp>

#include "utils/utils.h"

LabelFile::LabelFile(const QString& filename) {
  if (!filename.isEmpty())
  {
    load(filename);
  }
  filename_ = filename;
}
cv::Mat LabelFile::load_image_file(const QString& filename) {
  cv::Mat raw_image;
  try{
    raw_image = cv::imread(filename.toStdString(), cv::IMREAD_UNCHANGED);
  }
  catch (...) {
    LOG(ERROR) << fmt::format("Failed opening image file: {1}",
                              filename.toStdString());
    return raw_image;
  }
  // apply orientation to image according to exif
  cv::Mat image = apply_exif_orientation(filename.toStdString(), raw_image);

  return image;
}
void LabelFile::load(const QString& filename) {
  std::vector<QString> keys {
      "version",
      "imageData",
      "imagePath",
      "shapes",
      "flags",
      "imageHeight",
      "imageWidth"
  };

  std::vector<QString> shape_keys {
      "label",
      "points",
      "group_id",
      "flags"
  };
}
