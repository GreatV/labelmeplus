#include "canvas.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include <QApplication>
#include <algorithm>
#include <utility>

#include "utils/utils.h"

#define MOVE_SPEED 5.0

std::vector<QString> createModeMultiPoints{
    "polygon",
    "linestrip",
};

std::vector<QString> createMode2Points{"rectangle", "circle", "line"};

Canvas::Canvas(const double epsilon, QString double_click,
               const int num_backups, std::map<std::string, bool> crosshair,
               QWidget *parent)
    : epsilon_(epsilon),
      double_click_(std::move(double_click)),
      num_backups_(num_backups),
      crosshair_(std::move(crosshair)),
      QWidget(parent) {
  if (!(double_click_.isEmpty() || double_click_ == "close")) {
    LOG(ERROR) << fmt::format("Unexpected value for double_click event: {1}",
                              double_click_.toStdString());
  }

  mode_ = EDIT;
  shapes_.clear();
  shapesBackups_.clear();
  current_ = nullptr;
  selectedShapes_.clear();
  selectedShapesCopy_.clear();

  /* line_ represents:
   *     - createMode == "polygon": edge from last point to current
   *     - createMode == "rectangle": diagonal line of the rectangle
   *     - createMode == "line": the line
   *     - createMode == "point": the point
   */
  line_ = new Shape();

  menus[0] = new QMenu();
  menus[1] = new QMenu();

  // set widget options
  this->setMouseTracking(true);
  this->setFocusPolicy(Qt::WheelFocus);
}
bool Canvas::fillDrawing() { return fill_drawing_; }
void Canvas::setFillDrawing(bool value) { fill_drawing_ = value; }
QString Canvas::createMode() { return createMode_; }
void Canvas::createMode(const QString &value) {
  std::vector<QString> mode_list{
      "polygon", "rectangle", "circle", "point", "linestrip",
  };
  if (std::find(mode_list.begin(), mode_list.end(), value) == mode_list.end()) {
    LOG(ERROR) << fmt::format("Unsupported createMode: {0}",
                              value.toStdString());
  }
  createMode_ = value;
}
void Canvas::storeShapes() {
  QList<Shape *> shapesBackup;
  for (auto &shape : shapes_) {
    shapesBackup.emplace_back(shape);
  }

  std::vector<QList<Shape *>> shapesBackups;
  if (shapesBackups_.size() > num_backups_) {
    // self.shapesBackups = self.shapesBackups[-self.num_backups - 1: ]
    std::move(shapesBackups_.end() - num_backups_, shapesBackups_.end(),
              std::back_inserter(shapesBackups));
  }
  shapesBackups.emplace_back(shapesBackup);

  shapesBackups_ = shapesBackups;
}

bool Canvas::isShapeRestorable() {
  /*
   * We save the state AFTER each edit (not before) so for an
   * edit to be undoable, we expect the CURRENT and the PREVIOUS state
   * to be in the undo stack.
   */
  if (shapesBackups_.size() < 2) {
    return false;
  }
  return true;
}
void Canvas::restoreShape() {
  /*
   * This does __part__ of the job of restoring shapes.
   * The complete process is also done in app.py::undoShapeEdit
   * and app.py::loadShapes and our own Canvas::loadShapes function.
   */
  if (!isShapeRestorable()) {
    return;
  }
  shapesBackups_.pop_back();  // pop latest

  // The application will eventually call Canvas.loadShapes which will
  // push this right back onto the stack.
  auto shapesBackup = *(shapesBackups_.end() - 1);
  shapesBackups_.pop_back();
  shapes_ = shapesBackup;
  selectedShapes_.clear();
  selectedShapes_.shrink_to_fit();
  for (auto &shape : shapes_) {
    shape->selected = false;
  }
  this->update();
}

void Canvas::enterEvent(QEnterEvent *event) { overrideCursor(cursor_); }

void Canvas::leaveEvent(QEvent *event) {
  unHighlight();
  restoreCursor();
}

void Canvas::focusOutEvent(QFocusEvent *event) { restoreCursor(); }

