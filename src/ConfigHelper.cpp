#include "ConfigHelper.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QComboBox>

#define MAX_LIST_HISTROY_CNT 10

CConfigHelper::CConfigHelper(QObject* parent) : QObject(parent) {
    m_protoJsonOption.add_whitespace = true;
    m_protoJsonOption.always_print_primitive_fields = true;
    m_protoJsonOption.preserve_proto_field_names = true;
}

CConfigHelper::~CConfigHelper() {
    delete m_pSettings;
    m_pSettings = nullptr;
}

bool CConfigHelper::init(const QString& configuration) {
    m_configFile = configuration;
    QFile file = QFile(m_configFile);
    m_bFirstCreateFile = !file.exists();
    m_pSettings = new QSettings(m_configFile, QSettings::IniFormat, this);

    m_maxComboxHistroyItemCnt = m_pSettings->value("/Widget/ComboxMaxHistroyCount").toInt();
    if (0 == m_maxComboxHistroyItemCnt) {
        m_maxComboxHistroyItemCnt = MAX_LIST_HISTROY_CNT;
    }

    return true;
}

bool CConfigHelper::isFirstCreateFile() {
    return m_bFirstCreateFile;
}

void CConfigHelper::deleteConfigFile() {
    QFile file = QFile(m_configFile);
    file.remove();
}

void CConfigHelper::saveMainWindowGeometry(const QByteArray& geometry) {
    m_pSettings->setValue("/Window/MainWindowGeometry", QVariant(geometry));
}

void CConfigHelper::saveMainWindowState(const QByteArray& state) {
    m_pSettings->setValue("/Window/MainWindowState", QVariant(state));
}

void CConfigHelper::saveSubWindowGeometry(const QByteArray& geometry) {
    m_subWindowGeometry = geometry;
    m_pSettings->setValue("/Window/subWindowGeometry", QVariant(geometry));
}

QByteArray CConfigHelper::getMainWindowGeometry() const {
    return m_pSettings->value("/Window/MainWindowGeometry").toByteArray();
}

QByteArray CConfigHelper::getMainWindowState() const {
    return m_pSettings->value("/Window/MainWindowState").toByteArray();
}

QByteArray CConfigHelper::getSubWindowGeometry() {
    if (m_subWindowGeometry.isEmpty()) {
        m_subWindowGeometry = m_pSettings->value("/Window/subWindowGeometry").toByteArray();
    }
    return m_subWindowGeometry;
}

void CConfigHelper::saveSplitterH(const QByteArray& state) {
    m_pSettings->setValue("/Window/SplitterHorizon", QVariant(state));
}

void CConfigHelper::saveSplitterV(const QByteArray& state) {
    m_pSettings->setValue("/Window/SplitterVertical", QVariant(state));
}

QByteArray CConfigHelper::getSplitterH() const {
    return m_pSettings->value("/Window/SplitterHorizon").toByteArray();
}

QByteArray CConfigHelper::getSplitterV() const {
    return m_pSettings->value("/Window/SplitterVertical").toByteArray();
}

void CConfigHelper::saveProtoPath(const QString& path) {
    m_pSettings->setValue("/Protobuf/ProtoPath", path);
}

QString CConfigHelper::getProtoPath() const {
    return m_pSettings->value("/Protobuf/ProtoPath").toString();
}

void CConfigHelper::saveWidgetComboxState(const QString& name, const QComboBox& combox) {
    QStringList accountList;
    for (int index = 0; index < combox.count(); index++) {
        accountList.append(combox.itemText(index));
    }
    m_pSettings->setValue("/Widget/" + name, accountList.join(","));
}

void CConfigHelper::restoreWidgetComboxState(const QString& name, QComboBox& combox) {
    QStringList stringList = m_pSettings->value("/Widget/" + name).toString().split(",");
    for (int i = 0; i < stringList.count(); ++i) {
        if (stringList[i].isEmpty()) {
            continue;
        }
        combox.addItem(stringList[i]);
    }

    if (!stringList.empty()) {
        combox.setCurrentIndex(0);
    }
}

void CConfigHelper::saveWidgetCheckboxState(const QString& name, const QCheckBox& checkbox) {
    m_pSettings->setValue("/Widget/" + name, checkbox.checkState());
}

void CConfigHelper::restoreWidgetCheckboxState(const QString& name, QCheckBox& checkbox) {
    checkbox.setCheckState(static_cast<Qt::CheckState>(m_pSettings->value("/Widget/" + name).toInt()));
}

int CConfigHelper::getHistroyComboxItemMaxCnt() {
    return m_maxComboxHistroyItemCnt;
}

QString CConfigHelper::getStyleSheetLineEditNormal() {
    return "QLineEdit {\
  background-color: #ffffff;\
  padding-top: 2px;\
  padding-bottom: 2px;\
  padding-left: 4px;\
  padding-right: 4px;\
  border-style: solid;\
  border: 1px solid rgb(122, 122, 122);\
}\
QLineEdit:disabled {\
  background-color: #19232D;\
  color: #787878;\
}\
QLineEdit:focus {\
  border: 1px solid rgb(0, 116, 207);\
}\
QLineEdit:hover {\
  border: 1px solid rgb(0, 0, 0);\
  widget-animation-duration: 10000;\
}\
QLineEdit:selected {\
  background-color: rgb(0, 116, 207);\
  color: #32414B;\
}";
}

QString CConfigHelper::getStyleSheetLineEditError() {
    return "QLineEdit {\
  background-color: #ffffff;\
  padding-top: 2px;\
  padding-bottom: 2px;\
  padding-left: 4px;\
  padding-right: 4px;\
  border-style: solid;\
  border: 1px solid rgb(229, 20, 0);\
}\
QLineEdit:disabled {\
  background-color: #19232D;\
  color: #787878;\
}\
QLineEdit:focus {\
  border: 1px solid rgb(200, 100, 0);\
}\
QLineEdit:hover {\
  border: 1px solid rgb(200, 20, 0);\
}\
QLineEdit:selected {\
  background-color: rgb(200, 100, 0);\
  color: #32414B;\
}";
}

void CConfigHelper::saveCachePath(const QString& path) {
    m_pSettings->setValue("/Global/CachePath", path);
}

QString CConfigHelper::getCachePath() {
    return m_pSettings->value("/Global/CachePath").toString();
}

google::protobuf::util::JsonPrintOptions& CConfigHelper::getJsonPrintOption() {
    return m_protoJsonOption;
}

void CConfigHelper::saveLuaScriptPath(const QString& path) {
    m_pSettings->setValue("/LuaScript/path/", path);
}

QString CConfigHelper::getLuaScriptPath() {
    return m_pSettings->value("/LuaScript/path/").toString();
}
