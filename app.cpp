#include "app.h"

#include <fmt/format.h>
#include <glog/logging.h>
#include <widgets/file_dialog_preview.h>

#include <QApplication>
#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QMimeData>
#include <QTimer>
#include <QWidgetAction>
#include <filesystem>
#include <functional>
#include <opencv2/opencv.hpp>

#include "config/config.h"
#include "imgviz.hpp"
#include "init.h"
#include "label_file.h"
#include "opener.h"
#include "shape.h"
#include "utils/utils.h"
#include "widgets/brightness_contrast_dialog.h"
#include "widgets/tool_bar.h"

// const auto& LABEL_COLORMAP = imgviz::getLabelColor(255);

static void qimage_to_mat(const QImage& image, cv::OutputArray out) {
  switch (image.format()) {
    case QImage::Format_Invalid: {
      cv::Mat empty;
      empty.copyTo(out);
      break;
    }
    case QImage::Format_RGB32: {
      cv::Mat view(image.height(), image.width(), CV_8UC4,
                   (void*)image.constBits(), image.bytesPerLine());
      view.copyTo(out);
      break;
    }
    case QImage::Format_RGB888: {
      cv::Mat view(image.height(), image.width(), CV_8UC3,
                   (void*)image.constBits(), image.bytesPerLine());
      cvtColor(view, out, cv::COLOR_RGB2BGR);
      break;
    }
    default: {
      QImage conv = image.convertToFormat(QImage::Format_ARGB32);
      cv::Mat view(conv.height(), conv.width(), CV_8UC4,
                   (void*)conv.constBits(), conv.bytesPerLine());
      view.copyTo(out);
      break;
    }
  }
}

static void mat_to_qimage(cv::InputArray image, QImage& out) {
  switch (image.type()) {
    case CV_8UC4: {
      cv::Mat view(image.getMat());
      QImage view2(view.data, view.cols, view.rows, (qsizetype)view.step[0],
                   QImage::Format_ARGB32);
      out = view2.copy();
      break;
    }
    case CV_8UC3: {
      cv::Mat mat;
      cvtColor(image, mat,
               cv::COLOR_BGR2BGRA);  // COLOR_BGR2RGB doesn't behave so use RGBA
      QImage view(mat.data, mat.cols, mat.rows, qsizetype(mat.step[0]),
                  QImage::Format_ARGB32);
      out = view.copy();
      break;
    }
    case CV_8UC1: {
      cv::Mat mat;
      cvtColor(image, mat, cv::COLOR_GRAY2BGRA);
      QImage view(mat.data, mat.cols, mat.rows, (qsizetype)mat.step[0],
                  QImage::Format_ARGB32);
      out = view.copy();
      break;
    }
    default: {
      throw std::invalid_argument("Image format not supported");
      break;
    }
  }
}

static bool fileExists(const QString& path) {
  QFileInfo check_file(path);
  // check if file exists and if yes: Is it really a file and no directory?
  if (check_file.exists() && check_file.isFile()) {
    return true;
  } else {
    return false;
  }
}

static std::map<std::string, std::any> format_shape(Shape* s) {
  std::map<std::string, std::any> data;
  data = s->other_data_;
  data["label"] = s->label_.toStdString();
  std::vector<std::vector<double>> points{};
  for (auto& point : s->points) {
    std::vector<double> p{};
    p.emplace_back(point.x());
    p.emplace_back(point.y());
    points.emplace_back(p);
  }
  data["points"] = points;
  data["group_id"] = s->group_id_;
  data["shape_type"] = s->shape_type_.toStdString();
  data["flags"] = s->flags_;

  return data;
}

static QColor getYamlColor(const YAML::Node& node) {
  auto color = QColor();
  if (node.Type() == YAML::NodeType::Sequence) {
    auto r = node[0].as<int>();
    auto g = node[1].as<int>();
    auto b = node[2].as<int>();
    auto a = node[3].as<int>();
    color = QColor(r, g, b, a);
  }
  return color;
}

static QList<QKeySequence> getYamlKeySequence(const YAML::Node& node,
                                              const std::string& key) {
  QList<QKeySequence> shortcuts = {};
  if (auto shortcuts_node = node[key]) {
    if (shortcuts_node.IsNull()) {
      return shortcuts;
    }

    if (shortcuts_node.IsSequence()) {
      auto native_text_keys = shortcuts_node.as<std::vector<std::string>>();
      for (auto& key_text : native_text_keys) {
        shortcuts.emplaceBack(QKeySequence(QString::fromStdString(key_text)));
      }
      return shortcuts;
    }

    auto native_text_key = shortcuts_node.as<std::string>();
    shortcuts.emplaceBack(
        QKeySequence(QString::fromStdString(native_text_key)));
    return shortcuts;
  }

  return shortcuts;
}

