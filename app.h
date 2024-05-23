#ifndef APP_H
#define APP_H

#include <QMainWindow>

class app : public QMainWindow {
  Q_OBJECT

 public:
  explicit app(QWidget* parent = nullptr);
  ~app();

 private:
  QString __appname__ = "labelme++";
  bool dirty = false;
  bool _noSelectionSlot = false;
  void* _copied_shapes = nullptr;
};
#endif  // APP_H
