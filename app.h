#ifndef APP_H
#define APP_H

#include <yaml-cpp/yaml.h>

#include <QDockWidget>
#include <QMainWindow>

#include "widgets/canvas.h"
#include "widgets/label_dialog.h"
#include "widgets/label_list_widget.h"
#include "widgets/unique_label_qlist_widget.h"
#include "widgets/zoom_widget.h"

class app : public QMainWindow {
  Q_OBJECT

 public:
  app(const std::string &config, const std::string &filename,
      const std::string &output, const std::string &output_file,
      const std::string &output_dir, QWidget *parent = nullptr);
  ~app();

 private:
  YAML::Node get_config();
  static QColor getYamlColor(const YAML::Node &node);

 private:
  std::string output_file_;
  YAML::Node config_;
  bool dirty_;
  bool _noSelectionSlot;
  std::string _copied_shapes;
  QString lastOpenDir_;

  LabelDialog *labelDialog_;
  LabelListWidget *labelList_;

  QDockWidget *flag_dock_;
  QListWidget *flag_widget_;
  QDockWidget *shape_dock_;

  UniqueLabelQListWidget *uniqLabelList_;
  QDockWidget *label_dock_;

  QLineEdit *fileSearch_;
  QListWidget *fileListWidget_;
  QVBoxLayout *fileListLayout_;

  QDockWidget *file_dock_;
  ZoomWidget *zoomWidget_;

  Canvas *canvas_;
};
#endif  // APP_H