void Canvas::overrideCursor(const QCursor &cursor) {
  restoreCursor();
  cursor_ = cursor;
  QApplication::setOverrideCursor(cursor);
}
void Canvas::restoreCursor() { QApplication::restoreOverrideCursor(); }
void Canvas::unHighlight() {
  if (hShape_ != nullptr) {
    hShape_->highlightClear();
    this->update();
  }

  prevShape_ = hShape_;
  prevVertex_ = hVertex_;
  prevEdge_ = hEdge_;

  hShape_ = nullptr;
  hVertex_ = hEdge_ = -1;
}

bool Canvas::isVisible(Shape *shape) {
  if (visible_.count(shape)) {
    return visible_[shape];
  }
  return true;
}
bool Canvas::drawing() { return mode_ == CREATE; }
bool Canvas::editing() { return mode_ == EDIT; }
void Canvas::setEditing(const bool value) {
  mode_ = value ? EDIT : CREATE;
  if (mode_ == EDIT) {
    // CREATE -> EDIT
    this->repaint();  // clear crosshair
  } else {
    // EDIT -> CREATE
    unHighlight();
    deSelectShape();
  }
}
void Canvas::deSelectShape() {
  if (!selectedShapes_.empty()) {
    setHiding(false);
    QList<Shape *> selectedShapes{};
    emit selectionChanged(selectedShapes);
    hShapeIsSelected_ = false;
    this->update();
  }
}
void Canvas::setHiding(const bool enable) {
  // _hideBackground = enable ? hideBackground_ : false;
  _hideBackground = enable && hideBackground_;
}
bool Canvas::selectedVertex() { return hVertex_ >= 0; }
bool Canvas::selectedEdge() { return hEdge_ >= 0; }