app::app(const YAML::Node& config, const QString& filename,
         const QString& output, const QString& output_file,
         const QString& output_dir, QWidget* parent)
    : QMainWindow(parent) {
  QString _output_file = output_file;

  if (!output.isEmpty()) {
    LOG(WARNING)
        << "argument `output` is deprecated, use `output_file` instead";

    if (output_file.isEmpty()) {
      _output_file = output;
    }
  }

  // see labelme/config/default_config.yaml for valid configuration
  auto _config = config;
  if (config.IsNull()) {
    _config = get_config();
  }
  config_ = _config;

  //  // set default shape colors
  //  Shape shape;
  //  auto color = config_["shape"]["line_color"];
  //  shape.line_color = getYamlColor(color);
  //  color = config_["shape"]["fill_color"];
  //  shape.fill_color = getYamlColor(color);
  //  color = config_["shape"]["select_line_color"];
  //  shape.select_line_color = getYamlColor(color);
  //  color = config_["shape"]["select_fill_color"];
  //  shape.select_fill_color = getYamlColor(color);
  //  color = config_["shape"]["vertex_fill_color"];
  //  shape.vertex_fill_color = getYamlColor(color);
  //  color = config_["shape"]["hvertex_fill_color"];
  //  shape.hvertex_fill_color = getYamlColor(color);

  //  // set point size from config file
  //  auto point_size = config_["shape"]["point_size"];
  //  if (point_size.Type() == YAML::NodeType::Scalar) {
  //    shape.point_size = point_size.as<int>();
  //  }

  this->setWindowTitle(__appname__);

  // whether we need to save or not.
  dirty_ = false;
  _noSelectionSlot = false;
  _copied_shapes = {};

  // Main widgets and related state
  std::map<std::string, bool> fit_to_content{};
  if (config_["fit_to_content"] && config_["fit_to_content"].IsMap()) {
    fit_to_content =
        config_["fit_to_content"].as<std::map<std::string, bool>>();
  }

  std::map<std::string, std::vector<std::string>> flags{};
  if (config_["label_flags"] && !config_["label_flags"].IsNull()) {
    flags = config_["label_flags"]
                .as<std::map<std::string, std::vector<std::string>>>();
  }

  QStringList labels{};
  if (config_["labels"] && !config_["labels"].IsNull()) {
    for (auto& item : config_["labels"].as<std::vector<std::string>>()) {
      labels.emplace_back(QString::fromStdString(item));
    }
  }

  bool sort_labels = false;
  if (config_["sort_labels"]) {
    sort_labels = config_["sort_labels"].as<bool>();
  }

  bool show_text_field = false;
  if (config_["show_label_text_field"]) {
    show_text_field = config_["show_label_text_field"].as<bool>();
  }

  std::string completion{"startswith"};
  if (config_["label_completion"]) {
    completion = config_["label_completion"].as<std::string>();
  }

  labelDialog_ = new LabelDialog(fit_to_content, flags, labels, sort_labels,
                                 show_text_field, completion, this);

  labelList_ = new LabelListWidget();
  lastOpenDir_ = "";

  flag_dock_ = new QDockWidget(tr("Flags"), this);
  flag_dock_->setObjectName("Flags");

  flag_widget_ = new QListWidget();
  if (config_["flags"]) {
    std::map<std::string, bool> _flags;
    for (auto i = 0; i < config_["flags"].size(); ++i) {
      auto item = config_["flags"][i];
      _flags[item.as<std::string>()] = false;
    }
    loadFlags(_flags);
  }

  flag_dock_->setWidget(flag_widget_);
  QObject::connect(this->flag_widget_, SIGNAL(itemChanged(QListWidgetItem*)),
                   this, SLOT(setDirty()));

  QObject::connect(this->labelList_,
                   SIGNAL(itemSelectionChanged(QList<QStandardItem*>,
                                               QList<QStandardItem*>)),
                   this, SLOT(labelSelectionChanged()));
  QObject::connect(this->labelList_,
                   SIGNAL(itemDoubleClicked(LabelListWidgetItem*)), this,
                   SLOT(editLabel(LabelListWidgetItem*)));
  QObject::connect(this->labelList_, SIGNAL(itemChanged), this,
                   SLOT(labelItemChanged));
  QObject::connect(this->labelList_, SIGNAL(itemDropped), this,
                   SLOT(labelOrderChanged));

  shape_dock_ = new QDockWidget(tr("Polygon Labels"), this);
  shape_dock_->setObjectName("Labels");
  shape_dock_->setWidget(labelList_);

  uniqLabelList_ = new UniqueLabelQListWidget();
  uniqLabelList_->setToolTip(
      tr("Select label to start annotating for it. Press 'Esc' to deselect."));

  if (config_["labels"] && !config_["labels"].IsNull()) {
    for (const auto& label : config_["labels"].as<std::vector<std::string>>()) {
      auto q_label = QString::fromStdString(label);
      auto item = uniqLabelList_->createItemFromLabel(q_label);
      uniqLabelList_->addItem(item);
      std::vector<int> rgb = _get_rgb_by_label(q_label);
      uniqLabelList_->setItemLabel(item, q_label, rgb);
    }
  }

  label_dock_ = new QDockWidget(tr("Label List"), this);
  label_dock_->setObjectName("Label List");
  label_dock_->setWidget(uniqLabelList_);

  fileSearch_ = new QLineEdit();
  fileSearch_->setPlaceholderText(tr("Search Filename"));
  QObject::connect(this->fileSearch_, SIGNAL(textChanged(QString)), this,
                   SLOT(fileSearchChanged()));

  fileListWidget_ = new QListWidget();
  QObject::connect(fileListWidget_, SIGNAL(itemSelectionChanged()), this,
                   SLOT(fileSelectionChanged()));

  fileListLayout_ = new QVBoxLayout();
  fileListLayout_->setContentsMargins(0, 0, 0, 0);
  fileListLayout_->setSpacing(0);
  fileListLayout_->addWidget(fileSearch_);
  fileListLayout_->addWidget(fileListWidget_);

  file_dock_ = new QDockWidget(tr("File List"), this);
  file_dock_->setObjectName("Files");

  auto* fileListWidget = new QWidget();
  fileListWidget->setLayout(fileListLayout_);
  file_dock_->setWidget(fileListWidget);

  zoomWidget_ = new ZoomWidget();
  this->acceptDrops();

  double epsilon = 10.0;
  if (config_["epsilon"]) {
    epsilon = config_["epsilon"].as<double>();
  }

  QString double_click = "close";
  int num_backups = 10;
  std::map<std::string, bool> crosshair = {
      {"polygon", false}, {"rectangle", true}, {"circle", false},
      {"line", false},    {"point", false},    {"linestrip", false}};
  if (config_["canvas"]) {
    double_click = QString::fromStdString(
        config_["canvas"]["double_click"].as<std::string>());
    num_backups = config_["canvas"]["num_backups"].as<int>();
    crosshair =
        config_["canvas"]["crosshair"].as<std::map<std::string, bool>>();
  }

  canvas_ = new Canvas(epsilon, double_click, num_backups, crosshair);
  QObject::connect(canvas_, SIGNAL(zoomRequest(int, QPointF)), this,
                   SLOT(zoomRequest(int, QPointF)));

  scrollArea_ = new QScrollArea();
  scrollArea_->setWidget(canvas_);
  scrollArea_->setWidgetResizable(true);
  scrollBars_ = {{Qt::Vertical, scrollArea_->verticalScrollBar()},
                 {Qt::Horizontal, scrollArea_->horizontalScrollBar()}};

  QObject::connect(canvas_, SIGNAL(scrollRequest(int, int)), this,
                   SLOT(scrollRequest(int, int)));

  QObject::connect(canvas_, SIGNAL(newShape()), this, SLOT(newShape()));
  QObject::connect(canvas_, SIGNAL(shapedMove()), this, SLOT(setDirty()));
  QObject::connect(canvas_, SIGNAL(selectionChanged(QList<lmp::Shape*>)), this,
                   SLOT(shapeSelectionChanged(QList<lmp::Shape*>)));
  QObject::connect(canvas_, SIGNAL(drawingPolygon(bool)), this,
                   SLOT(toggleDrawingSensitive(bool)));

  setCentralWidget(scrollArea_);

  auto features = QDockWidget::DockWidgetFeatures();
  std::vector<QDockWidget*> dock_list = {
      flag_dock_,
      label_dock_,
      shape_dock_,
      file_dock_,
  };
  std::vector<std::string> dock_names = {
      "flag_dock",
      "label_dock",
      "shape_dock",
      "file_dock",
  };
  for (auto i = 0; i < dock_list.size(); ++i) {
    auto dock_name = dock_names[i];

    if (config_[dock_name]["closable"] &&
        config_[dock_name]["closable"].as<bool>()) {
      features = features | QDockWidget::DockWidgetClosable;
    }

    if (config_[dock_name]["floatable"] &&
        config_[dock_name]["floatable"].as<bool>()) {
      features = features | QDockWidget::DockWidgetFloatable;
    }

    if (config_[dock_name]["movable"] &&
        config_[dock_name]["movable"].as<bool>()) {
      features = features | QDockWidget::DockWidgetMovable;
    }

    auto& dock = dock_list[i];
    dock->setFeatures(features);
    if (config_[dock_name]["show"] && !config_[dock_name]["show"].as<bool>()) {
      dock->setVisible(false);
    }
  }

  this->addDockWidget(Qt::RightDockWidgetArea, flag_dock_);
  this->addDockWidget(Qt::RightDockWidgetArea, label_dock_);
  this->addDockWidget(Qt::RightDockWidgetArea, shape_dock_);
  this->addDockWidget(Qt::RightDockWidgetArea, file_dock_);

  auto shortcuts = config_["shortcuts"];

  auto shortcut = getYamlKeySequence(shortcuts, "close");
  quit_action_ =
      newAction(this, tr("&Quit"), shortcut, "quit", tr("Quit application"));
  QObject::connect(quit_action_, SIGNAL(triggered(bool)), this, SLOT(close()));

  shortcut = getYamlKeySequence(shortcuts, "open");
  open_action_ = newAction(this, tr("&Open"), shortcut, "open",
                           tr("Open image or label file"));
  QObject::connect(open_action_, SIGNAL(triggered(bool)), this,
                   SLOT(openFile(bool)));

  shortcut = getYamlKeySequence(shortcuts, "open_dir");
  opendir_action_ =
      newAction(this, tr("&Open Dir"), shortcut, "open", tr("Open Dir"));
  QObject::connect(opendir_action_, SIGNAL(triggered(bool)), this,
                   SLOT(openDirDialog(bool)));

  shortcut = getYamlKeySequence(shortcuts, "open_next");
  open_next_img_action_ =
      newAction(this, tr("&Next Image"), shortcut, "next",
                tr("Open next (hold Ctl+Shift to copy labels)"), false, false);
  QObject::connect(open_next_img_action_, SIGNAL(triggered(bool)), this,
                   SLOT(openNextImg(bool)));

  shortcut = getYamlKeySequence(shortcuts, "open_prev");
  open_prev_img_action_ =
      newAction(this, tr("&Prev Image"), shortcut, "prev",
                tr("Open prev (hold Ctl+Shift to copy labels)"), false, false);
  QObject::connect(open_prev_img_action_, SIGNAL(triggered(bool)), this,
                   SLOT(openPrevImg(bool)));

  shortcut = getYamlKeySequence(shortcuts, "save");
  save_action_ = newAction(this, tr("&Save"), shortcut, "save",
                           tr("save labels to file"), false, false);
  QObject::connect(save_action_, SIGNAL(triggered(bool)), this,
                   SLOT(saveFile(bool)));

  shortcut = getYamlKeySequence(shortcuts, "save_as");
  save_as_action_ =
      newAction(this, tr("&Save as"), shortcut, "save-as",
                tr("save labels to a different file"), false, false);
  QObject::connect(save_as_action_, SIGNAL(triggered(bool)), this,
                   SLOT(saveFileAs(bool)));

  shortcut = getYamlKeySequence(shortcuts, "delete_file");
  delete_file_action_ =
      newAction(this, tr("&Delete File"), shortcut, "delete",
                tr("Delete current label file"), false, false);
  QObject::connect(delete_file_action_, SIGNAL(triggered(bool)), this,
                   SLOT(deleteFile()));

  shortcut = getYamlKeySequence(shortcuts, "save_to");
  change_output_dir_action_ =
      newAction(this, tr("&Change Output Dir"), shortcut, "open",
                tr("change where annotations are load/saved"));
  QObject::connect(change_output_dir_action_, SIGNAL(triggered(bool)), this,
                   SLOT(changeOutputDirDialog(bool)));

  shortcut = {};
  save_auto_action_ = newAction(this, tr("Save &Automatically"), shortcut,
                                "save", tr("Save automatically"), true, true);
  QObject::connect(save_auto_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onSaveAutoTriggered(bool)));
  save_auto_action_->setChecked(config_["auto_save"].as<bool>());

  shortcut = {};
  save_with_image_data_action_ =
      newAction(this, tr("Save With Image Data"), shortcut, "",
                tr("Save image data in label file"), true, true,
                config_["store_data"].as<bool>());
  QObject::connect(save_with_image_data_action_, SIGNAL(triggered(bool)), this,
                   SLOT(enableSaveImageWithData(bool)));

  shortcut = getYamlKeySequence(shortcuts, "close");
  close_action_ = newAction(this, tr("&Close"), shortcut, "close",
                            tr("Close current file"));
  QObject::connect(close_action_, SIGNAL(triggered(bool)), this,
                   SLOT(closeFile(bool)));

  shortcut = getYamlKeySequence(shortcuts, "toggle_keep_prev_mode");
  toggle_keep_prev_mode_action_ =
      newAction(this, tr("Key Previous Annotation"), shortcut, "",
                tr("Toggle \"Keep previous annotation\" mode"), true);
  QObject::connect(toggle_keep_prev_mode_action_, SIGNAL(triggered(bool)), this,
                   SLOT(toggleKeepPrevMode()));
  toggle_keep_prev_mode_action_->setChecked(config_["keep_prev"].as<bool>());

  shortcut = getYamlKeySequence(shortcuts, "create_polygon");
  create_mode_action_ =
      newAction(this, tr("Create Polygons"), shortcut, "objects",
                tr("Start drawing polygons"), false, false);
  QObject::connect(create_mode_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onCreateModeTriggered()));

  shortcut = getYamlKeySequence(shortcuts, "create_rectangle");
  create_rectangle_mode_action_ =
      newAction(this, tr("Create Rectangle"), shortcut, "objects",
                tr("Start drawing rectangles"), false, false);
  QObject::connect(create_rectangle_mode_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onCreateRectangleModeTriggered()));

  shortcut = getYamlKeySequence(shortcuts, "create_circle");
  create_circle_mode_action_ =
      newAction(this, tr("Create Circle"), shortcut, "objects",
                tr("Start drawing circles"), false, false);
  QObject::connect(create_circle_mode_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onCreateCircleModeTriggered()));

  shortcut = getYamlKeySequence(shortcuts, "create_line");
  create_line_mode_action_ =
      newAction(this, tr("Create Line"), shortcut, "objects",
                tr("Start drawing lines"), false, false);
  QObject::connect(create_line_mode_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onCreateLineModeTriggered()));

  shortcut = getYamlKeySequence(shortcuts, "create_point");
  create_point_mode_action_ =
      newAction(this, tr("Create Point"), shortcut, "objects",
                tr("Start drawing points"), false, false);
  QObject::connect(create_point_mode_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onCreatePointModeTriggered()));

  shortcut = getYamlKeySequence(shortcuts, "create_linestrip");
  create_linestrip_mode_action_ =
      newAction(this, tr("Create LineStrip"), shortcut, "objects",
                tr("Start drawing linestrip, Ctrl+LeftClick ends creation"),
                false, false);
  QObject::connect(create_linestrip_mode_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onCreateLinestripModeTriggered()));

  shortcut = getYamlKeySequence(shortcuts, "edit_polygon");
  edit_mode_action_ =
      newAction(this, tr("Edit Polygons"), shortcut, "edit",
                tr("Move and edit the selected polygons"), false, false);
  QObject::connect(edit_mode_action_, SIGNAL(triggered(bool)), this,
                   SLOT(setEditMode()));

  shortcut = getYamlKeySequence(shortcuts, "delete_polygon");
  delete_mode_action_ =
      newAction(this, tr("Delete Polygons"), shortcut, "cancel",
                tr("Delete the selected polygons"), false, false);
  QObject::connect(delete_mode_action_, SIGNAL(triggered(bool)), this,
                   SLOT(deleteSelectedShape()));

  shortcut = getYamlKeySequence(shortcuts, "duplicate_polygon");
  duplicate_action_ = newAction(
      this, tr("Duplicate Polygon"), shortcut, "copy",
      tr("Create a duplicate of the selected polygons"), false, false);
  QObject::connect(duplicate_action_, SIGNAL(triggered(bool)), this,
                   SLOT(duplicateSelectedShape()));

  shortcut = getYamlKeySequence(shortcuts, "copy_polygon");
  copy_action_ =
      newAction(this, tr("Copy Polygon"), shortcut, "copy_clipboard",
                tr("Copy selected polygons to clipboard"), false, false);
  QObject::connect(copy_action_, SIGNAL(triggered(bool)), this,
                   SLOT(copySelectedShape()));

  shortcut = getYamlKeySequence(shortcuts, "paste_polygon");
  paste_action_ =
      newAction(this, tr("Paste Polygon"), shortcut, "paste_polygon",
                tr("Paste copied polygons"), false, false);
  QObject::connect(paste_action_, SIGNAL(triggered(bool)), this,
                   SLOT(pasteSelectedShape()));

  shortcut = getYamlKeySequence(shortcuts, "undo_last_point");
  undo_last_point_action_ =
      newAction(this, tr("Undo Last Point"), shortcut, "undo",
                tr("Undo last drawn point"), false, false);
  QObject::connect(undo_last_point_action_, SIGNAL(triggered(bool)),
                   this->canvas_, SLOT(undoLastPoint()));

  shortcut = getYamlKeySequence(shortcuts, "remove_selected_point");
  remove_point_action_ =
      newAction(this, tr("Remove Selected Point"), shortcut, "edit",
                tr("Remove selected point from polygon"), false, false);
  QObject::connect(remove_point_action_, SIGNAL(triggered(bool)), this,
                   SLOT(removeSelectedPoint()));

  shortcut = getYamlKeySequence(shortcuts, "undo");
  undo_action_ = newAction(this, tr("Undo"), shortcut, "undo",
                           tr("Undo last add and edit of shape"), false, false);
  QObject::connect(undo_action_, SIGNAL(triggered(bool)), this,
                   SLOT(undoShapeEdit()));

  shortcut = {};
  hide_all_action_ = newAction(this, tr("&Hide\nPolygons"), shortcut, "eye",
                               tr("Hide all polygons"), false, false);
  QObject::connect(hide_all_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onHideAllTriggered()));

  shortcut = {};
  show_all_action_ = newAction(this, tr("&Show\nPolygons"), shortcut, "eye",
                               tr("Show all polygons"), false, false);
  auto show_all = [this]() { this->togglePolygons(true); };
  QObject::connect(show_all_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onShowAllTriggered()));

  shortcut = {};
  help_action_ = newAction(this, tr("&Tutorial"), shortcut, "help",
                           tr("Show tutorial page"));
  QObject::connect(help_action_, SIGNAL(triggered(bool)), this,
                   SLOT(tutorial()));

  auto* zoom = new QWidgetAction(this);
  zoom->setDefaultWidget(zoomWidget_);
  //  auto zoom_in_shortcut = getYamlKeySequence(shortcuts, "zoom_in");
  //  auto zoom_out_shortcut = getYamlKeySequence(shortcuts, "zoom_out");
  std::string what_is_this{};
  // TODO: fix it.
  //  auto _ = QString("%1, %2").arg(zoom_in_shortcut).arg(zoom_out_shortcut);
  //  std::string what_is_this = fmt::format("Zoom in or out of the image. Also
  //  accessible with {} and {} from the canvas.",
  //                               fmtShortcut(QString::fromStdString(_)).toStdString(),
  //                               fmtShortcut("Ctrl+Wheel").toStdString());
  zoomWidget_->setWhatsThis(QString::fromStdString(what_is_this));
  zoomWidget_->setEnabled(false);

  shortcut = getYamlKeySequence(shortcuts, "zoom_in");
  zoomIn_action_ = newAction(this, tr("Zoom &In"), shortcut, "zoom-in",
                             tr("Increase zoom level"), false, false);
  QObject::connect(zoomIn_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onZoomInTriggered()));

  shortcut = getYamlKeySequence(shortcuts, "zoom_out");
  zoomOut_action_ = newAction(this, tr("&Zoom Out"), shortcut, "zoom-out",
                              tr("Decrease zoom level"), false, false);
  QObject::connect(zoomOut_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onZoomOutTriggered()));

  shortcut = getYamlKeySequence(shortcuts, "zoom_to_original");
  zoomOrg_action_ = newAction(this, tr("&Original size"), shortcut, "zoom",
                              tr("Zoom to original size"), false, false);
  QObject::connect(zoomOrg_action_, SIGNAL(triggered(bool)), this,
                   SLOT(onZoomOrgTriggered()));

  shortcut = getYamlKeySequence(shortcuts, "keep_prev_scale");
  keepPrevScale_action_ =
      newAction(this, tr("&Keep previous Scale"), shortcut, "",
                tr("Keep previous zoom scale"), true, true,
                config_["keep_prev_scale"].as<bool>());
  QObject::connect(keepPrevScale_action_, SIGNAL(triggered(bool)), this,
                   SLOT(enableKeepPrevScale(bool)));

  shortcut = getYamlKeySequence(shortcuts, "fit_window");
  fitWindow_action_ = newAction(this, tr("&Fit Window"), shortcut, "fit-window",
                                tr("Zoom follows window size"), true, false);
  QObject::connect(fitWindow_action_, SIGNAL(triggered(bool)), this,
                   SLOT(setFitWindow(bool)));

  shortcut = getYamlKeySequence(shortcuts, "fit_width");
  fitWidth_action_ = newAction(this, tr("Fit &Width"), shortcut, "fit-width",
                               tr("Zoom follows window width"), true, false);
  QObject::connect(fitWidth_action_, SIGNAL(triggered(bool)), this,
                   SLOT(setFitWidth(bool)));

  shortcut = {};
  brightnessContrast_action_ =
      newAction(this, tr("&Brightness Contrast"), shortcut, "color",
                tr("Adjust brightness and contrast"), false, false);
  QObject::connect(brightnessContrast_action_, SIGNAL(triggered(bool)), this,
                   SLOT(brightnessContrast(bool)));

  // Group zoom controls into a list for easier toggling.
  zoomActions_ = {zoomIn_action_, zoomOut_action_, zoomOrg_action_,
                  fitWindow_action_, fitWidth_action_};

  fitWindow_action_->setChecked(Qt::Checked);

  shortcut = getYamlKeySequence(shortcuts, "edit_label");
  edit_action_ =
      newAction(this, tr("&Edit Label"), shortcut, "edit",
                tr("Modify the label of the selected polygon"), false, false);
  QObject::connect(edit_action_, SIGNAL(triggered(bool)), this,
                   SLOT(editLabel()));

  shortcut = {};
  fill_drawing_action_ =
      newAction(this, tr("Fill Drawing Polygon"), shortcut, "color",
                tr("Fill polygon while drawing"), true, true);
  QObject::connect(fill_drawing_action_, SIGNAL(triggered(bool)), this->canvas_,
                   SLOT(setFillDrawing(bool)));
  fill_drawing_action_->trigger();

  // label list context menu
  labelMenu_ = new QMenu();
  QList<QAction*> action_list = {edit_action_, delete_mode_action_};
  ::addActions(labelMenu_, action_list);
  labelList_->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(labelList_, SIGNAL(customContextMenuRequested(QPoint)), this,
                   SLOT(popLabelListMenu(QPoint)));

  // need to add some actions here to activate the shortcut
  edit_menu_actions_ = {edit_action_,
                        duplicate_action_,
                        delete_mode_action_,
                        nullptr,
                        undo_action_,
                        undo_last_point_action_,
                        nullptr,
                        remove_point_action_,
                        nullptr,
                        toggle_keep_prev_mode_action_};
  // menu shown at right click
  menu_ = {
      create_mode_action_,
      create_rectangle_mode_action_,
      create_circle_mode_action_,
      create_line_mode_action_,
      create_point_mode_action_,
      create_linestrip_mode_action_,
      edit_mode_action_,
      edit_action_,
      duplicate_action_,
      copy_action_,
      paste_action_,
      delete_mode_action_,
      undo_action_,
      undo_last_point_action_,
      remove_point_action_,
  };

  onLoadActive_ = {close_action_,
                   create_mode_action_,
                   create_rectangle_mode_action_,
                   create_circle_mode_action_,
                   create_line_mode_action_,
                   create_point_mode_action_,
                   create_linestrip_mode_action_,
                   edit_mode_action_,
                   brightnessContrast_action_};

  onShapesPresent_actions_ = {save_as_action_, hide_all_action_,
                              show_all_action_};

  QObject::connect(canvas_, SIGNAL(vertexSelected(bool)), remove_point_action_,
                   SLOT(setEnabled(bool)));

  // Menus
  file_menu_ = menu(tr("&File"));
  edit_menu_ = menu(tr("&Edit"));
  view_menu_ = menu(tr("&view"));
  help_menu_ = menu(tr("&Help"));
  recentFiles_menu_ = new QMenu(tr("Open &Recent"));
  labelList_menu_ = labelMenu_;

  // add file menu
  QList<QAction*> file_menu_actions{
      open_action_,
      open_next_img_action_,
      open_prev_img_action_,
      opendir_action_,
  };
  ::addActions(file_menu_, file_menu_actions);
  file_menu_->addMenu(recentFiles_menu_);
  file_menu_actions = {save_action_,
                       save_as_action_,
                       save_auto_action_,
                       change_output_dir_action_,
                       save_with_image_data_action_,
                       close_action_,
                       delete_file_action_,
                       nullptr,
                       quit_action_};
  ::addActions(file_menu_, file_menu_actions);

  help_menu_->addAction(help_action_);

  QList<QAction*> view_menu_actions{
      flag_dock_->toggleViewAction(),
      label_dock_->toggleViewAction(),
      shape_dock_->toggleViewAction(),
      file_dock_->toggleViewAction(),
      nullptr,
      fill_drawing_action_,
      nullptr,
      hide_all_action_,
      show_all_action_,
      nullptr,
      zoomIn_action_,
      zoomOut_action_,
      zoomOrg_action_,
      keepPrevScale_action_,
      nullptr,
      fitWindow_action_,
      fitWidth_action_,
      nullptr,
      brightnessContrast_action_,
  };

  ::addActions(view_menu_, view_menu_actions);

  QObject::connect(file_menu_, SIGNAL(aboutToShow()), this,
                   SLOT(updateFileMenu()));

  // Custom context menu for the canvas widget:
  ::addActions(canvas_->menus[0], menu_);
  copy_here_action_ = newAction(this, tr("&Copy here"));
  move_here_action_ = newAction(this, tr("&Move here"));
  QList<QAction*> copy_move_actions{copy_here_action_, move_here_action_};
  ::addActions(canvas_->menus[1], copy_move_actions);

  tools_ = toolbar("Tools");
  this->statusBar()->showMessage(QString(tr("%1 Started.")).arg(__appname__));
  this->statusBar()->show();

  if (!output_file.isEmpty() && config_["auto_save"].as<bool>()) {
    LOG(WARNING) << "If `auto_save` argument is True, `output_file` argument "
                    "is ignored and output filename is automatically set as "
                    "<IMAGE_BASENAME>.json";
  }
  output_file_ = output_file;
  output_dir_ = output_dir;

  // Application state.
  image_ = new QImage();

  if (!filename.isEmpty() && QFileInfo(filename).isDir()) {
    importDirImages(filename, {}, false);
  } else {
    filename_ = filename;
  }

  if (config_["file_search"]) {
    auto file_search = config_["file_search"].as<std::string>();
    fileSearch_->setText(QString::fromStdString(file_search));
    fileSearchChanged();
  }

  // Could be completely declarative.
  // Restore application settings.
  settings_ = new QSettings(tr("labelmeplus"), tr("labelmeplus"));
  recentFiles_ =
      settings_->value("recentFiles", QList<QString>{}).toStringList();
  auto size = settings_->value("window/size", QSize(600, 500)).toSize();
  auto position = settings_->value("window/position", QPoint(0, 0)).toPoint();
  auto state = settings_->value("window/state", QByteArray()).toByteArray();

  this->resize(size);
  this->move(position);

  // or simply:
  // restoreGeometry(settings['window/geometry']
  restoreState(state);

  // Populate the File menu dynamically.
  updateFileMenu();
  // Since loading the file may take some time,
  // make sure it runs in the background.
  if (!filename.isEmpty()) {
    auto load_file = [this]() { this->loadFile(this->filename_); };
    queueEvent(load_file);
  }

  // Callbacks:
  QObject::connect(zoomWidget_, SIGNAL(valueChanged(int)), this,
                   SLOT(paintCanvas()));

  populateModeActions();
}

