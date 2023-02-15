#ifndef LABELMEPLUS_LABEL_DIALOG_H
#define LABELMEPLUS_LABEL_DIALOG_H

#include <QCompleter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

class LabelQLineEdit : public QLineEdit {
  QListWidget* list_widget_;

 public:
  void setListWidget(QListWidget* list_widget);

 protected:
  void keyPressEvent(QKeyEvent* event) override;
};

class LabelDialog : public QDialog {
  Q_OBJECT

 public:
  explicit LabelDialog(
      const std::map<std::string, bool>& fit_to_content,
      const std::map<std::string, std::vector<std::string>>& flags,
      const QStringList& labels, bool sort_labels = true,
      bool show_text_field = true, const std::string& completion = "startswith",
      QWidget* parent = nullptr, const QString& text = "Enter object label");
  void addLabelHistory(const QString& label);
  void popUp(QString& out_label, std::map<std::string, bool>& out_flags,
             int& out_group_id, const QString& text = "", bool move = true,
             const std::map<std::string, bool>& flags = {}, int group_id = -1);

  LabelQLineEdit* edit_;

 private:
  std::map<std::string, bool> fit_to_content_{
      {"row", false},
      {"column", true},
  };
  QLineEdit* edit_group_id_;

  QVBoxLayout* layout_;
  QDialogButtonBox *buttonBox_, *bb_;

  QListWidget* labelList_;

  bool sort_labels_;

  std::map<std::string, std::vector<std::string>> flags_;
  QVBoxLayout* flagsLayout_;

  QCompleter* completer_;

 private slots:
  void labelSelected(QListWidgetItem* item);
  void validate();
  void labelDoubleClicked(QListWidgetItem* item);
  void postProcess();
  void updateFlags(const QString& label_new);
  void deleteFlags();
  void resetFlags(const QString& label = "");
  void setFlags(const std::map<std::string, bool>& flags);
  std::map<std::string, bool> getFlags();
  int getGroupId();
};
#endif  // LABELMEPLUS_LABEL_DIALOG_H