// Update line with last point and current coordinates.
void Canvas::mouseMoveEvent(QMouseEvent *event) {
  auto pos = transformPos(event->position());

  prevMovePoint_ = pos;
  restoreCursor();

  // Polygon drawing.
  if (drawing()) {
    line_->shape_type(createMode());

    overrideCursor(CURSOR_DRAW);
    if (current_ == nullptr) {
      repaint();  // draw crosshair
      return;
    }

    if (outOfPixmap(pos)) {
      /*
       * Don't allow the user to draw outside the pixmap.
       * Project the point to the pixmap's edges.
       */
      pos = intersectionPoint((*current_)[-1], pos);
    } else if (snapping_ && current_->size() > 1 && createMode() == "polygon" &&
               closeEnough(pos, (*current_)[0])) {
      /*
       * Attract line to starting point and
       * color rise to alert the user.
       */
      pos = (*current_)[0];
      overrideCursor(CURSOR_POINT);
      current_->highlightVertex(0, Shape::NEAR_VERTEX);
    }

    if (createMode() == "polygon" || createMode() == "linestrip") {
      (*line_)[0] = (*current_)[-1];
      (*line_)[1] = pos;
    }

    if (createMode() == "rectangle") {
      line_->points = {(*current_)[0], pos};
      line_->close();
    }

    if (createMode() == "circle") {
      line_->points = {(*current_)[0], pos};
      line_->shape_type("circle");
    }

    if (createMode() == "line") {
      line_->points = {(*current_)[0], pos};
      line_->close();
    }

    if (createMode() == "point") {
      line_->points = {(*current_)[0]};
      line_->close();
    }

    this->repaint();
    current_->highlightClear();
    return;
  }

  // Polygon copy moving.
  if (Qt::RightButton & event->buttons()) {
    if (!selectedShapesCopy_.empty() && prevPoint_.isNull()) {
      overrideCursor(CURSOR_MOVE);
      boundedMoveShapes(selectedShapesCopy_, pos);
      this->repaint();
    } else if (!selectedShapes_.empty()) {
      selectedShapesCopy_.clear();
      selectedShapesCopy_.shrink_to_fit();
      std::move(selectedShapes_.begin(), selectedShapes_.end(),
                std::back_inserter(selectedShapesCopy_));
      this->repaint();
    }
    return;
  }

  // Polygon/vertex moving.
  if (Qt::LeftButton & event->button()) {
    if (selectedVertex()) {
      boundedMoveVertex(pos);
      this->repaint();
      movingShape_ = true;
    } else if (!selectedShapes_.empty() && !prevPoint_.isNull()) {
      overrideCursor(CURSOR_MOVE);
      boundedMoveShapes(selectedShapes_, pos);
      this->repaint();
      movingShape_ = true;
    }
    return;
  }

  /*
   * Just hovering over the canvas, 2 possibilities:
   * - Highlight shapes
   * - Highlight vertex
   * Update shape/vertex fill and tooltip value accordingly.
   */
  this->setToolTip(tr("Image"));
  std::vector<Shape *> reversed_shapes;
  for (auto &shape : shapes_) {
    if (isVisible(shape)) {
      reversed_shapes.emplace_back(shape);
    }
  }
  std::reverse(reversed_shapes.begin(), reversed_shapes.end());

  for (auto &shape : reversed_shapes) {
    // Look for a nearby vertex to highlight, If that fails,
    // check if we happen to be inside a shape.
    auto index = shape->nearestVertex(pos, epsilon_ / scale_);
    auto index_edge = shape->nearestEdge(pos, epsilon_ / scale_);
    if (index >= 0) {
      if (selectedVertex()) {
        hShape_->highlightClear();
      }
      prevVertex_ = hVertex_ = index;
      prevShape_ = hShape_ = shape;
      prevEdge_ = hEdge_;
      hEdge_ = -1;
      shape->highlightVertex(index, Shape::MOVE_VERTEX);
      overrideCursor(CURSOR_POINT);
      this->setToolTip(tr("click & drag to move point"));
      this->setStatusTip(this->toolTip());
      this->update();
      break;
    } else if (index_edge >= 0 && shape->canAddPoint()) {
      if (selectedVertex()) {
        hShape_->highlightClear();
      }
      prevVertex_ = hVertex_;
      hVertex_ = -1;
      prevShape_ = hShape_ = shape;
      prevEdge_ = hEdge_ = index_edge;
      overrideCursor(CURSOR_POINT);
      this->setToolTip(tr("click to create point"));
      this->setStatusTip(this->toolTip());
      this->update();
      break;
    } else if (shape->containsPoint(pos)) {
      if (selectedVertex()) {
        hShape_->highlightClear();
      }
      prevVertex_ = hVertex_;
      hVertex_ = -1;
      prevShape_ = hShape_ = shape;
      prevEdge_ = hEdge_;
      hEdge_ = -1;
      auto tip = fmt::format("Click and drag to move shape {0}",
                             shape->label_.toStdString());
      this->setToolTip(tr(tip.c_str()));
      this->setStatusTip(this->toolTip());
      overrideCursor(CURSOR_GRAB);
      this->update();
      break;
    } else {
      // Nothing found, clear highlights, reset state.
      unHighlight();
    }
    emit vertexSelected(hVertex_ >= 0);
  }
}

