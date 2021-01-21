#pragma once

#include <QValidator>

class IP4Validator : public QValidator
{
public:
  IP4Validator(QObject* parent = 0);
  State validate(QString& input, int&) const;
};