app::~app() = default;

QMenu* app::menu(const QString& title, const QList<QAction*>& actions) {
  QMenu* menu = this->menuBar()->addMenu(title);
  if (!actions.isEmpty()) {
    ::addActions(menu, actions);
  }

  return menu;
}

ToolBar* app::toolbar(const QString& title, const QList<QAction*>& actions) {
  auto* toolbar = new ToolBar(title);
  toolbar->setObjectName(QString("%1 - ToolBar").arg(title));
  toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  if (!actions.empty()) {
    ::addActions(toolbar, actions);
  }
  this->addToolBar(Qt::LeftToolBarArea, toolbar);
  return toolbar;
}

bool app::noShapes() { return labelList_->model()->rowCount() != 0; }

void app::populateModeActions() {
  tools_->clear();
  ::addActions(tools_, tool_);
  canvas_->menus[0]->clear();
  ::addActions(canvas_->menus[0], menu_);
  edit_menu_->clear();
  QList<QAction*> actions{
      create_mode_action_,        create_rectangle_mode_action_,
      create_circle_mode_action_, create_line_mode_action_,
      create_point_mode_action_,  create_linestrip_mode_action_,
      edit_mode_action_};
  actions = actions + edit_menu_actions_;
  ::addActions(edit_menu_, actions);
}

