#include "ip4validator.h"

IP4Validator::IP4Validator(QObject *parent)
    : QValidator(parent)
{}

QValidator::State IP4Validator::validate(QString &input, int &) const
{
    if (input.isEmpty()) {
        return Acceptable;
    }

    QStringList ipPort = input.split(":");
    if (ipPort.size() > 2) {
        return Invalid;
    }

    QStringList slist = ipPort[0].split(".");
    int number = slist.size();
    if (number > 4) {
        return Invalid;
    }

    bool emptyGroup = false;
    for (int i = 0; i < number; i++) {
        bool ok;
        if(slist[i].isEmpty()){
          emptyGroup = true;
          continue;
        }
        ushort value = slist[i].toUShort(&ok);
        if(!ok || value > 255) {
            return Invalid;
        }
    }

    if(number < 4 || emptyGroup) {
        return Intermediate;
    }

    if (ipPort.size() == 2) {
        if (ipPort[1].isEmpty()) {
            return Intermediate;
        }

        bool ok;
        ipPort[1].toUShort(&ok);
        if (!ok) {
            return Invalid;
        }
    }

    return Acceptable;
}
