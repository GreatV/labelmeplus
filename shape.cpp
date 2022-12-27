#include "shape.h"

#include <fmt/format.h>

#include "utils/utils.h"

std::vector<QString> valid_shape{
    "polygon", "rectangle", "point", "line", "circle", "linestrip",
};

static std::map<QString, int> shape_type_map{
    {"polygon", shape_polygon}, {"rectangle", shape_rectangle},
    {"point", shape_point},     {"line", shape_line},
    {"circle", shape_circle},   {"linestrip", shape_linestrip}};

std::vector<QString> multi_points_shape{"polygon", "linestrip"};

Shape::Shape(const QString& label, const QColor& line_color,
             const QString& shape_type, const QString& flags, int group_id) {
  label_ = label;
  group_id_ = group_id;
  fill_ = false;
  selected_ = false;
  shape_type_ = shape_type;
  flags_ = flags;

  _highlightIndex = -1;
  _highlightMode = NEAR_VERTEX;

  _highlightSettings.insert(
      std::make_pair(NEAR_VERTEX, std::make_pair(4.0, P_ROUND)));
  _highlightSettings.insert(
      std::make_pair(MOVE_VERTEX, std::make_pair(1.5, P_SQUARE)));

  _closed = false;
  line_color_ = line_color;
  shape_type_ = shape_type;
}
QString Shape::shape_type() { return shape_type_; }
void Shape::shape_type(const QString value) {
  if (std::find(valid_shape.begin(), valid_shape.end(), value) !=
      valid_shape.end()) {
    throw fmt::format("Unexpected shape_type: {1}", value.toStdString());
  }
  _shape_type = value;
}

void Shape::close() { _closed = true; }

void Shape::addPoint(const QPointF& point) {
  if (!points_.empty() && point == points_[0]) {
    _closed = true;
  } else {
    points_.emplace_back(point);
  }
}

bool Shape::canAddPoint() {
  return find(multi_points_shape.begin(), multi_points_shape.end(),
              shape_type_) != multi_points_shape.end();
}

QPointF Shape::popPoint() {
  QPointF point;
  if (!points_.empty()) {
    point = *(points_.end() - 1);
    points_.pop_back();
    return point;
  }
  return point;
}

void Shape::insertPoint(int i, const QPointF& point) {
  auto iter = points_.begin();
  points_.insert(iter + i, point);
}
void Shape::removePoint(int i) {
  auto iter = points_.begin();
  points_.erase(iter + i);
}
bool Shape::isClosed() { return _closed; }
void Shape::setOpen() { _closed = false; }
QRectF Shape::getRectFromLine(const QPointF& pt1, const QPointF& pt2) {
  auto x1 = pt1.x();
  auto y1 = pt1.y();
  auto x2 = pt2.x();
  auto y2 = pt2.y();
  return QRectF(x1, y1, x2 - x1, y2 - y1);
}

void Shape::paint(QPainter painter) {
  QColor color;
  if (!points_.empty()) {
    color = selected_ ? select_line_color : line_color_;
  }
  QPen pen = QPen(color);
  // Try using integer sizes for smoother drawing(?)
  pen.setWidth(std::max(1, int(round(2.0 / scale))));
  painter.setPen(pen);

  auto line_path = QPainterPath();
  auto vrtx_path = QPainterPath();

  switch (shape_type_map[shape_type_]) {
    case shape_rectangle: {
      //      assert(len(self.points) in [1, 2])
      if (points_.size() == 2) {
        auto rectangle = getRectFromLine(points_[0], points_[1]);
        line_path.addRect(rectangle);
      }

      for (auto i = 0; i < points_.size(); ++i) {
        drawVertex(vrtx_path, i);
      }
      break;
    }
    case shape_circle: {
      //      assert()
      if (points_.size() == 2) {
        auto rectangle = getCircleRectFromLine(points_);
        line_path.addEllipse(rectangle);
      }

      for (auto i = 0; i < points_.size(); ++i) {
        drawVertex(vrtx_path, i);
      }
      break;
    }
    case shape_linestrip: {
      line_path.moveTo(points_[0]);

      for (auto i = 0; i < points_.size(); ++i) {
        auto p = points_[i];
        line_path.lineTo(p);
        drawVertex(vrtx_path, i);
      }
      break;
    }
    default: {
      line_path.moveTo(points_[0]);
      // Uncommenting the following line will draw 2 paths
      // for the 1st vertex, and make it non-filled, which
      // may be desirable.
      // drawVertex(vrtx_path, 0);

      for (auto i = 0; i < points_.size(); ++i) {
        auto p = points_[i];
        line_path.lineTo(p);
        drawVertex(vrtx_path, i);
      }

      if (isClosed()) {
        line_path.lineTo(points_[0]);
      }

      break;
    }
  }

  painter.drawPath(line_path);
  painter.drawPath(vrtx_path);
  painter.fillPath(vrtx_path, _vertex_fill_color);

  if (fill_) {
    color = selected_ ? select_fill_color : fill_color;
    painter.fillPath(line_path, color);
  }
}

