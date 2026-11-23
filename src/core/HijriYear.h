#pragma once

#include "core/Types.h"

#include <QDate>
#include <QHash>
#include <QList>

namespace islamic {

// A year of (Gregorian day, Hijri day) pairs, exactly as the provider sends
// them. This is the whole reason the library does no Hijri arithmetic: every
// answer is a lookup, and a lookup cannot drift by a day.
class HijriYear
{
public:
    void add(const QDate &date, const HijriDate &hijri);

    bool isEmpty() const { return m_byDate.isEmpty(); }
    int size() const { return m_byDate.size(); }
    QDate firstDate() const;
    QDate lastDate() const;

    HijriDate hijriOf(const QDate &date) const;
    QDate dateOf(const HijriDate &hijri) const;
    bool covers(const QDate &date) const { return m_byDate.contains(date); }

private:
    static QString key(const HijriDate &hijri);

    QHash<QDate, HijriDate> m_byDate;
    QHash<QString, QDate> m_byHijri;
};

} // namespace islamic
