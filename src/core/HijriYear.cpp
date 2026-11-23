#include "core/HijriYear.h"

namespace islamic {

QString HijriYear::key(const HijriDate &h)
{
    return QString::number(h.year) + QLatin1Char('-') + QString::number(h.month)
         + QLatin1Char('-') + QString::number(h.day);
}

void HijriYear::add(const QDate &date, const HijriDate &hijri)
{
    if (!date.isValid() || !hijri.isValid())
        return;
    m_byDate.insert(date, hijri);
    m_byHijri.insert(key(hijri), date);
}

QDate HijriYear::firstDate() const
{
    if (m_byDate.isEmpty())
        return {};
    QDate first;
    for (auto it = m_byDate.cbegin(); it != m_byDate.cend(); ++it) {
        if (!first.isValid() || it.key() < first)
            first = it.key();
    }
    return first;
}

QDate HijriYear::lastDate() const
{
    if (m_byDate.isEmpty())
        return {};
    QDate last;
    for (auto it = m_byDate.cbegin(); it != m_byDate.cend(); ++it) {
        if (!last.isValid() || it.key() > last)
            last = it.key();
    }
    return last;
}

HijriDate HijriYear::hijriOf(const QDate &date) const
{
    return m_byDate.value(date, HijriDate{});
}

QDate HijriYear::dateOf(const HijriDate &hijri) const
{
    return m_byHijri.value(key(hijri));
}

} // namespace islamic
