#pragma once

#include <qapplication.h>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTextCursor>

#include "ConfigHelper.h"

class CJsonHighlighter
{
public:
    CJsonHighlighter(QTextDocument* parent = 0) : m_pParent(parent) {
        m_pattern = QRegularExpression("(\"(\\\\u[a-zA-Z0-9]{4}|\\\\[^u]|[^\"])*\"(\\s*:)?|\\b(true|false)\\b|-?\\d+(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?)");
        QString keyColor = ConfigHelper::instance().getJsonHighlightColor("key");
        m_keyFormat.setForeground(QColor((keyColor.isEmpty()) ? "#AF7AC5" : keyColor));
        QString boolColor = ConfigHelper::instance().getJsonHighlightColor("bool");
        m_boolFormat.setForeground(QColor((boolColor.isEmpty()) ? "#3498DB" : boolColor));
        QString strColor = ConfigHelper::instance().getJsonHighlightColor("string");
        m_stringFormat.setForeground(QColor((strColor.isEmpty()) ? "#1ABC9C" : strColor));
        QString numberColor = ConfigHelper::instance().getJsonHighlightColor("number");
        m_numberFormat.setForeground(QColor((numberColor.isEmpty()) ? "#3498DB" : numberColor));
    }

    void hightlight() {
        QRegularExpressionMatchIterator matchIterator = m_pattern.globalMatch(m_pParent->toPlainText());
        QTextCharFormat* pFormat = &m_numberFormat;
        QTextCursor cursor(m_pParent);
        QTextCharFormat plainFormat(cursor.charFormat());
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();

            auto capturedLength = match.capturedLength();
            QString word = match.captured();

            pFormat = &m_numberFormat;

            QRegularExpression re;
            re.setPattern("^\"");
            QRegularExpressionMatch subMatch = re.match(word);

            re.setPattern("true|false");
            QRegularExpressionMatch subMatch2 = re.match(word);

            if (subMatch.hasMatch()) {
                re.setPattern(":$");
                subMatch = re.match(word);
                if (subMatch.hasMatch()) {
                    capturedLength -= 1;
                    pFormat = &m_keyFormat;
                } else {
                    pFormat = &m_stringFormat;
                }
            } else if (subMatch2.hasMatch()) {
                pFormat = &m_boolFormat;
            }

            cursor.setPosition(match.capturedStart());
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, capturedLength);
            cursor.setCharFormat(*pFormat);
        }
        cursor.setPosition(0);
        cursor.setCharFormat(plainFormat);
    }

private:
    QTextDocument* m_pParent = nullptr;
    QRegularExpression m_pattern;

    QTextCharFormat m_keyFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_boolFormat;
};
