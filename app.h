#ifndef APP_H
#define APP_H

#include <yaml-cpp/yaml.h>

#include <QAction>
#include <QDockWidget>
#include <QImage>
#include <QMainWindow>
#include <QMenuBar>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>

#include "label_file.h"
#include "widgets/canvas.h"
#include "widgets/label_dialog.h"
#include "widgets/label_list_widget.h"
#include "widgets/tool_bar.h"
#include "widgets/unique_label_qlist_widget.h"
#include "widgets/zoom_widget.h"

typedef enum zoom_method {
  FIT_WINDOW,
  FIT_WIDTH,
  MANUAL_ZOOM,
} zoom_method;

class app : public QMainWindow {
  Q_OBJECT

 public:
  app(const YAML::Node& config = {}, const QString& filename = {},
      const QString& output = {}, const QString& output_file = {},
      const QString& output_dir = {}, QWidget* parent = nullptr);
  ~app();

 private:
  QString output_file_;
  QString output_dir_;
  YAML::Node config_;
  bool dirty_;
  bool _noSelectionSlot;
  QList<Shape*> _copied_shapes;
  QString lastOpenDir_;

  LabelDialog* labelDialog_;
  LabelListWidget* labelList_;

  QDockWidget* flag_dock_;
  QListWidget* flag_widget_;
  QDockWidget* shape_dock_;

  UniqueLabelQListWidget* uniqLabelList_;
  QDockWidget* label_dock_;

  QLineEdit* fileSearch_;
  QListWidget* fileListWidget_;
  QVBoxLayout* fileListLayout_;

  QDockWidget* file_dock_;
  ZoomWidget* zoomWidget_;

  Canvas* canvas_;
  QScrollArea* scrollArea_;
  ToolBar* tools_;
  std::map<int, QScrollBar*> scrollBars_;

  // Actions
  QAction *quit_action_, *open_action_, *opendir_action_,
      *open_next_img_action_;
  QAction *open_prev_img_action_, *save_action_, *save_as_action_,
      *delete_file_action_;
  QAction *change_output_dir_action_, *save_auto_action_,
      *save_with_image_data_action_;
  QAction *close_action_, *toggle_keep_prev_mode_action_, *create_mode_action_;
  QAction *create_rectangle_mode_action_, *create_circle_mode_action_,
      *create_line_mode_action_;
  QAction *create_point_mode_action_, *create_linestrip_mode_action_,
      *edit_mode_action_;
  QAction *delete_mode_action_, *duplicate_action_, *copy_action_,
      *paste_action_;
  QAction *undo_last_point_action_, *remove_point_action_, *undo_action_,
      *hide_all_action_;
  QAction *show_all_action_, *help_action_;
  QAction *zoomIn_action_, *zoomOut_action_, *zoomOrg_action_,
      *keepPrevScale_action_;
  QAction *fitWindow_action_, *fitWidth_action_, *brightnessContrast_action_;
  QAction *edit_action_, *fill_drawing_action_;
  QAction *copy_here_action_, *move_here_action_;

  QList<QAction*> zoomActions_{};

  int zoomMode_ = FIT_WINDOW;

  QList<QAction*> tool_;
  QList<QAction*> menu_;
  QList<QAction*> edit_menu_actions_{};
  QList<QAction*> onLoadActive_{};
  QList<QAction*> onShapesPresent_actions_{};

  QMenu* labelMenu_;

  // Menus
  QMenu* file_menu_;
  QMenu* edit_menu_;
  QMenu* view_menu_;
  QMenu* help_menu_;
  QMenu* recentFiles_menu_;
  QMenu* labelList_menu_;

  QImage* image_;
  QString imagePath_{};
  QImage imageData_{};
  LabelFile* labelFile_{};
  QList<QString> recentFiles_{};
  int maxRecent_ = 7;
  void* otherData_;
  int zoom_level_ = 100;
  bool fit_window_ = false;
  std::map<QString, std::tuple<int, int>> zoom_value_{};
  std::map<QString, std::tuple<int, int>> brightnessContrast_values_;
  std::map<int, std::map<QString, int>> scroll_values_{};
  QString filename_;

  QSettings* settings_;

