#ifndef SHAPE_H
#define SHAPE_H

#include <QAction>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <any>

namespace lmp {

inline QColor DEFAULT_LINE_COLOR = QColor(0, 255, 0, 128);        // bf hovering
inline QColor DEFAULT_FILL_COLOR = QColor(0, 255, 0, 128);        // hovering
inline QColor DEFAULT_SELECT_LINE_COLOR = QColor(255, 255, 255);  // selected
inline QColor DEFAULT_SELECT_FILL_COLOR = QColor(0, 255, 0, 155);  // selected
inline QColor DEFAULT_VERTEX_FILL_COLOR = QColor(0, 255, 0, 255);  // hovering
inline QColor DEFAULT_HVERTEX_FILL_COLOR =
    QColor(255, 255, 255, 255);  // hovering

class Shape {
 public:
  // Render handles as squares
  char P_SQUARE = 0;

  // Render handles as circles
  char P_ROUND = 1;

  // Flag for the handles we would move if dragging
  inline static const char MOVE_VERTEX = 0;

  // Flag for all other handles on the current shape
  inline static const char NEAR_VERTEX = 1;

  // The following class variables influence the drawing of all shape objects.
  static inline QColor line_color = DEFAULT_LINE_COLOR;
  static inline QColor fill_color = DEFAULT_FILL_COLOR;
  static inline QColor select_line_color = DEFAULT_SELECT_LINE_COLOR;
  static inline QColor select_fill_color = DEFAULT_SELECT_FILL_COLOR;
  static inline QColor vertex_fill_color = DEFAULT_VERTEX_FILL_COLOR;
  static inline QColor hvertex_fill_color = DEFAULT_HVERTEX_FILL_COLOR;

  char point_type = P_ROUND;
  static inline int point_size = 8;
  double scale = 1.0;

 public:
  Shape();
  Shape(const QString& label, const QColor& line_color,
        const QString& shape_type, const std::map<std::string, bool>& flags,
        int group_id);
  QPointF operator[](int i) const;
  QPointF& operator[](int i);
  size_t size();
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
  void highlightClear();
  void highlightVertex(int i, char action);
  void moveBy(const QPointF& offset);
  void moveVertexBy(int i, const QPointF& offset);
  int nearestVertex(const QPointF& point, double epsilon);
  int nearestEdge(const QPointF& point, double epsilon);
  bool containsPoint(const QPointF& point);
  QRectF boundingRect();

  void paint(QPainter* painter);

 public:
  std::vector<QPointF> points;
  QString label_;
  QColor line_color_;
  int group_id_;
  bool fill_;
  bool selected;
  QString shape_type_;
  std::map<std::string, bool> flags_;

  std::map<std::string, std::any> other_data_;

 private:
  int _highlightIndex;
  char _highlightMode;
  std::map<char, std::pair<double, char>> _highlightSettings;
  bool _closed;
  QColor _vertex_fill_color;

  void drawVertex(QPainterPath path, int i);
  QPainterPath makePath();
};

};  // namespace lmp
Q_DECLARE_METATYPE(lmp::Shape);
#endif
