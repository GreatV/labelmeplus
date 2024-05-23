#ifndef LABEL_DIALOG_H
#define LABEL_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>

class LabelQLineEdit : public QLineEdit {
  Q_OBJECT

 public:
  explicit LabelQLineEdit(QWidget* parent = nullptr);
  void setListWidget(QListWidget* list_widget);

 protected:
  void keyPressEvent(QKeyEvent* event) override;

 private:
  QListWidget* list_widget_ = nullptr;
};

class LabelDialog : public QDialog {
  Q_OBJECT
 public:
  explicit LabelDialog(const QString& text = "Enter object label",
                       QWidget* parent = nullptr,
                       const QStringList& labels = {},
                       const bool sort_labels = true,
                       const bool show_text_field = true,
                       const QString& completion = "startwith",
                       const std::map<QString, bool>& fit_to_content = {},
                       const std::map<QString, bool>& flags = {});

 private:
  std::map<QString, bool> fit_to_content_ = {{"row", false}, {"column", true}};
  LabelQLineEdit* edit_ = nullptr;
 private slots:
  void postProcess();
};

#endif  // LABEL_DIALOG_H