void Shape::drawVertex(QPainterPath path, int i) {
  auto d = point_size / scale;
  auto shape = point_type;
  auto point = points_[i];

  if (i == _highlightIndex) {
    double size;
    std::tie(size, shape) = _highlightSettings[_highlightMode];
    d *= size;
  }

  if (_highlightIndex >= 0) {
    _vertex_fill_color = hvertex_fill_color;
  } else {
    _vertex_fill_color = vertex_fill_color;
  }

  if (shape == P_SQUARE) {
    path.addRect(point.x() - d / 2, point.y() - d / 2, d, d);
  } else if (shape == P_ROUND) {
    path.addEllipse(QPointF(point.x(), point.y()), d / 2.0, d / 2.0);
  } else {
    assert((false && "unsupported vertex shape"));
  }
}

// Computes parameters to draw with `QPainterPath::addEllipse`
QRectF Shape::getCircleRectFromLine(const std::vector<QPointF>& line) {
  QRectF rectangle;
  if (line.size() != 2) {
    return rectangle;
  }
  auto c = line[0];
  auto point = line[1];
  auto r = line[0] - line[1];
  auto d = sqrt(pow(r.x(), 2) + pow(r.y(), 2));
  rectangle = QRectF(c.x() - d, c.y() - d, 2 * d, 2 * d);
  return rectangle;
}

int Shape::nearestVertex(const QPointF& point, const double epsilon) {
  double min_distance = DBL_MAX;
  int min_i = -1;
  for (auto i = 0; i < points_.size(); ++i) {
    auto p = points_[i];
    auto dist = distance(p - point);
    if (dist < epsilon && dist < min_distance) {
      min_distance = dist;
      min_i = i;
    }
  }
  return min_i;
}

int Shape::nearestEdge(const QPointF& point, double epsilon) {
  double min_distance = DBL_MAX;
  int post_i = -1;
  for (auto i = 0; i < points_.size(); ++i) {
    std::vector<QPointF> line{points_[i - 1], points_[i]};
    auto dist = distance_to_line(point, line);
    if (dist <= epsilon && dist < min_distance) {
      min_distance = dist;
      post_i = i;
    }
  }
  return post_i;
}
QPainterPath Shape::makePath() {
  QPainterPath path;
  if (shape_type_ == "rectangle") {
    path = QPainterPath();
    if (points_.size() == 2) {
      auto rectangle = getRectFromLine(points_[0], points_[1]);
      path.addRect(rectangle);
    }
  } else if (shape_type_ == "circle") {
    path = QPainterPath();

    if (points_.size() == 2) {
      auto rectangle = getCircleRectFromLine(points_);
      path.addEllipse(rectangle);
    }
  } else {
    path = QPainterPath(points_[0]);
    for (auto& p : points_) {
      path.lineTo(p);
    }
  }

  return path;
}
bool Shape::containsPoint(const QPointF& point) {
  return makePath().contains(point);
}
QRectF Shape::boundingRect() { return makePath().boundingRect(); }
void Shape::moveBy(const QPointF& offset) {
  for (auto& p : points_) {
    p += offset;
  }
}

void Shape::moveVertexBy(const int i, const QPointF& offset) {
  points_[i] += offset;
}
void Shape::highlightVertex(const int i, const char action) {
  _highlightIndex = i;
  _highlightMode = action;
}
void Shape::highlightClear() { _highlightIndex = -1; }