// Even if we auto-save the file, we keep the ability to undo.
void app::setDirty() {
  undo_action_->setEnabled(canvas_->isShapeRestorable());

  if (config_["auto_save"].as<bool>() || save_auto_action_->isChecked()) {
    auto basename = QFileInfo(imagePath_).completeBaseName();
    auto dirname = QFileInfo(imagePath_).dir().path();
    auto label_file =
        QFileInfo(QDir(dirname).filePath(QString("%1.json").arg(basename)))
            .absoluteFilePath();
    if (!output_dir_.isEmpty()) {
      auto label_file_without_path = QFileInfo(label_file).fileName();
      label_file =
          QFileInfo(QDir(output_dir_).filePath(label_file_without_path))
              .absoluteFilePath();
    }
    saveLabels(label_file);
    return;
  }
  dirty_ = true;
  save_action_->setEnabled(true);
  auto title = __appname__;
  if (!filename_.isEmpty()) {
    title = QString("%1 - %2").arg(title).arg(filename_);
  }
  this->setWindowTitle(title);
}

void app::setClean() {
  dirty_ = false;
  save_action_->setEnabled(false);
  create_mode_action_->setEnabled(true);
  create_rectangle_mode_action_->setEnabled(true);
  create_circle_mode_action_->setEnabled(true);
  create_line_mode_action_->setEnabled(true);
  create_point_mode_action_->setEnabled(true);
  create_linestrip_mode_action_->setEnabled(true);
  auto title = __appname__;
  if (!filename_.isEmpty()) {
    title = QString("%1 - %2").arg(title).arg(filename_);
  }
  this->setWindowTitle(title);

  if (hasLabelFile()) {
    delete_file_action_->setEnabled(true);
  } else {
    delete_file_action_->setEnabled(false);
  }
}

// Enable/Disable widgets which depend on an opened image.
void app::toggleActions(const bool value) {
  zoomWidget_->setEnabled(value);

  for (auto& z : zoomActions_) {
    z->setEnabled(value);
  }

  for (auto& z : onLoadActive_) {
    z->setEnabled(value);
  }
}

template <typename Functor>
void app::queueEvent(Functor function) {
  QTimer::singleShot(0, function);
}

void app::status(const QString& message, int delay) {
  this->statusBar()->showMessage(message, delay);
}

void app::resetState() {
  labelList_->clear();
  filename_ = {};
  imagePath_ = {};
  imageData_ = {};
  labelFile_ = {};
  otherData_ = nullptr;
  canvas_->resetState();
}

LabelListWidgetItem* app::currentItem() {
  auto items = labelList_->selectedItems();
  if (!items.empty()) {
    return items[0];
  }
  return nullptr;
}

void app::addRecentFile(const QString& filename) {
  if (std::find(recentFiles_.begin(), recentFiles_.end(), filename) !=
      recentFiles_.end()) {
    recentFiles_.removeAll(filename);
  } else if (recentFiles_.size() >= maxRecent_) {
    recentFiles_.insert(0, filename);
  }
}

void app::undoShapeEdit() {
  canvas_->restoreShape();
  labelList_->clear();
  loadShapes(canvas_->shapes_);
  undo_action_->setEnabled(canvas_->isShapeRestorable());
}

void app::tutorial() {
  //    QString url =
  //    "https://github.com/wkentaro/labelme/tree/main/examples/tutorial";
  //    webbrowser.open(url);
  opener("https://github.com/wkentaro/labelme/tree/main/examples/tutorial");
}

/* Toggle drawing sensitive.
 *
 * In the middle of drawing, toggling between modes should be disabled.
 */
void app::toggleDrawingSensitive(const bool drawing) {
  edit_mode_action_->setEnabled(!drawing);
  undo_last_point_action_->setEnabled(drawing);
  undo_action_->setEnabled(!drawing);
  delete_file_action_->setEnabled(!drawing);
}

void app::toggleDrawMode(const bool edit, const QString& createMode) {
  canvas_->setEditing(edit);
  canvas_->createMode(createMode);
  if (edit) {
    create_mode_action_->setEnabled(true);
    create_rectangle_mode_action_->setEnabled(true);
    create_circle_mode_action_->setEnabled(true);
    create_line_mode_action_->setEnabled(true);
    create_point_mode_action_->setEnabled(true);
    create_linestrip_mode_action_->setEnabled(true);
  } else {
    if (createMode == "polygon") {
      create_mode_action_->setEnabled(false);
      create_rectangle_mode_action_->setEnabled(true);
      create_circle_mode_action_->setEnabled(true);
      create_line_mode_action_->setEnabled(true);
      create_point_mode_action_->setEnabled(true);
      create_linestrip_mode_action_->setEnabled(true);
    }

    if (createMode == "rectangle") {
      create_mode_action_->setEnabled(true);
      create_rectangle_mode_action_->setEnabled(false);
      create_circle_mode_action_->setEnabled(true);
      create_line_mode_action_->setEnabled(true);
      create_point_mode_action_->setEnabled(true);
      create_linestrip_mode_action_->setEnabled(true);
    }

    if (createMode == "line") {
      create_mode_action_->setEnabled(true);
      create_rectangle_mode_action_->setEnabled(true);
      create_rectangle_mode_action_->setEnabled(true);
      create_circle_mode_action_->setEnabled(true);
      create_line_mode_action_->setEnabled(false);
      create_point_mode_action_->setEnabled(true);
      create_linestrip_mode_action_->setEnabled(true);
    }

    if (createMode == "point") {
      create_mode_action_->setEnabled(true);
      create_rectangle_mode_action_->setEnabled(true);
      create_circle_mode_action_->setEnabled(true);
      create_line_mode_action_->setEnabled(true);
      create_point_mode_action_->setEnabled(false);
      create_linestrip_mode_action_->setEnabled(true);
    }

    if (createMode == "circle") {
      create_mode_action_->setEnabled(true);
      create_rectangle_mode_action_->setEnabled(true);
      create_circle_mode_action_->setEnabled(false);
      create_line_mode_action_->setEnabled(true);
      create_point_mode_action_->setEnabled(true);
      create_linestrip_mode_action_->setEnabled(true);
    }

    if (createMode == "linestrip") {
      create_mode_action_->setEnabled(true);
      create_rectangle_mode_action_->setEnabled(true);
      create_circle_mode_action_->setEnabled(true);
      create_line_mode_action_->setEnabled(true);
      create_point_mode_action_->setEnabled(true);
      create_linestrip_mode_action_->setEnabled(false);
    }
  }
  edit_mode_action_->setEnabled(!edit);
}

void app::setEditMode() { toggleDrawMode(true); }