void Canvas::mousePressEvent(QMouseEvent *event) {
  auto pos = transformPos(event->position());
  if (event->button() == Qt::LeftButton) {
    if (drawing()) {
      if (current_ != nullptr) {
        // Add point to existing shape.
        if (createMode_ == "polygon") {
          current_->addPoint((*line_)[1]);
          (*line_)[0] = (*current_)[-1];
          if (current_->isClosed()) {
            finalise();
          }
        } else if (std::find(createMode2Points.begin(), createMode2Points.end(),
                             createMode_) != createMode2Points.end()) {
          assert(current_->size() == 1);
          current_->points = line_->points;
          finalise();
        } else if (createMode_ == "linestrip") {
          current_->addPoint((*line_)[1]);
          (*line_)[0] = (*current_)[-1];
          if (event->modifiers() == Qt::ControlModifier) {
            finalise();
          }
        }
      } else if (!outOfPixmap(pos)) {
        // create new shape.
        current_ = new Shape("", QColor(), createMode_, {}, -1);
        current_->addPoint(pos);
        if (createMode_ == "point") {
          finalise();
        } else {
          if (createMode_ == "circle") {
            current_->shape_type("circle");
          }
          line_->points = std::vector<QPointF>{pos, pos};
          setHiding();
          emit drawingPolygon(true);
          this->update();
        }
      }
    }

    else if (editing()) {
      if (selectedEdge()) {
        addPointToEdge();
      } else if (selectedVertex() && event->modifiers() == Qt::ShiftModifier) {
        // Delete point if: left-click + SHIFT on a point
        removeSelectedPoint();
      }

      auto group_mode = event->modifiers() == Qt::ControlModifier;
      selectShapePoint(pos, group_mode);
      prevPoint_ = pos;
      this->repaint();
    }
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::RightButton) {
    size_t pos = selectedShapesCopy_.empty() ? 0 : 1;
    auto *menu = menus[pos];
    restoreCursor();
    if (!menu->exec(this->mapToGlobal(event->pos())) &&
        !selectedShapesCopy_.empty()) {
      // Cancel the move by deleting the shadow copy.
      selectedShapesCopy_.clear();
      selectedShapesCopy_.shrink_to_fit();
      this->repaint();
    }
  } else if (event->button() == Qt::LeftButton) {
    if (editing()) {
      if (hShape_ != nullptr && hShapeIsSelected_ && !movingShape_) {
        QList<Shape *> selectedShapes;
        for (auto &shape : selectedShapes_) {
          if (shape != hShape_) {
            selectedShapes.emplace_back(shape);
          }
        }
        emit selectionChanged(selectedShapes);
      }
    }
  }

  if (movingShape_ && hShape_ != nullptr) {
    auto index =
        std::find(shapes_.begin(), shapes_.end(), hShape_) - shapes_.begin();

    if ((*(shapesBackups_.end() - 1))[index]->points !=
        shapes_[index]->points) {
      storeShapes();
      emit shapedMove();
    }

    movingShape_ = false;
  }
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *event) {
  /*
   * We need at least 4 points here, since the mousePress handler
   * adds an extra one before this handler is called.
   */
  if (double_click_ == "close" && canCloseShape() && current_->size() > 3) {
    current_->popPoint();
    finalise();
  }
}

void Canvas::paintEvent(QPaintEvent *event) {
  if (pixmap_.isNull()) {
    return QWidget::paintEvent(event);
  }

  auto p = painter_;
  p->begin(this);
  p->setRenderHint(QPainter::Antialiasing);
  p->setRenderHint(QPainter::SmoothPixmapTransform);

  p->scale(scale_, scale_);
  p->translate(offsetToCenter());

  p->drawPixmap(0, 0, pixmap_);

  // draw crosshair
  if (crosshair_[createMode_.toStdString()] && drawing() &&
      !prevMovePoint_.isNull() && !outOfPixmap(prevMovePoint_)) {
    p->setPen(QColor(0, 0, 0));
    p->drawLine(0, int(prevMovePoint_.y()), this->width() - 1,
                int(prevMovePoint_.y()));
    p->drawLine(int(prevMovePoint_.x()), 0, int(prevMovePoint_.x()),
                this->height() - 1);
  }

  for (auto &shape : shapes_) {
    if ((shape->selected || !hideBackground_) && isVisible(shape)) {
      shape->scale = scale_;
      shape->fill_ = shape->selected || shape == hShape_;
      shape->paint(p);
    }
  }
  if (current_ != nullptr) {
    current_->paint(p);
    line_->paint(p);
  }
  if (!selectedShapesCopy_.empty()) {
    for (auto &shape : selectedShapesCopy_) {
      shape->paint(p);
    }
  }
  if (fillDrawing() && createMode_ == "polygon" &&
      current_->points.size() >= 2) {
    auto drawing_shape = current_;
    drawing_shape->addPoint((*line_)[1]);
    drawing_shape->fill_ = true;
    drawing_shape->paint(p);
  }
  p->end();
}

void Canvas::wheelEvent(QWheelEvent *event) {
  auto mods = event->modifiers();
  auto delta = event->angleDelta();
  if (Qt::ControlModifier == int(mods)) {
    /*
     * with Ctrl/Command Key
     * zoom
     */
    emit zoomRequest(delta.y(), event->position());
  } else {
    // scroll
    emit scrollRequest(delta.x(), Qt::Horizontal);
    emit scrollRequest(delta.y(), Qt::Vertical);
  }
  event->accept();
}

