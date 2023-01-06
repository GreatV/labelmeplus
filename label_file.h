#ifndef LABEL_FILE_H
#define LABEL_FILE_H

#include <QString>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

class LabelFile {
 public:
  explicit LabelFile(const QString& filename);
  void save(const std::string& filename,
            const std::vector<std::map<std::string, std::any>>& shapes,
            const std::string& imagePath,
            int imageHeight,
            int imageWidth,
            const std::string& imageData,
            const std::vector<std::map<std::string, std::any>>& otherData,
            const std::vector<std::map<std::string, bool>>& flags
            );
  static bool is_label_file(const std::string& filename);
 private:
  std::string flags_;
  std::vector<QString> shapes;
  QString imagePath;
  QString imageData;
  QString filename_;
  cv::Mat load_image_file(const std::string& filename);
  void load(const QString& filename);
  std::tuple<int, int> check_image_height_and_width(
      const cv::Mat& image_data,
      int image_height,
      int image_width);
};

#endif  // LABEL_FILE_H
