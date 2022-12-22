#include <QAction>
#include <QDir>
#include <QIcon>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QString>
#include <opencv2/opencv.hpp>

#include "utils/utils.h"

QDir here = QDir(QDir(__FILE__).absolutePath()).dirName();

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

QAction* newAction(QObject* parent, QString text, const QObject* receiver,
                   const char* slot, const QList<QKeySequence>& shortcuts,
                   const QString& icon, const QString& tip,
                   bool checkable = false, bool enable = true,
                   bool checked = false) {
  QAction* a = new QAction(text, parent);
  if (!icon.isEmpty()) {
    a->setIconText(text.replace(" ", "\n"));
    a->setIcon(newIcon(icon));
  }

  if (!shortcuts.empty()) {
    a->setShortcuts(shortcuts);
  }

  if (!tip.isEmpty()) {
    a->setToolTip(tip);
    a->setStatusTip(tip);
  }

  if (slot != nullptr) {
    QObject::connect(a, SIGNAL(triggered()), receiver, slot);
  }

  if (checkable) {
    a->setCheckable(true);
  }

  a->setEnabled(enable);
  a->setChecked(checked);

  return a;
}

void addActions(QMenu* widget, QList<QAction*> actions) {
  for (auto& action : actions) {
    if (action == nullptr) {
      widget->addSeparator();
    }
    widget->addAction(action);
  }
}

void addActions(QMenu* widget, QList<QMenu*> actions) {
  for (auto& action : actions) {
    if (action == nullptr) {
      widget->addSeparator();
    }
    widget->addMenu(action);
  }
}

QRegularExpressionValidator labelValidator() {
  return QRegularExpressionValidator(QRegularExpression("^[^ \t].+"), nullptr);
}

double distance(const cv::Point& p) { return sqrt(p.x * p.x + p.y * p.y); }

double distancetoline() { return 0; }

QString fmtShortcut(const QString& text) {
  auto items = text.split("+");
  auto mod = items[0];
  auto key = items[1];

  return QString("<b>%1</b>+<b>%2</b>").arg(mod, key);
}