void Canvas::keyPressEvent(QKeyEvent *event) {
  auto modifiers = event->modifiers();
  auto key = event->key();
  if (drawing()) {
    if (key == Qt::Key_Escape && current_ != nullptr) {
      current_ = nullptr;
      emit drawingPolygon(false);
      this->update();
    } else if (key == Qt::Key_Return && canCloseShape()) {
      finalise();
    } else if (modifiers == Qt::AltModifier) {
      snapping_ = false;
    }
  } else if (editing()) {
    if (key == Qt::Key_Up) {
      moveByKeyBoard(QPointF(0.0, -MOVE_SPEED));
    } else if (key == Qt::Key_Down) {
      moveByKeyBoard(QPointF(0.0, MOVE_SPEED));
    } else if (key == Qt::Key_Left) {
      moveByKeyBoard(QPointF(-MOVE_SPEED, 0.0));
    } else if (key == Qt::Key_Right) {
      moveByKeyBoard(QPointF(MOVE_SPEED, 0.0));
    }
  }
}

// Convert from widget-logical coordinates to pointer-logical ones.
QPointF Canvas::transformPos(QPointF point) {
  return point / scale_ - offsetToCenter();
}

QPointF Canvas::offsetToCenter() {
  auto s = scale_;
  auto area = QWidget::size();
  auto w = pixmap_.width() * s;
  auto h = pixmap_.height() * s;
  auto aw = area.width();
  auto ah = area.height();
  auto x = aw > w ? (aw - w) / (2 * s) : 0;
  auto y = ah > h ? (ah - h) / (2 * s) : 0;
  return {x, y};
}

bool Canvas::outOfPixmap(const QPointF &p) {
  auto w = pixmap_.width();
  auto h = pixmap_.height();
  return !((0 <= p.x() && p.x() <= w - 1) && (0 <= p.y() && p.y() <= h - 1));
}

/*
 * Cycle through each image edge in clockwise fashion,
 * and find the one intersecting the current line segment.
 * http://paulbourke.net/geometry/pointlineplane/
 */
QPointF Canvas::intersectionPoint(const QPointF &p1, const QPointF &p2) {
  auto size = pixmap_.size();
  std::vector<QPointF> points{
      {0, 0},
      {size.width() - 1.0, 0},
      {size.width() - 1.0, size.height() - 1.0},
      {0, size.height() - 1.0},
  };
  // x1, y1 should be in the pixmap, x2, y2 should be out of the pixmap
  auto x1 = std::min(std::max(p1.x(), 0.0), size.width() - 1.0);
  auto y1 = std::min(std::max(p1.y(), 0.0), size.height() - 1.0);
  auto x2 = p2.x();
  auto y2 = p2.y();

  coro_t::pull_type seq([&](coro_t::push_type &yield) {
    intersectingEdges(yield, QPointF(x1, y1), QPointF(x2, y2), points);
  });
  std::vector<double> d_list;
  std::vector<int> i_list;
  std::vector<QPointF> p_list;
  for (auto &item : seq) {
    double d;
    int i;
    QPointF p;
    std::tie(d, i, p) = item;
    d_list.push_back(d);
    i_list.push_back(i);
    p_list.push_back(p);
  }
  //  auto d = *std::min(d_list.begin(), d_list.end());
  auto i = *std::min(i_list.begin(), i_list.end());
  auto p = *std::min(p_list.begin(), p_list.end());

  auto x3 = points[i].x();
  auto y3 = points[i].y();
  auto x4 = points[(i + 1) % 4].x();
  auto y4 = points[(i + 1) % 4].y();

  if (p.x() == x1 && p.y() == y1) {
    // Handle cases where previous point is on one of the edges.
    if (x3 == x4) {
      return {x3, std::min(std::max(0.0, y2), std::max(y3, y4))};
    } else  // y3 == y4
    {
      return {std::min(std::max(0.0, x2), std::max(x3, x4)), y3};
    }
  }
  return p;
}

/*
 * Find intersecting edges.
 *
 * For each edge formed by `points`, yield the intersection
 * with the line segment `(x1,y1) - (x2,y2)`, if it exists.
 * Also return the distance of `(x2,y2)` to the middle of the
 * edge along with its index, so that the one closest can be chosen.
 */
