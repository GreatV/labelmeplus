#ifndef FILEDIALOGPREVIEW_H
#define FILEDIALOGPREVIEW_H

#include <QFileDialog>
#include <QLabel>
#include <QScrollArea>

class ScrollAreaPreview : public QScrollArea {
 public:
  explicit ScrollAreaPreview(QWidget* parent = nullptr);
  QLabel* label{};
  void setText(const QString& text);
  void setPixmap(const QPixmap& pixmap);
  void clear();
};

class FileDialogPreview : public QFileDialog {
  Q_OBJECT
 public:
  explicit FileDialogPreview(QWidget* parent = nullptr,
                             const QString& caption = QString(),
                             const QString& directory = QString(),
                             const QString& filter = QString());
  ScrollAreaPreview* labelPreview{};
 private slots:
  void onChange(const QString& path);
};

#endif  // FILEDIALOGPREVIEW_H
