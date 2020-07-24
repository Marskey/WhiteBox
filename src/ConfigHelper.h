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
    CConfigHelper(QObject *parent = nullptr);
    ~CConfigHelper();

    bool init(const QString &configuration);

    bool isFirstCreateFile();
    void deleteConfigFile();

    void saveMainWindowGeometry(const QByteArray &geometry);
    void saveMainWindowState(const QByteArray &state);

    QByteArray getMainWindowGeometry() const;
    QByteArray getMainWindowState() const;

    void saveSplitterH(const QByteArray& state);
    void saveSplitterV(const QByteArray& state);

    QByteArray getSplitterH() const;
    QByteArray getSplitterV() const;

    void saveWidgetComboxState(const QString& name, const QComboBox& combox);
    void restoreWidgetComboxState(const QString& name, QComboBox& combox);
    QString getWidgetComboxStateText(const QString& name, int index);

    void saveWidgetCheckboxState(const QString& name, const QCheckBox& checkbox);
    void restoreWidgetCheckboxState(const QString& name, QCheckBox& checkbox);

    int getHistroyComboxItemMaxCnt();

    QString getStyleSheetLineEditNormal();
    QString getStyleSheetLineEditError();

    void saveCachePath(const QString& path);
    QString getCachePath();

    QString getFont();
    void saveFont(const QString& family);

    google::protobuf::util::JsonPrintOptions& getJsonPrintOption();

private:
    QSettings *m_pSettings;
    QString m_configFile;

    bool m_bFirstCreateFile = false;

    QByteArray m_subWindowGeometry;
    int m_maxComboxHistroyItemCnt = 0; // ip 和 账号的历史数量
    google::protobuf::util::JsonPrintOptions m_protoJsonOption;
};

typedef CSingleton<CConfigHelper> ConfigHelper;