void app::updateFileMenu() {
  auto current = filename_;
  auto menu = recentFiles_menu_;
  menu->clear();
  QList<QString> files;
  for (auto& file : recentFiles_) {
    if (file != current && fileExists(file)) {
      files.emplaceBack(file);
    }
  }

  for (auto i = 0; i < files.size(); ++i) {
    auto f = files[i];
    auto icon = newIcon("labels");
    auto* action = new QAction(
        icon, QString("%1 %2").arg(i + 1).arg(QFileInfo(f).fileName()), this);
    auto load_recent = [this, f]() { this->loadRecent(f); };
    QObject::connect(action, SIGNAL(triggered), this, SLOT(load_recent));
    menu->addAction(action);
  }
}

void app::popLabelListMenu(const QPoint& point) {
  labelList_menu_->exec(labelList_->mapToGlobal(point));
}

bool app::validateLabel(const QString& label) {
  const std::vector<std::string> validate_label_list{"exact"};
  std::string validate_label{};

  // no validation
  if (config_["validate_label"].IsNull()) {
    return true;
  } else {
    validate_label = config_["validate_label"].as<std::string>();
  }

  for (auto i = 0; i < uniqLabelList_->count(); ++i) {
    QVariant label_i = uniqLabelList_->item(i)->data(Qt::UserRole);

    if (std::find(validate_label_list.begin(), validate_label_list.end(),
                  validate_label) != validate_label_list.end()) {
      if (label_i.toString() == label) {
        return true;
      }
    }
  }

  return false;
}

void app::editLabel(LabelListWidgetItem* item) {
  if (!canvas_->editing()) {
    return;
  }

  if (item == nullptr) {
    item = currentItem();
  }

  if (item == nullptr) {
    return;
  }

  auto shape = item->shape();
  if (shape == nullptr) {
    return;
  }

  QString text{};
  std::map<std::string, bool> flags{};
  int group_id = -1;
  labelDialog_->popUp(text, flags, group_id, shape->label_, true, shape->flags_,
                      shape->group_id_);

  if (text.isEmpty()) {
    return;
  }

  if (!validateLabel(text)) {
    QString validate_label{};
    if (!config_["validate_label"].IsNull()) {
      validate_label =
          QString::fromStdString(config_["validate_label"].as<std::string>());
    }
    errorMessage(tr("Invalid label"),
                 QString(tr("Invalid label %1 with validation type %2"))
                     .arg(text)
                     .arg(validate_label));

    return;
  }
  shape->label_ = text;
  shape->flags_ = flags;
  shape->group_id_ = group_id;

  _update_shape_color(shape);

  if (shape->group_id_ < 0) {
    int r = 0, g = 0, b = 0;
    shape->fill_color.getRgb(&r, &g, &b);
    auto label_html_escaped = shape->label_.toHtmlEscaped();
    auto content = QString("%1 <font color=\"#%2%3%4\">•</font>")
                       .arg(label_html_escaped)
                       .arg(r, 2, 16)
                       .arg(g, 2, 16)
                       .arg(b, 2, 16);
    item->setText(content);
  } else {
    auto content = QString("%1 (%2)").arg(shape->label_).arg(shape->group_id_);
    item->setText(content);
  }

  setDirty();

  if (uniqLabelList_->findItemByLabel(shape->label_) == nullptr) {
    auto item = uniqLabelList_->createItemFromLabel(shape->label_);
    uniqLabelList_->addItem(item);
    auto rgb = _get_rgb_by_label(shape->label_);
    uniqLabelList_->setItemLabel(item, shape->label_, rgb);
  }
}

void app::fileSearchChanged() {
  importDirImages(lastOpenDir_, fileSearch_->text(), false);
}

void app::fileSelectionChanged() {
  auto items = fileListWidget_->selectedItems();
  if (items.empty()) {
    return;
  }
  auto item = items[0];

  if (!mayContinue()) {
    return;
  }

  auto image_list = imageList();
  auto currentIndex = image_list.indexOf(item->text());
  if (currentIndex < image_list.size()) {
    auto filename = image_list[currentIndex];
    if (filename != nullptr) {
      loadFile(filename);
    }
  }
}

void app::shapeSelectionChanged(const QList<Shape*>& selected_shapes) {
  _noSelectionSlot = true;
  for (auto& shape : canvas_->selectedShapes_) {
    shape->selected = false;
  }
  labelList_->clearSelection();
  canvas_->selectedShapes_ = selected_shapes;
  for (auto& shape : canvas_->selectedShapes_) {
    shape->selected = true;
    auto item = labelList_->findItemByShape(shape);
    labelList_->selectItem(item);
    labelList_->scrollToItem(item);
  }
  _noSelectionSlot = false;
  auto n_selected = selected_shapes.size();
  delete_mode_action_->setEnabled(n_selected != 0);
  duplicate_action_->setEnabled(n_selected != 0);
  copy_action_->setEnabled(n_selected != 0);
  edit_action_->setEnabled(n_selected == 1);
}

void app::addLabel(Shape* shape) {
  QString text{};
  if (shape->group_id_ < 0) {
    text = shape->label_;
  } else {
    text = QString("%1 (%2)").arg(shape->label_).arg(shape->group_id_);
  }

  auto* label_list_item = new LabelListWidgetItem(text, shape);
  labelList_->addItem(label_list_item);
  if (uniqLabelList_->findItemByLabel(shape->label_) == nullptr) {
    auto item = uniqLabelList_->createItemFromLabel(shape->label_);
    uniqLabelList_->addItem(item);
    auto rgb = _get_rgb_by_label(shape->label_);
    uniqLabelList_->setItemLabel(item, shape->label_, rgb);
  }
  labelDialog_->addLabelHistory(shape->label_);
  for (auto& action : onShapesPresent_actions_) {
    action->setEnabled(true);
  }

  _update_shape_color(shape);
  int r = 0, g = 0, b = 0;
  shape->fill_color.getRgb(&r, &g, &b);
  auto label_html_escaped = shape->label_.toHtmlEscaped();
  auto content = QString("%1 <font color=\"#%2%3%4\">•</font>")
                     .arg(label_html_escaped)
                     .arg(r, 2, 16)
                     .arg(g, 2, 16)
                     .arg(b, 2, 16);
  label_list_item->setText(content);
}

void app::_update_shape_color(Shape* shape) {
  auto rgb = _get_rgb_by_label(shape->label_);
  shape->line_color = QColor(rgb[0], rgb[1], rgb[2]);
  shape->vertex_fill_color = QColor(rgb[0], rgb[1], rgb[2]);
  shape->hvertex_fill_color = QColor(255, 255, 255);
  shape->fill_color = QColor(rgb[0], rgb[1], rgb[2], 128);
  shape->select_line_color = QColor(255, 255, 255);
  shape->select_fill_color = QColor(rgb[0], rgb[1], rgb[2], 128);
}

std::vector<int> app::_get_rgb_by_label(const QString& label) {
  if (config_["shape_color"].as<std::string>() == "auto") {
    auto item = uniqLabelList_->findItemByLabel(label);
    if (item == nullptr) {
      item = uniqLabelList_->createItemFromLabel(label);
      uniqLabelList_->addItem(item);
      auto rgb = _get_rgb_by_label(label);
      uniqLabelList_->setItemLabel(item, label, rgb);
    }
    auto label_id = uniqLabelList_->indexFromItem(item).row() + 1;
    label_id += config_["shift_auto_shape_color"].as<int>();
    auto color = imgviz::getLabelColor(label_id % 255);
    return {color[0], color[1], color[2]};
  } else if (config_["shape_color"].as<std::string>() == "manual" &&
             config_["label_colors"] &&
             config_["label_colors"][label.toStdString()]) {
    auto color = config_["label_colors"][label.toStdString()];
    return {color[0].as<int>(), color[1].as<int>(), color[2].as<int>()};
  } else if (config_["default_shape_color"]) {
    auto color = config_["default_shape_color"][label.toStdString()];
    return {color[0].as<int>(), color[1].as<int>(), color[2].as<int>()};
  }

  return {0, 255, 0};
}

void app::remLabels(QList<Shape*> shapes) {
  for (auto& shape : shapes) {
    auto item = labelList_->findItemByShape(shape);
    labelList_->removeItem(item);
  }
}

void app::loadShapes(QList<Shape*> shapes, bool replace) {
  _noSelectionSlot = true;
  for (auto& shape : shapes) {
    addLabel(shape);
  }
  labelList_->clearSelection();
  _noSelectionSlot = false;
  canvas_->loadShapes(shapes, replace);
}

void app::loadLabels(
    const std::vector<std::map<std::string, std::any>>& shapes) {
  QList<Shape*> s;
  for (auto& node : shapes) {
    QString label{};
    if (node.find("label") != node.end()) {
      auto item = node.at("label");
      label = QString::fromStdString(std::any_cast<std::string>(item));
    }

    std::vector<std::vector<double>> points;
    if (node.find("points") != node.end()) {
      auto item = node.at("points");
      points = std::any_cast<std::vector<std::vector<double>>>(item);
    }
    QString shape_type{};
    if (node.find("shape_type") != node.end()) {
      auto item = node.at("shape_type");
      shape_type = QString::fromStdString(std::any_cast<std::string>(item));
    }

    std::map<std::string, bool> flags{};
    if (node.find("flags") != node.end()) {
      auto item = node.at("flags");
      flags = std::any_cast<std::map<std::string, bool>>(item);
    }
    int group_id = -1;
    if (node.find("group_id") != node.end()) {
      auto item = node.at("group_id");
      group_id = std::any_cast<int>(item);
    }

    std::map<std::string, std::any> other_data{};
    if (node.find("otherData") != node.end()) {
      auto item = node.at("otherData");
      other_data = std::any_cast<std::map<std::string, std::any>>(item);
    }

    if (points.empty()) {
      // skip point-empty shape
      continue;
    }

    auto* shape = new Shape(label, QColor(), shape_type, {}, group_id);

    for (auto& point : points) {
      shape->addPoint(QPointF(point[0], point[1]));
    }
    shape->close();

    std::map<std::string, bool> default_flags = {};
    if (config_["label_flags"]) {
      for (auto& [pattern, keys] :
           config_["label_flags"]
               .as<std::map<std::string, std::vector<std::string>>>()) {
        QRegularExpression re(QString::fromStdString(pattern));
        if (re.match(label).hasMatch()) {
          for (auto& key : keys) {
            default_flags[key] = false;
          }
        }
      }
    }
    shape->flags_ = default_flags;
    for (auto& [key, value] : flags) {
      shape->flags_[key] = value;
    }
    shape->other_data_ = other_data;

    s.emplaceBack(shape);
  }

  loadShapes(s);
}

void app::loadFlags(const std::map<std::string, bool>& flags) {
  flag_widget_->clear();
  for (auto& [key, flag] : flags) {
    auto* item = new QListWidgetItem(QString::fromStdString(key));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(flag ? Qt::Checked : Qt::Unchecked);
    flag_widget_->addItem(item);
  }
}

