#include "app.h"

app::app(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle(__appname__);
  dirty = false;
  _noSelectionSlot = false;
  _copied_shapes = nullptr;
}

app::~app() {}
