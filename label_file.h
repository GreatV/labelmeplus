#ifndef LABEL_FILE_H
#define LABEL_FILE_H

#include <QString>
#include <any>
#include <exception>
#include <opencv2/opencv.hpp>

class LabelFileError : public std::exception {};

class LabelFile {
 public:
  explicit LabelFile(const QString& filepath = {});
  static cv::Mat load_image_file(const QString& filepath);
  void save(const QString& filepath,
            const std::vector<std::map<std::string, std::any>>& shape_info_list,
            const QString& image_path, int image_height, int image_width,
            const cv::Mat& image_data = {},
            const std::map<std::string, std::any>& other_data = {},
            const std::map<std::string, bool>& flag_list = {});

  static bool is_label_file(const QString& filepath);

 public:
  inline static const char* suffix = ".json";
  QString imagePath{};
  cv::Mat imageData{};
  QString filename{};
  std::map<std::string, bool> flags{};
  std::map<std::string, std::any> otherData{};
  std::vector<std::map<std::string, std::any>> shapes{};

 private:
  void load(const QString& filepath);
  static std::tuple<int, int> check_image_height_and_width(const cv::Mat& image,
                                                           int image_height,
                                                           int image_width);
};
#endif
