#ifndef LABELMEPLUS_TOOL_BAR_H
#define LABELMEPLUS_TOOL_BAR_H

#include <QAction>
#include <QToolBar>
#include <QWidgetAction>

class ToolBar : public QToolBar {
  Q_OBJECT
 public:
  explicit ToolBar(const QString& title, QWidget* parent = nullptr);
  void addAction(QAction* action);
  void addAction(QWidgetAction* action);
};

#endif  // LABELMEPLUS_TOOL_BAR_H
