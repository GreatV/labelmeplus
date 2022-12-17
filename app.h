#ifndef APP_H
#define APP_H

#include <QMainWindow>

class app : public QMainWindow {
  Q_OBJECT

 public:
  app(const std::string &config, const std::string &filename,
      const std::string &output, const std::string &output_file,
      const std::string &output_dir, QWidget *parent = nullptr);
  ~app();

 private:
  std::string output_file_;
  std::string config_;

  std::string get_config();
};
#endif  // APP_H