 private:
  QMenu* menu(const QString& title, const QList<QAction*>& actions = {});
  void toolbar(ToolBar* toolbar, const QString& title,
               const QList<QAction*>& actions = {});
  bool noShapes();
  void populateModeActions();
  void setClean();
  void toggleActions(bool value = true);
  template <typename Functor>
  void queueEvent(Functor function);
  void status(const QString& message, int delay = 5000);
  void resetState();
  LabelListWidgetItem* currentItem();
  void addRecentFile(const QString& filename);
  void toggleDrawMode(bool edit = true, const QString& createMode = "polygon");

  bool validateLabel(const QString& label);
  void addLabel(Shape* shape);
  void _update_shape_color(Shape* shape);
  std::vector<int> _get_rgb_by_label(const QString& label);
  void remLabels(QList<Shape*> shapes);
  void loadShapes(QList<Shape*> shapes, bool replace = true);
  void loadLabels(const std::vector<std::map<std::string, std::any>>& shapes);
  void loadFlags(const std::map<std::string, bool>& flags);
  bool saveLabels(const QString& filename = {});

  void labelItemChanged(LabelListWidgetItem* item);
  void labelOrderChanged();

  // callback functions
  void setScroll(int orientation, double value);
  void setZoom(int value);
  void addZoom(double increment = 1.1);
  void onNewBrightnessContrast(const QImage& image);
  void togglePolygons(bool value);
  bool loadFile(QString filename);
  void resizeEvent(QResizeEvent* event) override;
  void adjustScale(bool initial = false);
  double scaleFitWindow();
  double scaleFitWidth();
  void closeEvent(QCloseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  // User Dialogs
  void loadRecent(const QString& filename);

  QString saveFileDialog();
  void _saveFile(const QString& filename);
  QString getLabelFile();

  // Message Dialogs.
  bool hasLabels();
  bool hasLabelFile();
  bool mayContinue();
  void errorMessage(const QString& title, const QString& message);
  QString currentPath();

  void copyShape();
  void moveShape();
  QList<QString> imageList();
  void importDroppedImageFiles(const QList<QString>& imageFiles);
  void importDirImages(const QString& dirpath, const QString& pattern = {},
                       bool load = true);
  QList<QString> scanAllImages(const QString& folderPath);

 private slots:
  void setDirty();
  void labelSelectionChanged();
  void fileSearchChanged();
  void fileSelectionChanged();
  void zoomRequest(int delta, const QPointF& pos);
  void scrollRequest(int delta, int orientation);
  void newShape();
  void shapeSelectionChanged(const QList<lmp::Shape*>& selected_shapes);
  void toggleDrawingSensitive(bool drawing = true);
  void openFile(bool _value = false);
  void openDirDialog(bool _value = false, const QString& dirpath = {});
  void openPrevImg(bool _value = false);
  void openNextImg(bool _value = false, bool load = true);
  void saveFile(bool _value = false);
  void saveFileAs(bool _value = false);
  void deleteFile();
  void changeOutputDirDialog(bool _value = false);
  void onSaveAutoTriggered(bool value = false);
  void enableSaveImageWithData(bool enabled);
  void closeFile(bool _value = false);
  void toggleKeepPrevMode();
  void onCreateModeTriggered();
  void onCreateRectangleModeTriggered();
  void onCreateCircleModeTriggered();
  void onCreateLineModeTriggered();
  void onCreatePointModeTriggered();
  void onCreateLinestripModeTriggered();
  void setEditMode();
  void deleteSelectedShape();
  void duplicateSelectedShape();
  void copySelectedShape();
  void pasteSelectedShape();
  void removeSelectedPoint();
  void undoShapeEdit();
  void onHideAllTriggered();
  void onShowAllTriggered();
  void tutorial();
  void onZoomInTriggered();
  void onZoomOutTriggered();
  void onZoomOrgTriggered();
  void enableKeepPrevScale(bool enabled = true);
  void setFitWindow(bool value = true);
  void setFitWidth(bool value = true);
  void brightnessContrast(bool value);
  void editLabel(LabelListWidgetItem* item = nullptr);
  void popLabelListMenu(const QPoint& point);
  void paintCanvas();
  void updateFileMenu();
};
#endif  // APP_H
