#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QStringList>

namespace islamic {

struct CacheEntry {
    QByteArray body;
    QDateTime fetchedAt;
    int httpStatus = 0;

    bool isValid() const { return !body.isEmpty(); }
    int ageSeconds(const QDateTime &now = QDateTime::currentDateTimeUtc()) const;
};

// Stores the provider's bytes exactly as they arrived. It never parses them and
// never rewrites them: a cached payload is evidence, not a convenience copy. That
// is what lets the model change without a cache migration, and what keeps us from
// ever holding a "repaired" version of their data on disk.
class PayloadCache
{
public:
    explicit PayloadCache(const QString &root = QString());

    QString root() const { return m_root; }
    void setRoot(const QString &root);

    bool read(const QString &key, CacheEntry *entry) const;
    bool write(const QString &key, const QByteArray &body,
               const QDateTime &fetchedAt = QDateTime::currentDateTimeUtc(),
               int httpStatus = 200);
    bool remove(const QString &key);
    bool contains(const QString &key) const;

    int pruneOlderThan(const QDateTime &cutoff);
    QStringList keys() const;
    qint64 totalSize() const;

    // Keys are the only thing that becomes a file name, so they are built here and
    // validated before use: no separators, no dots-dots, nothing that escapes root.
    static QString keyForYear(double lat, double lon, int year);
    static QString keyForMosqueDay(const QString &mosqueId, const QDate &date);
    static QString keyForNearby(double lat, double lon, int radiusKm);
    static bool isValidKey(const QString &key);

private:
    QString pathForKey(const QString &key) const;
    bool ensureRoot() const;

    QString m_root;
};

} // namespace islamic