void Canvas::intersectingEdges(coro_t::push_type &yield, const QPointF &point1,
                               const QPointF &point2,
                               const std::vector<QPointF> &points) {
  auto x1 = point1.x();
  auto y1 = point1.y();
  auto x2 = point2.x();
  auto y2 = point2.y();
  for (auto i = 0; i < 4; ++i) {
    auto x3 = points[i].x();
    auto y3 = points[i].y();
    auto x4 = points[(i + 1) % 4].x();
    auto y4 = points[(i + 1) % 4].y();
    auto denom = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1);
    auto nua = (x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3);
    auto nub = (x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3);
    if (denom == 0) {
      /*
       * This covers two cases:
       *  nua == nub == 0: Coincident
       *  otherwise: Parallel
       */
      continue;
    }
    auto ua = nua / denom;
    auto ub = nub / denom;
    if ((0 <= ua && ua <= 1) && (0 <= ub && ub <= 1)) {
      auto x = x1 + ua * (x2 - x1);
      auto y = y1 + ua * (y2 - y1);
      auto m = QPointF((x3 + x4) / 2, (y3 + y4) / 2);
      auto d = distance(m - QPointF(x2, y2));
      yield(std::tuple(d, i, QPointF(x, y)));
    }
  }
}

bool Canvas::closeEnough(const QPointF &p1, const QPointF &p2) {
  return distance(p1 - p2) < (epsilon_ / scale_);
}

bool Canvas::boundedMoveShapes(QList<Shape *> shapes, QPointF pos) {
  if (outOfPixmap(pos)) {
    return false;
  }
  auto o1 = pos + std::get<0>(offsets_);
  if (outOfPixmap(o1)) {
    pos -= QPointF(std::min(0.0, o1.x()), std::min(0.0, o1.y()));
  }
  auto o2 = pos + std::get<1>(offsets_);
  if (outOfPixmap(o2)) {
    pos += QPointF(std::min(0.0, pixmap_.width() - o2.x()),
                   std::min(0.0, pixmap_.height() - o2.y()));
  }
  /*
   * XXX: The next line tracks the new position of the cursor
   * relative to the shape, but also results in making it
   * a bit "shaky" when nearing the border and allows it to
   * go outside the shape's area for some reason.
   * self.calculateOffsets(self.selectedShapes, pos)
   */
  auto dp = pos - prevPoint_;
  if (!dp.isNull()) {
    for (auto &shape : shapes) {
      shape->moveBy(dp);
    }
    prevPoint_ = pos;
    return true;
  }
  return false;
}

void Canvas::boundedMoveVertex(QPointF pos) {
  auto index = hVertex_;
  auto shape = hShape_;
  auto point = (*shape)[index];
  if (outOfPixmap(pos)) {
    pos = intersectionPoint(point, pos);
  }
  shape->moveVertexBy(index, pos - point);
}

void Canvas::addPointToEdge() {
  auto shape = prevShape_;
  auto index = prevEdge_;
  auto point = prevMovePoint_;
  if (shape == nullptr || index < 0 || point.isNull()) {
    return;
  }
  shape->insertPoint(index, point);
  shape->highlightVertex(index, Shape::MOVE_VERTEX);
  hShape_ = shape;
  hVertex_ = index;
  hEdge_ = -1;
  movingShape_ = true;
}

void Canvas::removeSelectedPoint() {
  auto shape = prevShape_;
  auto index = prevVertex_;
  if (shape == nullptr || index < 0) {
    return;
  }
  shape->removePoint(index);
  shape->highlightClear();
  hShape_ = shape;
  prevVertex_ = -1;
  movingShape_ = true;
}

// Select the first shape created which contains this point.
void Canvas::selectShapePoint(const QPointF &point,
                              const bool &multiple_selection_mode) {
  if (selectedVertex()) {
    auto index = hVertex_;
    auto shape = hShape_;
    shape->highlightVertex(index, Shape::MOVE_VERTEX);
  } else {
    QList<Shape *> new_shapes;
    std::move(shapes_.begin(), shapes_.end(), std::back_inserter(new_shapes));
    std::reverse(new_shapes.begin(), new_shapes.end());
    for (auto &shape : new_shapes) {
      if (isVisible(shape) && shape->containsPoint(point)) {
        setHiding();
        if (std::find(selectedShapes_.begin(), selectedShapes_.end(), shape) !=
            selectedShapes_.end()) {
          if (multiple_selection_mode) {
            auto selectedShapes = selectedShapes_;
            selectedShapes.emplace_back(shape);
            emit selectionChanged(selectedShapes);
          } else {
            QList<Shape *> selectedShapes;
            selectedShapes.emplace_back(shape);
            emit selectionChanged(selectedShapes);
          }
          hShapeIsSelected_ = false;
        } else {
          hShapeIsSelected_ = true;
        }
        calculateOffsets(point);
        return;
      }
    }
  }
  deSelectShape();
}

