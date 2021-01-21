#pragma once

#include "ui_MessageIgnore.h"

#include "google/protobuf/message.h"

class CMsgIgnoreDialog : public QDialog, public Ui_MsgIgnore
{
  Q_OBJECT

public:
  CMsgIgnoreDialog(QWidget* parent = 0);
  ~CMsgIgnoreDialog();

  void init(std::unordered_set<std::string>& setIgnoredMsg);
  std::list<std::string> getIgnoredMsg();

private:
  void filterMessage(const QString& filterText, QListWidget& listWidget);

private slots:
  void handleLeftFilterTextChanged(const QString& text);
  void handleRightFilterTextChanged(const QString& text);

  void handleAll2RightBtnClicked();
  void handleAll2LeftBtnClicked();

  void handleOne2RightBtnClicked();
  void handleOne2LeftBtnClicked();

};