bool app::saveLabels(const QString& filename) {
  auto lf = new LabelFile();
  std::vector<std::map<std::string, std::any>> shapes{};
  for (auto i = 0; labelList_->model()->rowCount(); ++i) {
    // TODO: fixed
    LabelListWidgetItem* item =
        (LabelListWidgetItem*)labelList_->model()->item(i);
    auto shape = format_shape(item->shape());
    shapes.emplace_back(shape);
  }

  std::map<std::string, bool> flags{};
  for (auto i = 0; i < flag_widget_->count(); ++i) {
    auto item = flag_widget_->item(i);
    auto key = item->text().toStdString();
    auto flag = item->checkState() == Qt::Checked;
    flags[key] = flag;
  }

  try {
    auto p1 = std::filesystem::path(imagePath_.toStdString());
    auto p2 = std::filesystem::path(filename.toStdString()).parent_path();
    auto p = std::filesystem::relative(p2, p1);
    auto imagePath = QString::fromStdString(p.generic_string());
    auto imageData = config_["store_data"].as<bool>() ? imageData_ : QImage();
    if (!std::filesystem::exists(p)) {
      std::filesystem::create_directories(p);
    }
    // TODO: save
    //        lf->save();

    labelFile_ = lf;

    auto items = fileListWidget_->findItems(imagePath_, Qt::MatchExactly);

    if (items.size() > 0) {
      if (items.size() != 1) {
        LOG(ERROR) << "There are duplcate files.";
      }
      items[0]->setCheckState(Qt::Checked);
    }

    // disable allows next and previous image to proceed
    // self.filename = filename
    return true;

  } catch (std::exception& e) {
    errorMessage(tr("Error saving label data"),
                 QString(tr("<b>%1</b>")).arg(e.what()));
    return false;
  }
}

void app::duplicateSelectedShape() {
  auto added_shapes = canvas_->duplicateSelectedShapes();
  labelList_->clearSelection();
  for (auto& shape : added_shapes) {
    addLabel(shape);
  }
  setDirty();
}

void app::pasteSelectedShape() {
  loadShapes(_copied_shapes, true);
  setDirty();
}

void app::copySelectedShape() {
  _copied_shapes.clear();
  for (auto& s : canvas_->selectedShapes_) {
    _copied_shapes.emplaceBack(s);
  }
  paste_action_->setEnabled(_copied_shapes.size() > 0);
}

void app::labelSelectionChanged() {
  if (_noSelectionSlot) {
    return;
  }
  if (canvas_->editing()) {
    QList<Shape*> selected_shapes{};
    for (auto& item : labelList_->selectedItems()) {
      selected_shapes.emplaceBack(item->shape());
    }
    if (!selected_shapes.empty()) {
      canvas_->selectShapes(selected_shapes);
    } else {
      canvas_->deSelectShape();
    }
  }
}

void app::labelItemChanged(LabelListWidgetItem* item) {
  auto shape = item->shape();
  canvas_->setShapeVisible(shape, item->checkState() == Qt::Checked);
}

void app::labelOrderChanged() {
  setDirty();
  QList<Shape*> shapes{};
  for (auto i = 0; i < labelList_->model()->rowCount(); ++i) {
    // TODO: maybe bug
    auto item = (LabelListWidgetItem*)labelList_->model()->item(i);
    shapes.emplaceBack(item->shape());
  }
  canvas_->loadShapes(shapes);
}

/* Pop-up and give focus to the label editor.
 *
 * position MUST be in global coordinates.
 */
void app::newShape() {
  auto items = uniqLabelList_->selectedItems();
  QString text{};
  if (!items.empty()) {
    text = items[0]->data(Qt::UserRole).toString();
  }
  std::map<std::string, bool> flags{};
  int group_id = -1;
  if (config_["display_label_popup"].as<bool>() || text.isEmpty()) {
    auto previous_text = labelDialog_->edit_->text();
    labelDialog_->popUp(text, flags, group_id, text);
    if (text.isEmpty()) {
      labelDialog_->edit_->setText(previous_text);
    }
  }

  if (!text.isEmpty() && !validateLabel(text)) {
    auto validate_label = config_["validate_label"].as<std::string>();

    errorMessage(tr("Invalid label"),
                 QString(tr("Invalid label %1 with validation type `%2`"))
                     .arg(text)
                     .arg(QString::fromStdString(validate_label)));

    text = "";
  }

  if (!text.isEmpty()) {
    labelList_->clearSelection();
    auto shape = canvas_->setLastLabel(text, flags);
    shape->group_id_ = group_id;
    addLabel(shape);
    edit_mode_action_->setEnabled(true);
    undo_last_point_action_->setEnabled(false);
    undo_action_->setEnabled(true);
    setDirty();
  } else {
    canvas_->undoLastLine();
    canvas_->shapesBackups_.pop_back();
  }
}

void app::scrollRequest(const int delta, const int orientation) {
  auto units = -delta * 0.1;  // natural scroll
  auto bar = scrollBars_[orientation];
  auto value = bar->value() + bar->singleStep() * units;
  setScroll(orientation, value);
}

void app::setScroll(int orientation, double value) {
  scrollBars_[orientation]->setValue(int(value));
  scroll_values_[orientation][filename_] = value;
}

void app::setZoom(int value) {
  fitWidth_action_->setChecked(false);
  fitWindow_action_->setChecked(false);
  zoomMode_ = MANUAL_ZOOM;
  zoomWidget_->setValue(value);
  zoom_value_[filename_] = std::tuple<int, int>(zoomMode_, value);
}

void app::addZoom(double increment) {
  auto zoom_value = zoomWidget_->value() * increment;
  if (increment > 1) {
    zoom_value = std::ceil(zoom_value);
  } else {
    zoom_value = std::floor(zoom_value);
  }
  setZoom(zoom_value);
}

void app::zoomRequest(int delta, const QPointF& pos) {
  auto canvas_width_old = canvas_->width();
  auto units = 1.1;
  if (delta < 0) {
    units = 0.9;
  }
  addZoom(units);

  auto canvas_with_new = canvas_->width();
  if (canvas_width_old != canvas_with_new) {
    auto canvas_scale_factor = canvas_with_new / canvas_width_old;

    auto x_shift = std::round(pos.x() * canvas_scale_factor) - pos.x();
    auto y_shift = std::round(pos.y() * canvas_scale_factor) - pos.y();

    setScroll(Qt::Horizontal, scrollBars_[Qt::Horizontal]->value() + x_shift);
    setScroll(Qt::Vertical, scrollBars_[Qt::Vertical]->value() + y_shift);
  }
}

void app::setFitWindow(bool value) {
  if (value) {
    fitWidth_action_->setChecked(false);
  }
  zoomMode_ = value ? FIT_WINDOW : MANUAL_ZOOM;
  adjustScale();
}

void app::setFitWidth(bool value) {
  if (value) {
    fitWindow_action_->setChecked(true);
  }
  zoomMode_ = value ? FIT_WIDTH : MANUAL_ZOOM;
  adjustScale();
}

void app::enableKeepPrevScale(bool enabled) {
  config_["keep_prev_scale"] = enabled;
  keepPrevScale_action_->setChecked(enabled);
}

void app::onNewBrightnessContrast(const QImage& image) {
  canvas_->loadPixmap(QPixmap::fromImage(image), false);
}

void app::brightnessContrast(bool value) {
  //    auto* dialog = new BrightnessContrastDialog();
}

void app::togglePolygons(bool value) {
  for (auto i = 0; i < labelList_->model()->rowCount(); ++i) {
    auto item = labelList_->model()->item(i);
    item->setCheckState(value ? Qt::Checked : Qt::Unchecked);
  }
}

// Load the specified file, or the last opened file if none.
bool app::loadFile(QString filename) {
  // changing fileListWidget loads file
  auto image_list = imageList();
  if ((std::find(image_list.begin(), image_list.end(), filename) !=
       image_list.end()) &&
      fileListWidget_->currentRow() != image_list.indexOf(filename)) {
    fileListWidget_->setCurrentRow(image_list.indexOf(filename));
    fileListWidget_->repaint();
    return true;
  }
  resetState();
  canvas_->setEnabled(false);

  if (!filename.isEmpty()) {
    filename = settings_->value("filename", "").toString();
  }
  if (!QFile::exists(filename)) {
    errorMessage(tr("Error opening file"),
                 QString(tr("No such file: <b>%1</b>")).arg(filename));
    return false;
  }
  // assumes same name, but json extension.
  auto basename = QFileInfo(filename).baseName();
  this->status(QString(tr("Loading %1 ...")).arg(basename));

  auto label_file = basename + ".json";

  if (!output_dir_.isEmpty()) {
    auto label_file_without_path = QFileInfo(label_file).baseName();
    auto dir = QDir(output_dir_);
    label_file = dir.relativeFilePath(label_file_without_path);
  }
  if (QFile::exists(label_file) and LabelFile::is_label_file(label_file)) {
    try {
      labelFile_ = new LabelFile(label_file);
    } catch (std::exception& e) {
      errorMessage(
          tr("Error opening file"),
          QString(tr("<p><b>%1</b></p>"
                     "<p>Make sure <i>%2</i> is a valid label file.</p>"))
              .arg(e.what())
              .arg(label_file));

      this->status(QString(tr("Error reading %1")).arg(label_file));
      return false;
    }
    //            imageData_ = labelFile_->imageData;
    // TODO: fix it
    imagePath_ =
        QFileInfo(label_file).dir().dirName() + "/" + labelFile_->imagePath;
    //            otherData_ = labelFile_->otherData;
  } else {
    auto image_data = LabelFile::load_image_file(filename);
    mat_to_qimage(image_data, imageData_);
    if (imageData_.isNull()) {
      imagePath_ = filename;
    }
    if (labelFile_ == nullptr) delete labelFile_;
    labelFile_ = nullptr;
  }
  auto image = imageData_;
  if (image.isNull()) {
    QList<QString> formats{};
    auto supported_image_formats = QImageReader::supportedImageFormats();
    for (auto& fmt : supported_image_formats) {
      formats.emplace_back(QString("*.%1").arg(fmt));
    }
    errorMessage(tr("Error opening file"),
                 tr("<p>Make sure <i>%1</i> is a valid image file.<br/>"
                    "Supported image formats: %2</p>")
                     .arg(filename)
                     .arg(formats.join(", ")));
    this->status(tr("Error reading %1").arg(filename));
    return false;
  }
  delete image_;
  image_ = new QImage(image);
  filename_ = filename;
  QList<Shape*> prev_shapes;
  if (config_["keep_prev"]) {
    prev_shapes = canvas_->shapes_;
  }
  canvas_->loadPixmap(QPixmap::fromImage(image));
  std::map<std::string, bool> flags;
  for (auto item : config_["flags"]) {
    auto k = item.as<std::string>();
    flags[k] = false;
  }
  if (labelFile_) {
    loadLabels(labelFile_->shapes);
    if (!labelFile_->flags.empty()) {
      for (auto& [key, value] : labelFile_->flags) {
        flags[key] = value;
      }
    }
  }
  loadFlags(flags);
  if (config_["keep_prev"] && noShapes()) {
    loadShapes(prev_shapes, false);
    setDirty();
  } else {
    setClean();
  }
  canvas_->setEnabled(true);
  // set zoom values
  // TODO: set zoom_values;
  bool is_init_load = zoom_value_.empty();
  if (zoom_value_.find(filename_) != zoom_value_.end()) {
    zoomMode_ = std::get<0>(zoom_value_[filename_]);
    int zoom_value = std::get<1>(zoom_value_[filename_]);
    setZoom(zoom_value);
  } else if (is_init_load || !config_["keep_prev_scale"]) {
    adjustScale(true);
  }
  // set scroll values
  for (auto& [orientation, val_map] : scroll_values_) {
    if (val_map.find(filename_) != val_map.end()) {
      setScroll(orientation, val_map[filename_]);
    }
  }
  // set brightness contrast values
  auto* dialog = new BrightnessContrastDialog(imageData_, this);
  int brightness = -1, contrast = -1;
  if (brightnessContrast_values_.find(filename_) !=
      brightnessContrast_values_.end()) {
    std::tie(brightness, contrast) = brightnessContrast_values_[filename_];
  }
  if (config_["keep_prev_brightness"] && !recentFiles_.empty()) {
    double _;
    if (brightnessContrast_values_.find(recentFiles_[0]) !=
        brightnessContrast_values_.end()) {
      std::tie(brightness, _) = brightnessContrast_values_[recentFiles_[0]];
    }
  }
  if (config_["keep_prev_contrast"] && !recentFiles_.empty()) {
    double _;
    if (brightnessContrast_values_.find(recentFiles_[0]) !=
        brightnessContrast_values_.end()) {
      std::tie(_, contrast) = brightnessContrast_values_[recentFiles_[0]];
    }
  }
  if (brightness >= 0) {
    dialog->slider_brightness_->setValue(brightness);
  }
  if (contrast >= 0) {
    dialog->slider_contrast_->setValue(contrast);
  }
  brightnessContrast_values_[filename_] =
      std::tuple<int, int>(brightness, contrast);
  if (brightness >= 0 || contrast >= 0) {
    dialog->onNewValue(-1);
  }
  paintCanvas();
  addRecentFile(filename_);
  toggleActions(true);
  canvas_->setFocus();
  this->status(tr("Load %1").arg(QFileInfo(filename).baseName()));
  return true;
}