void Canvas::calculateOffsets(const QPointF &point) {
  double left = pixmap_.width() - 1;
  double right = 0;
  double top = pixmap_.height() - 1;
  double bottom = 0;
  for (auto &shape : selectedShapes_) {
    auto rect = shape->boundingRect();
    left = std::min(rect.left(), left);
    right = std::max(rect.right(), right);
    top = std::min(rect.top(), top);
    bottom = std::max(rect.bottom(), bottom);
  }
  auto x1 = left - point.x();
  auto y1 = top - point.y();
  auto x2 = right - point.x();
  auto y2 = bottom - point.y();

  std::get<0>(offsets_) = QPointF(x1, y1);
  std::get<1>(offsets_) = QPointF(x2, y2);
}

void Canvas::finalise() {
  assert(current_ != nullptr);
  current_->close();
  shapes_.emplace_back(current_);
  storeShapes();
  current_ = nullptr;
  setHiding(false);
  emit newShape();
  this->update();
}

bool Canvas::endMove(bool copy) {
  assert(!selectedShapes_.empty() && !selectedShapesCopy_.empty());
  assert(selectedShapes_.size() == selectedShapesCopy_.size());
  if (copy) {
    for (auto i = 0; i < selectedShapesCopy_.size(); ++i) {
      auto &shape = selectedShapesCopy_[i];
      shapes_.emplace_back(shape);
      selectedShapes_[i]->selected = false;
      selectedShapes_[i] = shape;
    }
  } else {
    for (auto i = 0; i < selectedShapesCopy_.size(); ++i) {
      auto &shape = selectedShapesCopy_[i];
      selectedShapes_[i]->points = shape->points;
    }
  }
  selectedShapesCopy_.clear();
  selectedShapesCopy_.shrink_to_fit();
  this->repaint();
  storeShapes();
  return true;
}

void Canvas::hideBackgroundShapes(bool value) {
  hideBackground_ = value;
  if (!selectedShapes_.empty()) {
    setHiding(true);
    this->update();
  }
}

bool Canvas::canCloseShape() {
  return drawing() && current_ != nullptr && current_->size() > 2;
}

void Canvas::selectShapes(const QList<Shape *> &shapes) {
  setHiding();
  emit selectionChanged(shapes);
  this->update();
}
QList<Shape *> Canvas::deleteSelected() {
  QList<Shape *> deleted_shapes{};
  if (!selectedShapes_.empty()) {
    for (auto &shape : selectedShapes_) {
      auto iter = std::remove(shapes_.begin(), shapes_.end(), shape);
      deleted_shapes.emplace_back(shape);
    }
    storeShapes();
    selectedShapes_.clear();
    update();
  }
  return deleted_shapes;
}

void Canvas::deleteShape(Shape *shape) {
  if (std::find(selectedShapes_.begin(), selectedShapes_.end(), shape) !=
      selectedShapes_.end()) {
    auto iter =
        std::remove(selectedShapes_.begin(), selectedShapes_.end(), shape);
  }
  if (std::find(shapes_.begin(), shapes_.end(), shape) != shapes_.end()) {
    auto iter = std::remove(shapes_.begin(), shapes_.end(), shape);
  }
  storeShapes();
  this->update();
}
QList<Shape *> Canvas::duplicateSelectedShapes() {
  if (!selectedShapes_.empty()) {
    selectedShapesCopy_.clear();
    selectedShapesCopy_.shrink_to_fit();
    for (auto &shape : selectedShapes_) {
      selectedShapesCopy_.emplace_back(shape);
    }
    boundedShiftShapes(selectedShapesCopy_);
    endMove(true);
  }
  return selectedShapes_;
}
void Canvas::boundedShiftShapes(const QList<Shape *> &shapes) {
  /*
   * Try to move in one direction, and if it fails in another.
   * Give up if both fail.
   */
  auto point = (*shapes[0])[0];
  auto offset = QPointF(2.0, 2.0);
  offsets_ = std::make_tuple(QPointF(), QPointF());
  prevPoint_ = point;
  if (!boundedMoveShapes(shapes, point - offset)) {
    boundedMoveShapes(shapes, point + offset);
  }
}

