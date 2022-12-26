#ifndef LABEL_FILE_H
#define LABEL_FILE_H

#include <QString>
#include <vector>
#include <opencv2/opencv.hpp>

class LabelFile {
  QString suffix = ".json";
 public:
  LabelFile(const QString& filename);
 private:
  std::vector<QString> shapes;
  QString imagePath;
  QString imageData;
  QString filename_;
  cv::Mat load_image_file(const QString& filename);
  void load(const QString& filename);
};

#endif  // LABEL_FILE_H
