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

QByteArray CConfigHelper::getMainWindowGeometry() const {
    return m_pSettings->value("/Window/MainWindowGeometry").toByteArray();
}

QByteArray CConfigHelper::getMainWindowState() const {
    return m_pSettings->value("/Window/MainWindowState").toByteArray();
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

QString CConfigHelper::getWidgetComboxStateText(const QString& name, int index) {
    QStringList stringList = m_pSettings->value("/Widget/" + name).toString().split(",");
    if (stringList.count() > index
        && index >= 0) {
        return stringList[index];
    }
    return "";
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

void CConfigHelper::saveCachePath(const QString& path) {
    m_pSettings->setValue("/Global/CachePath", path);
}

QString CConfigHelper::getCachePath() {
    return m_pSettings->value("/Global/CachePath").toString();
}

QString CConfigHelper::getFont() {
    return m_pSettings->value("/Global/font").toString();
}

void CConfigHelper::saveFont(const QString& family) {
    m_pSettings->setValue("/Global/font", family);
}

QString CConfigHelper::getJsonHighlightColor(const QString& name) {
    return m_pSettings->value("/JsonColor/" + name).toString();
}

google::protobuf::util::JsonPrintOptions& CConfigHelper::getJsonPrintOption() {
    return m_protoJsonOption;
}
