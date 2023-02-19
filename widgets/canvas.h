#ifndef LABELMEPLUS_CANVAS_H
#define LABELMEPLUS_CANVAS_H

#include <QEnterEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QWidget>
#include <any>
#include <boost/coroutine2/all.hpp>

#include "shape.h"

using namespace lmp;

typedef boost::coroutines2::coroutine<std::tuple<double, int, QPointF>> coro_t;

static Qt::CursorShape CURSOR_DEFAULT = Qt::ArrowCursor;
static Qt::CursorShape CURSOR_POINT = Qt::PointingHandCursor;
static Qt::CursorShape CURSOR_DRAW = Qt::CrossCursor;
static Qt::CursorShape CURSOR_MOVE = Qt::ClosedHandCursor;
static Qt::CursorShape CURSOR_GRAB = Qt::OpenHandCursor;

class Canvas : public QWidget {
  Q_OBJECT
 public:
  explicit Canvas(
      double epsilon = 10.0, const QString& double_click = "close",
      int num_backups = 10,
      const std::map<std::string, bool>& crosshair = {{"polygon", false},
                                                      {"rectangle", true},
                                                      {"circle", false},
                                                      {"line", false},
                                                      {"point", false},
                                                      {"linestrip", false}},
      QWidget* parent = nullptr);
  ~Canvas();
  QList<Shape*> selectedShapes_;
  QList<Shape*> shapes_;
  std::vector<QList<Shape*>> shapesBackups_;
  Shape* hShape_;
  double scale_ = 1.0;
  QPixmap pixmap_;

  unsigned char CREATE = 0;
  unsigned char EDIT = 1;

  int setcursor_count_ = 0;

  /* Menus:
   * 0: right-click without selection and dragging of shapes
   * 1: right-click with selection and dragging of shapes
   */
  QMenu* menus[2];

 private:
  QString createMode_ = "polygon";
  bool fill_drawing_ = false;
  unsigned char mode_;
  Shape* current_;
  QList<Shape*> selectedShapesCopy_;

  Shape* line_;
  QPointF prevPoint_;
  QPointF prevMovePoint_;
  std::tuple<QPointF, QPointF> offsets_;
  std::map<Shape*, bool> visible_;
  bool _hideBackground = false;
  bool hideBackground_ = false;

  QPainter* painter_;
  QCursor cursor_ = QCursor(CURSOR_DEFAULT);

  Shape* prevShape_;
  int prevVertex_ = -1;
  int prevEdge_ = -1;
  int hVertex_ = -1;
  int hEdge_ = -1;
  bool movingShape_ = false;
  bool snapping_ = true;
  bool hShapeIsSelected_ = false;

  double epsilon_;
  QString double_click_;
  int num_backups_;
  std::map<std::string, bool> crosshair_;

 public:
  bool fillDrawing();
  QString createMode();
  void createMode(const QString& value);
  void storeShapes();
  bool isShapeRestorable();
  void restoreShape();
  bool isVisible(Shape* shape);
  bool drawing();
  bool editing();
  void setEditing(bool value = true);
  void loadShapes(const QList<Shape*>& shapes, bool replace = true);
  QList<Shape*> duplicateSelectedShapes();
  void deSelectShape();
  void selectShapes(const QList<Shape*>& shapes);
  void setShapeVisible(Shape* shape, bool value);
  Shape* setLastLabel(const QString& text,
                      const std::map<std::string, bool>& flags);
  void undoLastLine();

  bool selectedVertex();
  bool selectedEdge();
  void resetState();
  void loadPixmap(const QPixmap& pixmap, bool clear_shapes = true);
  void removeSelectedPoint();
  void deleteShape(Shape* shape);
  QList<Shape*> deleteSelected();
  bool endMove(bool copy);

 private:
  void restoreCursor();
  void unHighlight();

  void setHiding(bool enable = true);
  QPointF transformPos(QPointF point);
  QPointF offsetToCenter();
  bool outOfPixmap(const QPointF& p);
  QPointF intersectionPoint(const QPointF& p1, const QPointF& p2);
  void intersectingEdges(coro_t::push_type& yield, const QPointF& point1,
                         const QPointF& point2,
                         const std::vector<QPointF>& points);
  bool closeEnough(const QPointF& p1, const QPointF& p2);
  bool boundedMoveShapes(QList<Shape*> shapes, QPointF pos);
  void boundedMoveVertex(QPointF pos);
  void addPointToEdge();

  void selectShapePoint(const QPointF& point,
                        const bool& multiple_selection_mode);
  void calculateOffsets(const QPointF& point);
  void finalise();

  void hideBackgroundShapes(bool value);
  bool canCloseShape();
  void boundedShiftShapes(const QList<Shape*>& shapes);
  QSize sizeHint();
  QSize minimumSizeHint();
  void moveByKeyBoard(const QPointF& offset);
  void overrideCursor(const QCursor& cursor);

 protected:
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;

 signals:
  void zoomRequest(int, QPointF);
  void scrollRequest(int, int);
  void newShape();
  void selectionChanged(QList<lmp::Shape*>);
  void shapedMove();
  void drawingPolygon(bool);
  void vertexSelected(bool);

 private slots:
  void undoLastPoint();
  void setFillDrawing(bool value);
};

#endif  // LABELMEPLUS_CANVAS_H
