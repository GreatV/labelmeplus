#include "tool_bar.h"

#include <QLayout>
#include <QToolButton>

ToolBar::ToolBar(const QString &title, QWidget *parent)
    : QToolBar(title, parent) {
  auto *layout = this->layout();
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  this->setContentsMargins(0, 0, 0, 0);
  this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
}

void ToolBar::addAction(QAction *action) {
  auto *btn = new QToolButton();
  btn->setDefaultAction(action);
  btn->setToolButtonStyle(this->toolButtonStyle());
  this->addWidget(btn);

  // center align
  for (auto i = 0; i < this->layout()->count(); ++i) {
    this->layout()->itemAt(i)->setAlignment(Qt::AlignCenter);
  }
}

void ToolBar::addAction(QWidgetAction *action) { QToolBar::addAction(action); }
