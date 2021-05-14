#pragma once

#include "Singleton.h"
#include <QSettings>
#include <google/protobuf/util/json_util.h>

class QCheckBox;
class QComboBox;
class CConfigHelper : public QObject
{
  Q_OBJECT

public:
  CConfigHelper(QObject* parent = nullptr);
  ~CConfigHelper();

  bool init(const QString& configuration);

  bool isFirstCreateFile();
  void deleteConfigFile();

  void saveMainWindowGeometry(const QByteArray& geometry);
  void saveMainWindowState(const QByteArray& state);

  QByteArray getMainWindowGeometry() const;
  QByteArray getMainWindowState() const;

  void saveSplitterH(const QByteArray& state);
  void saveSplitterV(const QByteArray& state);

  QByteArray getSplitterH() const;
  QByteArray getSplitterV() const;

  void saveWidgetComboBoxState(const QString& name, const QComboBox& comboBox);
  void restoreWidgetComboBoxState(const QString& name, QComboBox& comboBox);
  QString getWidgetComboBoxStateText(const QString& name, int index);

  void saveWidgetCheckboxState(const QString& name, const QCheckBox& checkbox);
  void restoreWidgetCheckboxState(const QString& name, QCheckBox& checkbox);

  int getHistroyComboBoxItemMaxCnt();

  void saveCachePath(const QString& path);
  QString getCachePath();

  QString getFont();
  void saveFont(const QString& family);

  QString getJsonHighlightColor(const QString& name);

  google::protobuf::util::JsonPrintOptions& getJsonPrintOption();

private:
  QSettings* m_pSettings;
  QString m_configFile;

  bool m_bFirstCreateFile = false;

  QByteArray m_subWindowGeometry;
  int m_maxComboBoxHistroyItemCnt = 0;
  google::protobuf::util::JsonPrintOptions m_protoJsonOption;
};

typedef CSingleton<CConfigHelper> ConfigHelper;