void app::resizeEvent(QResizeEvent* event) {
  if (canvas_ && !image_->isNull() && zoomMode_ != MANUAL_ZOOM) {
    adjustScale();
  }
  QWidget::resizeEvent(event);
}

void app::paintCanvas() {
  assert(!image_->isNull() && "cannot paint null image");
  canvas_->scale_ = 0.01 * zoomWidget_->value();
  canvas_->adjustSize();
  canvas_->update();
}

void app::adjustScale(bool initial) {
  double val = 1.0;
  if (initial) {
    val = scaleFitWindow();
  }
  switch (zoomMode_) {
    case FIT_WINDOW: {
      val = scaleFitWindow();
      break;
    }
    case FIT_WIDTH: {
      val = scaleFitWidth();
      break;
    }
    default: {
      val = 1.0;
      break;
    }
  }
  auto value = int(100 * val);
  zoomWidget_->setValue(value);
  zoom_value_[filename_] = std::tuple<int, int>(zoomMode_, value);
}

// Figure out the size of the pixmap to fit the main widget.
double app::scaleFitWindow() {
  auto e = 2.0;  // So that no scrollbars are generated.
  auto w1 = this->centralWidget()->width() - e;
  auto h1 = this->centralWidget()->height() - e;
  auto a1 = w1 / h1;
  // Calculate a new scale value based on the pixmap's aspect ratio.
  auto w2 = canvas_->pixmap_.width() - 0.0;
  auto h2 = canvas_->pixmap_.height() - 0.0;
  auto a2 = w2 / h2;
  return a2 >= a1 ? w1 / w2 : h1 / h2;
}

double app::scaleFitWidth() {
  // The epsilon does not seem to work too well here.
  auto w = this->centralWidget()->width() - 2.0;
  return w / canvas_->pixmap_.width();
}

void app::enableSaveImageWithData(const bool enabled) {
  config_["store_data"] = enabled;
  save_with_image_data_action_->setChecked(enabled);
}

void app::closeEvent(QCloseEvent* event) {
  if (!mayContinue()) {
    event->ignore();
  }
  settings_->setValue("filename", filename_);
  settings_->setValue("window/size", this->size());
  settings_->setValue("window/position", this->pos());
  settings_->setValue("window/state", saveState());
  settings_->setValue("recentFiles", recentFiles_);
  // ask the use for where to save the labels
  // settings_->setValue("window/geometry", saveGeometry());
  //    QWidget::closeEvent(event);
}
void app::dragEnterEvent(QDragEnterEvent* event) {
  QList<QString> extensions{};
  for (auto& fmt : QImageReader::supportedImageFormats()) {
    extensions.push_back(QString(fmt).toLower());
  }
  if (event->mimeData()->hasUrls()) {
    QList<QString> items{};
    for (auto& i : event->mimeData()->urls()) {
      items.emplace_back(i.toLocalFile());
    }
    for (auto& item : items) {
      auto flag = false;
      for (auto& ext : extensions) {
        if (item.endsWith(ext, Qt::CaseInsensitive)) {
          event->accept();
          flag = true;
          break;
        }
      }
      if (flag) {
        break;
      }
    }
  } else {
    event->ignore();
  }
  //    QWidget::dragEnterEvent(event);
}
void app::dropEvent(QDropEvent* event) {
  if (!mayContinue()) {
    event->ignore();
    return;
  }

  QList<QString> items{};
  for (auto& i : event->mimeData()->urls()) {
    items.emplace_back(i.toLocalFile());
  }

  importDroppedImageFiles(items);

  QWidget::dropEvent(event);
}

void app::loadRecent(const QString& filename) {
  if (mayContinue()) {
    loadFile(filename);
  }
}

void app::openPrevImg(const bool _value) {
  bool keep_prev = false;
  if (config_["keep_prev"]) {
    keep_prev = config_["keep_prev"].as<bool>();
  }
  if (QApplication::keyboardModifiers() ==
      (Qt::ControlModifier | Qt::ShiftModifier)) {
    config_["keep_prev"] = true;
  }

  if (!mayContinue()) {
    return;
  }

  if (imageList().empty()) {
    return;
  }

  if (filename_.isEmpty()) {
    return;
  }

  auto currentIndex = imageList().indexOf(filename_);
  if (currentIndex - 1 >= 0) {
    auto filename = imageList()[currentIndex - 1];
    if (!filename.isEmpty()) {
      loadFile(filename);
    }
  }
  config_["keep_prev"] = keep_prev;
}

void app::openNextImg(const bool _value, const bool load) {
  bool keep_prev = false;
  if (config_["keep_prev"]) {
    keep_prev = config_["keep_prev"].as<bool>();
  }
  if (QApplication::keyboardModifiers() ==
      (Qt::ControlModifier | Qt::ShiftModifier)) {
    config_["keep_prev"] = true;
  }
  if (!mayContinue()) {
    return;
  }
  if (imageList().empty()) {
    return;
  }
  QString filename{};
  if (filename_.isEmpty()) {
    filename = imageList()[0];
  } else {
    auto currentIndex = imageList().indexOf(filename_);
    if (currentIndex + 1 < imageList().size()) {
      filename = imageList()[currentIndex + 1];
    } else {
      filename = *(imageList().end() - 1);
    }
  }
  filename_ = filename;

  if (!filename_.isEmpty() && load) {
    loadFile(filename_);
  }

  config_["keep_prev"] = keep_prev;
}

void app::openFile(const bool _value) {
  if (!mayContinue()) {
    return;
  }
  auto path =
      filename_.isEmpty() ? "./" : QFileInfo(filename_).dir().absolutePath();
  QList<QString> formats{};
  for (auto& fmt : QImageReader::supportedImageFormats()) {
    formats.emplaceBack(QString("*.%1").arg(fmt));
  }
  formats.emplaceBack(LabelFile::suffix);
  QString filters = tr("Image & Label files (%1)").arg(formats.join(" "));

  auto* fileDialog = new FileDialogPreview(this);
  fileDialog->setFileMode(FileDialogPreview::ExistingFile);
  fileDialog->setNameFilter(filters);
  fileDialog->setWindowTitle(
      tr("%1 - Choose Image or Label file").arg(__appname__));
  fileDialog->setWindowFilePath(path);
  fileDialog->setViewMode(FileDialogPreview::Detail);
  if (fileDialog->exec()) {
    auto filename = fileDialog->selectedFiles()[0];
    if (!filename.isEmpty()) {
      loadFile(filename);
    }
  }
}