/*
 * These two, along with a call to adjustSize are required for the
 * scroll area.
 */
QSize Canvas::sizeHint() { return minimumSizeHint(); }

QSize Canvas::minimumSizeHint() {
  if (pixmap_.isNull()) {
    return scale_ * pixmap_.size();
  }
  return QWidget::minimumSizeHint();
}

void Canvas::moveByKeyBoard(const QPointF &offset) {
  if (!selectedShapes_.empty()) {
    boundedMoveShapes(selectedShapes_, prevPoint_ + offset);
    this->repaint();
    movingShape_ = true;
  }
}
void Canvas::keyReleaseEvent(QKeyEvent *event) {
  auto modifiers = event->modifiers();
  if (drawing()) {
    if (modifiers == 0) {
      snapping_ = true;
    }
  } else if (editing()) {
    if (movingShape_ && !selectedShapes_.empty()) {
      auto index =
          std::find(shapes_.begin(), shapes_.end(), selectedShapes_[0]) -
          shapes_.begin();
      if ((*(shapesBackups_.end() - 1))[index]->points !=
          shapes_[index]->points) {
        storeShapes();
        emit shapedMove();
      }
      movingShape_ = false;
    }
  }
}
Shape *Canvas::setLastLabel(const QString &text,
                            const std::map<std::string, bool> &flags) {
  (*(shapes_.end() - 1))->label_ = text;
  (*(shapes_.end() - 1))->flags_ = flags;
  shapesBackups_.pop_back();
  storeShapes();
  return *(shapes_.end() - 1);
}

void Canvas::undoLastLine() {
  current_ = *(shapes_.end() - 1);
  current_->setOpen();
  shapes_.pop_back();
  if (std::find(createModeMultiPoints.begin(), createModeMultiPoints.end(),
                createMode_) != createModeMultiPoints.end()) {
    line_->points = {(*current_)[-1], (*current_)[0]};
  } else if (std::find(createMode2Points.begin(), createMode2Points.end(),
                       createMode_) != createMode2Points.end()) {
    current_->points = {current_->points[0], current_->points[1]};
  } else if (createMode_ == "point") {
    delete current_;
    current_ = nullptr;
  }
  emit drawingPolygon(true);
}
void Canvas::undoLastPoint() {
  if (current_ == nullptr || current_->isClosed()) {
    return;
  }
  current_->popPoint();
  if (current_->size() > 0) {
    (*line_)[0] = (*current_)[-1];
  } else {
    delete current_;
    current_ = nullptr;
    emit drawingPolygon(false);
  }
  this->update();
}

void Canvas::loadPixmap(const QPixmap &pixmap, const bool clear_shapes) {
  pixmap_ = pixmap;
  if (clear_shapes) {
    for (auto &shape : shapes_) {
      delete shape;
      shape = nullptr;
    }
    shapes_.clear();
    shapes_.shrink_to_fit();
  }
  this->update();
}

void Canvas::loadShapes(const QList<Shape *> &shapes, const bool replace) {
  if (replace) {
    this->shapes_ = shapes;
  } else {
    std::copy(shapes.begin(), shapes.end(), std::back_inserter(this->shapes_));
  }
  storeShapes();
  delete current_;
  current_ = nullptr;
  delete hShape_;
  hShape_ = nullptr;
  hVertex_ = -1;
  hEdge_ = -1;
  this->update();
}
void Canvas::setShapeVisible(Shape *shape, const bool value) {
  visible_[shape] = value;
  this->update();
}
void Canvas::resetState() {
  restoreCursor();
  pixmap_ = QPixmap();
  for (auto &shape_list : shapesBackups_) {
    for (auto &shape : shape_list) {
      delete shape;
      shape = nullptr;
    }
    shape_list.clear();
  }
  shapesBackups_.clear();
  this->update();
}
