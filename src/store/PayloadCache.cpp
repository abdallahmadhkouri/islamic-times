#include "store/PayloadCache.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace islamic {

namespace {

// Two decimals is about a kilometre: two neighbours in the same town must share a
// cache entry, and a different town must not.
QString coordinate(double value)
{
    return QString::number(value, 'f', 2);
}

} // namespace

int CacheEntry::ageSeconds(const QDateTime &now) const
{
    if (!fetchedAt.isValid())
        return -1;
    return static_cast<int>(fetchedAt.secsTo(now));
}

PayloadCache::PayloadCache(const QString &root)
    : m_root(root.isEmpty()
                 ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + QStringLiteral("/iqamah")
                 : root)
{
}

void PayloadCache::setRoot(const QString &root)
{
    m_root = root;
}

bool PayloadCache::isValidKey(const QString &key)
{
    if (key.isEmpty() || key.size() > 120)
        return false;
    if (key.contains(QLatin1String("..")))
        return false;
    for (const QChar c : key) {
        const bool allowed = (c >= QLatin1Char('a') && c <= QLatin1Char('z'))
                             || (c >= QLatin1Char('A') && c <= QLatin1Char('Z'))
                             || (c >= QLatin1Char('0') && c <= QLatin1Char('9'))
                             || c == QLatin1Char('-') || c == QLatin1Char('_') || c == QLatin1Char('.');
        if (!allowed)
            return false;
    }
    return true;
}

QString PayloadCache::keyForYear(double lat, double lon, int year)
{
    return QStringLiteral("year-%1-%2-%3").arg(coordinate(lat), coordinate(lon)).arg(year);
}

QString PayloadCache::keyForMosqueDay(const QString &mosqueId, const QDate &date)
{
    return QStringLiteral("mosque-%1-%2").arg(mosqueId, date.toString(Qt::ISODate));
}

QString PayloadCache::keyForNearby(double lat, double lon, int radiusKm)
{
    return QStringLiteral("near-%1-%2-%3").arg(coordinate(lat), coordinate(lon)).arg(radiusKm);
}

QString PayloadCache::pathForKey(const QString &key) const
{
    return m_root + QLatin1Char('/') + key + QStringLiteral(".json");
}

bool PayloadCache::ensureRoot() const
{
    return QDir().mkpath(m_root);
}

bool PayloadCache::read(const QString &key, CacheEntry *entry) const
{
    if (!entry || !isValidKey(key))
        return false;

    QFile file(pathForKey(key));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QJsonObject envelope = QJsonDocument::fromJson(file.readAll()).object();
    const QString body = envelope.value(QStringLiteral("body")).toString();
    if (body.isEmpty())
        return false;

    entry->body = body.toUtf8();
    entry->fetchedAt = QDateTime::fromString(envelope.value(QStringLiteral("fetched_at")).toString(),
                                             Qt::ISODate);
    entry->httpStatus = envelope.value(QStringLiteral("http_status")).toInt();
    return true;
}

bool PayloadCache::write(const QString &key, const QByteArray &body,
                         const QDateTime &fetchedAt, int httpStatus)
{
    if (!isValidKey(key) || body.isEmpty() || !ensureRoot())
        return false;

    QJsonObject envelope;
    envelope.insert(QStringLiteral("key"), key);
    envelope.insert(QStringLiteral("fetched_at"), fetchedAt.toUTC().toString(Qt::ISODate));
    envelope.insert(QStringLiteral("http_status"), httpStatus);
    envelope.insert(QStringLiteral("body"), QString::fromUtf8(body));

    // QSaveFile writes beside the target and renames: a crash mid-write leaves the
    // previous file intact instead of a truncated one.
    QSaveFile file(pathForKey(key));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
    return file.commit();
}

bool PayloadCache::remove(const QString &key)
{
    if (!isValidKey(key))
        return false;
    return QFile::remove(pathForKey(key));
}

bool PayloadCache::contains(const QString &key) const
{
    return isValidKey(key) && QFile::exists(pathForKey(key));
}

int PayloadCache::pruneOlderThan(const QDateTime &cutoff)
{
    int removed = 0;
    for (const QString &key : keys()) {
        CacheEntry entry;
        if (!read(key, &entry)) {
            remove(key);              // unreadable is worse than absent
            ++removed;
            continue;
        }
        if (!entry.fetchedAt.isValid() || entry.fetchedAt < cutoff.toUTC()) {
            if (remove(key))
                ++removed;
        }
    }
    return removed;
}

QStringList PayloadCache::keys() const
{
    QStringList result;
    const QStringList files = QDir(m_root).entryList({ QStringLiteral("*.json") }, QDir::Files);
    for (const QString &file : files)
        result.append(file.chopped(5));   // drop ".json"
    return result;
}

qint64 PayloadCache::totalSize() const
{
    qint64 total = 0;
    const QStringList files = QDir(m_root).entryList({ QStringLiteral("*.json") }, QDir::Files);
    for (const QString &file : files)
        total += QFileInfo(m_root + QLatin1Char('/') + file).size();
    return total;
}

} // namespace islamic