void app::changeOutputDirDialog(const bool _value) {
  auto default_output_dir = output_dir_;
  if (default_output_dir.isEmpty() && !filename_.isEmpty()) {
    default_output_dir = QFileInfo(filename_).dir().absolutePath();
  }
  if (default_output_dir.isEmpty()) {
    default_output_dir = currentPath();
  }

  auto output_dir = QFileDialog::getExistingDirectory(
      this, tr("%1 - Save/Load Annotations in Directory").arg(__appname__),
      default_output_dir,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (output_dir.isEmpty()) {
    return;
  }

  output_dir_ = output_dir;

  this->statusBar()->showMessage(tr("%1 Annotations will be saved/loaded in %2")
                                     .arg("Change Annotations Dir")
                                     .arg(output_dir_));
  this->statusBar()->show();

  auto current_filename = filename_;
  importDirImages(lastOpenDir_, {}, false);

  auto image_list = imageList();
  if (std::find(image_list.begin(), image_list.end(), current_filename) !=
      image_list.end()) {
    // retain currently selected file
    fileListWidget_->setCurrentRow(image_list.indexOf(current_filename));
  }
  fileListWidget_->repaint();
}

void app::onSaveAutoTriggered(bool value) {
  save_auto_action_->setChecked(value);
}

void app::saveFile(const bool _value) {
  assert(!image_->isNull() && "cannot save empty image");
  if (labelFile_ != nullptr) {
    // overwrite when in directory
    _saveFile(labelFile_->filename);
  } else if (output_file_ != nullptr) {
    _saveFile(output_file_);
    this->close();
  } else {
    _saveFile(saveFileDialog());
  }
}

void app::saveFileAs(bool _value) {
  assert(!image_->isNull() && "cannot save empty image");
  _saveFile(saveFileDialog());
}

QString app::saveFileDialog() {
  auto caption = tr("%1 - Choose File").arg(__appname__);
  auto filters = tr("Label files (*%1)").arg(LabelFile::suffix);
  QFileDialog* dlg = nullptr;
  if (!output_dir_.isEmpty()) {
    dlg = new QFileDialog(this, caption, output_dir_, filters);
  } else {
    dlg = new QFileDialog(this, caption, currentPath(), filters);
  }
  dlg->setDefaultSuffix(QString(LabelFile::suffix).sliced(1));
  dlg->setAcceptMode(QFileDialog::AcceptSave);
  dlg->setOption(QFileDialog::DontConfirmOverwrite, false);
  dlg->setOption(QFileDialog::DontUseNativeDialog, false);
  auto basename = QFileInfo(filename_).baseName();
  QString default_labelfile_name{};
  if (!output_dir_.isEmpty()) {
    default_labelfile_name =
        QDir(output_dir_).absoluteFilePath(basename + LabelFile::suffix);
  } else {
    default_labelfile_name =
        QDir(currentPath()).absoluteFilePath(basename + LabelFile::suffix);
  }
  auto filename =
      dlg->getSaveFileName(this, caption, default_labelfile_name, filters);

  return filename;
}

void app::_saveFile(const QString& filename) {
  if (!filename.isEmpty() && saveLabels(filename)) {
    addRecentFile(filename);
    setClean();
  }
}

void app::closeFile(const bool _value) {
  if (!mayContinue()) {
    return;
  }
  resetState();
  setClean();
  toggleActions(false);
  canvas_->setEnabled(false);
  save_as_action_->setEnabled(false);
}

QString app::getLabelFile() {
  QString label_file{};
  if (filename_.endsWith(".json", Qt::CaseInsensitive)) {
    label_file = filename_;
  } else {
    label_file = QFileInfo(filename_).baseName() + ".json";
  }

  return label_file;
}

void app::deleteFile() {
  auto* mb = new QMessageBox();
  auto msg =
      tr("You are about to permanently delete this label file, "
         "proceed anyway?");
  auto answer = mb->warning(this, tr("Attention"), msg,
                            QMessageBox::Yes | QMessageBox::No);
  if (answer != QMessageBox::Yes) {
    return;
  }

  auto label_file = getLabelFile();
  if (QFile::exists(label_file)) {
    QFile::remove(label_file);
    LOG(INFO) << "Label file is removed: " << label_file.toStdString();

    auto item = fileListWidget_->currentItem();
    item->setCheckState(Qt::Unchecked);

    resetState();
  }
}

bool app::hasLabels() {
  if (noShapes()) {
    errorMessage(tr("No objects labeled"),
                 tr("You must label at least one object to save the file."));
    return false;
  }
  return true;
}

bool app::hasLabelFile() {
  if (filename_.isEmpty()) {
    return false;
  }

  auto label_file = getLabelFile();
  return QFile::exists(label_file);
}

bool app::mayContinue() {
  if (!dirty_) {
    return true;
  }

  auto* mb = new QMessageBox();
  auto msg = tr("Save annotations to %1 before closing?").arg(filename_);
  auto answer = mb->question(
      this, tr("Save annotations?"), msg,
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
      QMessageBox::Save);
  if (answer == QMessageBox::Discard) {
    return true;
  } else if (answer == QMessageBox::Save) {
    saveFile();
    return true;
  } else {
    return false;
  }
}

void app::errorMessage(const QString& title, const QString& message) {
  auto mb = QMessageBox::critical(
      this, title, QString("<p><b>%1</b></p>%2").arg(title).arg(message));
}

QString app::currentPath() {
  if (!filename_.isEmpty()) {
    return QFileInfo(filename_).dir().absolutePath();
  }
  return ".";
}

void app::toggleKeepPrevMode() {
  config_["keep_prev"] = !config_["keep_prev"].as<bool>();
}

void app::removeSelectedPoint() {
  canvas_->removeSelectedPoint();
  canvas_->update();
  if (!canvas_->hShape_->points.empty()) {
    canvas_->deleteShape(canvas_->hShape_);
    remLabels({canvas_->hShape_});
    if (noShapes()) {
      for (auto& action : onShapesPresent_actions_) {
        action->setEnabled(false);
      }
    }
  }
  setDirty();
}

void app::deleteSelectedShape() {
  auto yes = QMessageBox::Yes;
  auto no = QMessageBox::No;
  // TODO: add message
  auto msg = tr("");
  //    auto msg = tr("You are about to permanently delete %1 polygons, "
  //                  "proceed anyway?").arg(canvas_->selectedShapes_);
  if (yes == QMessageBox::warning(this, tr("Attention"), msg, yes | no, yes)) {
    remLabels(canvas_->deleteSelected());
    setDirty();
    if (noShapes()) {
      for (auto& action : onShapesPresent_actions_) {
        action->setEnabled(false);
      }
    }
  }
}

void app::copyShape() {
  canvas_->endMove(true);
  for (auto& shape : canvas_->selectedShapes_) {
    addLabel(shape);
  }
  labelList_->clearSelection();
  setDirty();
}

void app::moveShape() {
  canvas_->endMove(false);
  setDirty();
}

void app::openDirDialog(bool _value, const QString& dirpath) {
  if (!mayContinue()) {
    return;
  }

  auto defaultOpenDirPath = dirpath.isEmpty() ? "." : dirpath;
  if (!lastOpenDir_.isEmpty() && QFile::exists(lastOpenDir_)) {
    defaultOpenDirPath = lastOpenDir_;
  } else {
    defaultOpenDirPath =
        filename_.isEmpty() ? "." : QFileInfo(filename_).dir().absolutePath();
  }

  auto targetDirPath = QFileDialog::getExistingDirectory(
      this, tr("%1 - Open Directory").arg(__appname__), defaultOpenDirPath,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  importDirImages(targetDirPath);
}

QList<QString> app::imageList() {
  QList<QString> list{};
  for (auto i = 0; i < fileListWidget_->count(); ++i) {
    auto item = fileListWidget_->item(i);
    list.emplaceBack(item->text());
  }
  return list;
}

void app::importDroppedImageFiles(const QList<QString>& imageFiles) {
  QList<QString> extensions{};
  for (auto& fmt : QImageReader::supportedImageFormats()) {
    extensions.emplaceBack(QString(".%1").arg(fmt));
  }
  filename_ = {};
  auto image_list = imageList();
  for (auto& file : imageFiles) {
    if (std::find(image_list.begin(), image_list.end(), file) !=
        image_list.end()) {
      continue;
    }

    bool flag = true;
    for (auto& ext : extensions) {
      if (file.endsWith(ext, Qt::CaseInsensitive)) {
        flag = false;
        break;
      }
    }
    if (flag) {
      continue;
    }

    auto label_file = QFileInfo(file).baseName() + ".json";
    if (!output_dir_.isEmpty()) {
      auto label_file_without_path = QFileInfo(label_file).baseName();
      label_file = QDir(output_dir_).absoluteFilePath(label_file_without_path);
    }
    auto* item = new QListWidgetItem(file);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if (QFile::exists(label_file) && LabelFile::is_label_file(label_file)) {
      item->setCheckState(Qt::Checked);
    } else {
      item->setCheckState(Qt::Unchecked);
    }
    fileListWidget_->addItem(item);
  }

  if (imageList().size() > 1) {
    open_next_img_action_->setEnabled(true);
    open_prev_img_action_->setEnabled(true);
  }

  openNextImg();
}

void app::importDirImages(const QString& dirpath, const QString& pattern,
                          const bool load) {
  open_next_img_action_->setEnabled(true);
  open_prev_img_action_->setEnabled(true);

  if (!mayContinue() || dirpath.isEmpty()) {
    return;
  }

  lastOpenDir_ = dirpath;
  filename_ = {};
  fileListWidget_->clear();
  for (auto& filename : scanAllImages(dirpath)) {
    if (!pattern.isEmpty() && filename.indexOf(pattern) >= 0) {
      continue;
    }

    auto label_file = QFileInfo(filename).baseName() + ".json";
    if (!output_dir_.isEmpty()) {
      auto label_file_without_path = QFileInfo(label_file).baseName() + ".json";
      label_file = QDir(output_dir_).absoluteFilePath(label_file_without_path);
    }
    auto* item = new QListWidgetItem(filename);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if (QFile::exists(label_file) && LabelFile::is_label_file(label_file)) {
      item->setCheckState(Qt::Checked);
    } else {
      item->setCheckState(Qt::Unchecked);
    }
    fileListWidget_->addItem(item);
  }
  openNextImg(false, load);
}

QList<QString> app::scanAllImages(const QString& folderPath) {
  QList<QString> extensions{};
  for (auto& fmt : QImageReader::supportedImageFormats()) {
    extensions.emplaceBack(QString(".%1").arg(fmt));
  }

  QList<QString> images{};

  QDir dir(folderPath);
  dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
  dir.setSorting(QDir::NoSort);

  auto entryList = dir.entryList();

  QCollator collator;
  collator.setNumericMode(true);
  std::sort(entryList.begin(), entryList.end(), collator);

  for (auto& file : entryList) {
    for (auto& ext : extensions) {
      if (file.endsWith(ext, Qt::CaseInsensitive)) {
        images.emplaceBack(file);
        break;
      }
    }
  }

  return images;
}
void app::onCreateModeTriggered() { toggleDrawMode(false, "polygon"); }
void app::onCreateRectangleModeTriggered() {
  toggleDrawMode(false, "rectangle");
}
void app::onCreateCircleModeTriggered() { toggleDrawMode(false, "circle"); }
void app::onCreateLineModeTriggered() { toggleDrawMode(false, "line"); }
void app::onCreatePointModeTriggered() { toggleDrawMode(false, "point"); }
void app::onCreateLinestripModeTriggered() {
  toggleDrawMode(false, "linestrip");
}
void app::onHideAllTriggered() { togglePolygons(false); }
void app::onShowAllTriggered() { togglePolygons(true); }
void app::onZoomInTriggered() { addZoom(1.1); }
void app::onZoomOutTriggered() { addZoom(0.9); }
void app::onZoomOrgTriggered() { setZoom(100); }
