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
        m_keyColor = ConfigHelper::instance().getJsonHighlightColor("key");
        m_keyColor = (m_keyColor.isEmpty()) ? "#AF7AC5" : m_keyColor;
        m_keyFormat.setForeground(QColor(m_keyColor));

        m_boolColor = ConfigHelper::instance().getJsonHighlightColor("bool");
        m_boolColor = (m_boolColor.isEmpty()) ? "#3498DB" : m_boolColor;
        m_boolFormat.setForeground(QColor(m_boolColor));

        m_strColor = ConfigHelper::instance().getJsonHighlightColor("string");
        m_strColor = (m_strColor.isEmpty()) ? "#1ABC9C" : m_strColor;
        m_stringFormat.setForeground(QColor(m_strColor));

        m_numberColor = ConfigHelper::instance().getJsonHighlightColor("number");
        m_numberColor = (m_numberColor.isEmpty()) ? "#3498DB" : m_numberColor;
        m_numberFormat.setForeground(QColor(m_numberColor));
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

    std::string highlightJsonData(const QString& jsonData) {
        QString result = jsonData;
        QRegularExpression reAll;
        reAll.setPattern("\"(\\\\u[a-zA-Z0-9]{4}|\\\\[^u]|[^\"])*\"(\\s*:)?|\\b(true|false)\\b|-?\\d+(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?|(\n)|(\\s)");
        QRegularExpressionMatchIterator i = reAll.globalMatch(jsonData);

        int offset = 0;
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString word = match.captured();
            auto capturedLength = match.capturedLength();

            if (word == '\n') {
                result.replace(match.capturedStart() + offset, capturedLength, "<br>");
                offset += 4 - match.capturedLength();
                continue;
            }
            if (word == ' ') {
                result.replace(match.capturedStart() + offset, capturedLength, "&nbsp;");
                offset += 6 - match.capturedLength();
                continue;
            }

            QString style = m_numberColor;// number

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
                  word.chop(1);
                  style = m_keyColor; // key
                } else {
                  style = m_strColor; // string
                }
            } else if (subMatch2.hasMatch()) {
                style = m_boolColor; // bool
            }

            QString newWord = "<font color=" + style + ">" + word + "</font>";
            result.replace(match.capturedStart() + offset, capturedLength, newWord);
            offset += newWord.length() - capturedLength;
        }

        return result.toStdString();
    }


private:
    QTextDocument* m_pParent = nullptr;
    QRegularExpression m_pattern;

    QTextCharFormat m_keyFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_boolFormat;
    QString m_keyColor;
    QString m_boolColor;
    QString m_strColor;
    QString m_numberColor;
};
