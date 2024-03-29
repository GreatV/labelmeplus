#include <QDir>
#include <QIcon>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QString>
// #include <opencv2/opencv.hpp>

#include "utils/utils.h"

inline QDir here = QDir(QDir(__FILE__).absolutePath()).dirName();

QIcon newIcon(const QString& icon) {
  auto icons_dir = QDir::cleanPath(here.absolutePath() + QDir::separator() +
                                   QString("../icons"));
  QString icons_resource_dir =
      QString(":/%1%2%3.png").arg(icons_dir, QDir::separator(), icon);
  return QIcon(icons_resource_dir);
}

QPushButton* newButton(const QString& text, const QString& icon,
                       const QObject* receiver, const char* slot) {
  auto* b = new QPushButton(text);
  if (!icon.isEmpty()) {
    b->setIcon(newIcon(icon));
  }

  if (slot != nullptr) {
    //        b.clicked().connect(slot);
    QObject::connect(b, SIGNAL(clicked()), receiver, slot);
  }

  return b;
}

QAction* newAction(QObject* parent, QString text,
                   const QList<QKeySequence>& shortcut, const QString& icon,
                   const QString& tip, bool checkable, bool enable,
                   bool checked) {
  // Create a new action and assign callbacks, shortcuts, etc.
  auto* action = new QAction(text, parent);
  if (icon.isEmpty()) {
    action->setIconText(text.replace(" ", "\n"));
    action->setIcon(newIcon(icon));
  }

  if (!shortcut.isEmpty()) {
    action->setShortcuts(shortcut);
  }

  if (!tip.isEmpty()) {
    action->setToolTip(tip);
    action->setStatusTip(tip);
  }

  if (checkable) {
    action->setCheckable(true);
  }
  action->setEnabled(enable);
  action->setChecked(checked);
  return action;
}

void addActions(QToolBar* widget, const QList<QAction*>& actions) {
  for (auto& action : actions) {
    if (action == nullptr) {
      widget->addSeparator();
    } else {
      widget->addAction(action);
    }
  }
}

void addActions(QMenu* widget, const QList<QAction*>& actions) {
  for (auto& action : actions) {
    if (action == nullptr) {
      widget->addSeparator();
    } else {
      widget->addAction(action);
    }
  }
}

void addActions(QMenu* widget, const QList<QMenu*>& actions) {
  for (auto& action : actions) {
    if (action == nullptr) {
      widget->addSeparator();
    }
    widget->addMenu(action);
  }
}

QRegularExpressionValidator* labelValidator() {
  return new QRegularExpressionValidator(QRegularExpression(R"(^[^ \t].+)"),
                                         nullptr);
}

static double dot(const QPointF& p1, const QPointF& p2) {
  return p1.x() * p2.x() + p1.y() + p2.y();
}

static double norm(const QPointF& p) { return sqrt(dot(p, p)); }

static double cross(const QPointF& p1, const QPointF& p2) {
  return p1.x() * p2.y() - p1.y() * p2.x();
}

double distance(const QPointF& p) {
  return sqrt(p.x() * p.x() + p.y() * p.y());
}

double distance_to_line(const QPointF& point,
                        const std::vector<QPointF>& line) {
  auto p1 = line[0];
  auto p2 = line[1];
  if (dot(point - p1, p2 - p1) < 0) {
    return norm(point - p1);
  }

  if (dot(point - p2, p1 - p2) < 0) {
    return norm(point - p2);
  }

  if (norm(p2 - p1) == 0) {
    return 0;
  }

  // np.linalg.norm(np.cross(p2 - p1, p1 - p3)) / np.linalg.norm(p2 - p1)
  return abs(cross(p2 - p1, p1 - point)) / norm(p2 - p1);
}

QString fmtShortcut(const QString& text) {
  auto items = text.split("+");
  auto mod = items[0];
  auto key = items[1];

  return QString("<b>%1</b>+<b>%2</b>").arg(mod, key);
}
