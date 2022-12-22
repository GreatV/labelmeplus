#ifndef SHAPE_H
#define SHAPE_H

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>


QColor DEFAULT_LINE_COLOR=QColor(0, 255, 0, 128);  // bf hovering
QColor DEFAULT_FILL_COLOR=QColor(0, 255, 0, 128);  // hovering
QColor DEFAULT_SELECT_LINE_COLOR=QColor(255, 255, 255);  // selected
QColor DEFAULT_SELECT_FILL_COLOR=QColor(0, 255, 0, 155);  // selected
QColor DEFAULT_VERTEX_FILL_COLOR=QColor(0, 255, 0, 255);  // hovering
QColor DEFAULT_HVERTEX_FILL_COLOR=QColor(255, 255, 255, 255);  // hovering


class Shape
{
  // Render handles as squares
  char P_SQUARE = 0;

  // Render handles as circles
  char P_ROUND = 1;

  // Flag for the handles we would move if dragging
  char MOVE_VERTEX = 0;

  // Flag for all other handles on the current shape
  char NEAR_VERTEX = 1;

  // The following class variables influence the drawing of all shape objects.
  QColor line_color = DEFAULT_LINE_COLOR;
  QColor fill_color = DEFAULT_FILL_COLOR;
  QColor select_line_color = DEFAULT_SELECT_LINE_COLOR;
  QColor select_fill_color = DEFAULT_SELECT_FILL_COLOR;
  QColor vertex_fill_color = DEFAULT_VERTEX_FILL_COLOR;
  QColor hvertex_fill_color = DEFAULT_HVERTEX_FILL_COLOR;
  char point_type = P_ROUND;
  int point_size = 8;
  double scale = 1.0;

 public:
  Shape(const QString& label, const QColor& line_color, const QString& shape_type, const QString& flags, int group_id);
  QString shape_type();
  void shape_type(QString value = "polygon");
  void close();
  void addPoint(const QPointF& point);
  bool canAddPoint();
  QPointF popPoint();
  void insertPoint(int i, const QPointF& point);
  void removePoint(int i);
  bool isClosed();
  void setOpen();
  QRectF getRectFromLine(const QPointF& pt1, const QPointF& pt2);
  QRectF getCircleRectFromLine(const std::vector<QPointF>& line);

  void paint(QPainter painter);

 public:
  QString label_;
  QColor line_color_;
  int group_id_;
  std::vector<QPointF> points_;
  bool fill_;
  bool selected_;
  QString flags_;
  QString shape_type_;
  QString other_data_;

 private:
  int _highlightIndex;
  char _highlightMode;
  std::map<char, std::pair<double, char>> _highlightSettings;
  bool _closed;
  QString _shape_type;
  QColor _vertex_fill_color;

  void drawVertex(QPainterPath path, int i);
  int nearestVertex(const QPointF& point, double epsilon);
};

#endif
