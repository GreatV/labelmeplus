#ifndef UTILS_H
#define UTILS_H
#include <QAction>
#include <QRectF>
#include <QRegularExpressionValidator>
#include <QToolBar>
#include <opencv2/opencv.hpp>
#include <string>

/// @brief shape type
enum marked_shape_type {
  shape_circle = 0,
  shape_rectangle,
  shape_line,
  shape_linestrip,
  shape_point,
  shape_polygon
};

void lblsave(const std::string& filename, const cv::Mat& lbl);

double distance(const QPointF& p);

double distance_to_line(const QPointF& point, const std::vector<QPointF>& line);

cv::Mat apply_exif_orientation(const std::string& image_filepath,
                               const cv::Mat& raw_image);

void addActions(QMenu* widget, const QList<QAction*>& actions);
void addActions(QMenu* widget, const QList<QMenu*>& actions);
void addActions(QToolBar* widget, const QList<QAction*>& actions);

QIcon newIcon(const QString& icon);
QAction* newAction(QObject* parent, QString text,
                   const QList<QKeySequence>& shortcut = {},
                   const QString& icon = "", const QString& tip = "",
                   bool checkable = false, bool enable = true,
                   bool checked = false);

QRegularExpressionValidator* labelValidator();
QString fmtShortcut(const QString& text);
#endif
