#include "providers/IqamahParser.h"

#include "core/DayRules.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTime>

namespace islamic {

namespace {

void fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

// "06:42" (24h) is the canonical field; "06:42 AM" appears in the text
// rendering, so accept both rather than depending on which one a payload keeps.
QTime parseTime(const QJsonValue &value)
{
    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        const QTime t = parseTime(object.value(QStringLiteral("time")));
        if (t.isValid())
            return t;
        return parseTime(object.value(QStringLiteral("text")));
    }
    if (!value.isString())
        return {};
    const QString text = value.toString().trimmed();
    if (text.isEmpty())
        return {};
    QTime t = QTime::fromString(text, QStringLiteral("HH:mm"));
    if (t.isValid())
        return t;
    t = QTime::fromString(text, QStringLiteral("hh:mm AP"));
    if (t.isValid())
        return t;
    return QTime::fromString(text, QStringLiteral("H:mm"));
}

QDate parseDate(const QJsonValue &value)
{
    if (!value.isString())
        return {};
    return QDate::fromString(value.toString().trimmed(), Qt::ISODate);
}

HijriDate parseHijri(const QJsonValue &value)
{
    if (!value.isString())
        return {};
    const QStringList parts = value.toString().trimmed().split(QLatin1Char('-'));
    if (parts.size() != 3)
        return {};
    HijriDate hijri;
    hijri.year = parts.at(0).toInt();
    hijri.month = parts.at(1).toInt();
    hijri.day = parts.at(2).toInt();
    return hijri.isValid() ? hijri : HijriDate{};
}

Provenance parseProvenance(const QJsonObject &payload, const QDate &dayDate)
{
    Provenance provenance;
    const QJsonObject source = payload.value(QStringLiteral("source")).toObject();
    const QString kind = source.value(QStringLiteral("kind")).toString();
    if (kind == QLatin1String("calculated"))
        provenance.source = TimesSource::IqamahCalculated;
    else if (!kind.isEmpty())
        provenance.source = TimesSource::IqamahMosque;

    provenance.publishedFor = dayDate;
    provenance.stale = payload.value(QStringLiteral("stale")).toBool(false)
                    || payload.value(QStringLiteral("status")).toString() == QLatin1String("stale");

    const QString asOf = payload.value(QStringLiteral("as_of")).toString();
    if (!asOf.isEmpty()) {
        const QDate date = QDate::fromString(asOf, Qt::ISODate);
        if (date.isValid())
            provenance.asOf = QDateTime(date, QTime(0, 0));
    }
    const QString extracted = payload.value(QStringLiteral("extracted_at")).toString();
    if (!extracted.isEmpty()) {
        const QDateTime stamp = QDateTime::fromString(extracted, Qt::ISODate);
        if (stamp.isValid())
            provenance.asOf = stamp;
    }
    return provenance;
}

// "fajr": 18 is an angle in degrees; "fajr": "90 min" is an interval. Both are
// kept, in their own field, because they mean different things.
void readAngleOrMinutes(const QJsonValue &value, double *angle, int *minutes)
{
    if (value.isDouble()) {
        *angle = value.toDouble();
        return;
    }
    if (value.isString()) {
        const QString text = value.toString().trimmed().toLower();
        if (text.endsWith(QLatin1String("min"))) {
            *minutes = text.split(QLatin1Char(' ')).first().toInt();
            return;
        }
        bool ok = false;
        const double degrees = text.toDouble(&ok);
        if (ok)
            *angle = degrees;
    }
}

MethodPreset parseMethod(const QJsonObject &payload)
{
    MethodPreset method;
    const QJsonObject object = payload.value(QStringLiteral("method")).toObject();
    method.id = object.value(QStringLiteral("id")).toString();
    method.name = object.value(QStringLiteral("name")).toString();
    readAngleOrMinutes(object.value(QStringLiteral("fajr")), &method.fajrAngle, &method.fajrMinutes);
    readAngleOrMinutes(object.value(QStringLiteral("isha")), &method.ishaAngle, &method.ishaMinutes);
    method.asr = object.value(QStringLiteral("asr")).toString();
    method.highLat = object.value(QStringLiteral("highlat")).toString();

    const QJsonObject adjustments = object.value(QStringLiteral("adjustments")).toObject();
    for (auto it = adjustments.constBegin(); it != adjustments.constEnd(); ++it)
        method.adjustments.insert(it.key(), it.value().toInt());

    method.defaultsVersion = payload.value(QStringLiteral("defaults_version")).toString();
    return method;
}

Prayer prayerFor(const QJsonObject &prayers, PrayerKey key)
{
    Prayer prayer;
    prayer.key = key;
    const QJsonObject entry = prayers.value(prayerKeyId(key)).toObject();
    if (entry.isEmpty())
        return prayer;

    const QJsonObject iqamah = entry.value(QStringLiteral("iqamah")).toObject();
    prayer.iqama = parseTime(iqamah.value(QStringLiteral("time")).isUndefined()
                             ? iqamah
                             : iqamah.value(QStringLiteral("time")));

    const QJsonObject azaan = entry.value(QStringLiteral("azaan")).toObject();
    const QJsonObject mosque = azaan.value(QStringLiteral("mosque")).toObject();
    prayer.adhan = parseTime(mosque.value(QStringLiteral("time")).isUndefined()
                             ? mosque
                             : mosque.value(QStringLiteral("time")));
    if (!prayer.adhan.isValid())
        prayer.adhan = parseTime(azaan.value(QStringLiteral("method")));

    return prayer;
}

} // namespace

bool IqamahParser::parseMosqueDay(const QByteArray &json, PrayerDay *day, QString *error)
{
    if (!day) {
        fail(error, QStringLiteral("null output"));
        return false;
    }
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        fail(error, QStringLiteral("invalid JSON: ") + parseError.errorString());
        return false;
    }
    const QJsonObject payload = document.object();
    const QJsonArray days = payload.value(QStringLiteral("days")).toArray();
    if (days.isEmpty()) {
        fail(error, QStringLiteral("no days in payload"));
        return false;
    }

    const QJsonObject entry = days.first().toObject();
    PrayerDay result;
    result.date = parseDate(entry.value(QStringLiteral("date")));
    result.hijri = parseHijri(entry.value(QStringLiteral("hijri")));
    if (!result.date.isValid()) {
        fail(error, QStringLiteral("day has no date"));
        return false;
    }

    const QJsonObject prayers = entry.value(QStringLiteral("prayers")).toObject();
    result.prayers.reserve(6);
    for (PrayerKey key : allPrayerKeys()) {
        if (key == PrayerKey::Sunrise) {
            Prayer sunrise;
            sunrise.key = key;
            const QJsonObject sunriseObject = entry.value(QStringLiteral("sunrise")).toObject();
            const QJsonObject mosque = sunriseObject.value(QStringLiteral("mosque")).toObject();
            sunrise.adhan = parseTime(mosque.value(QStringLiteral("time")).isUndefined()
                                      ? mosque
                                      : mosque.value(QStringLiteral("time")));
            if (!sunrise.adhan.isValid())
                sunrise.adhan = parseTime(sunriseObject.value(QStringLiteral("method")));
            result.prayers.append(sunrise);
        } else {
            result.prayers.append(prayerFor(prayers, key));
        }
    }

    const QJsonArray jumuah = entry.value(QStringLiteral("jumuah")).toArray();
    for (const QJsonValue &value : jumuah) {
        const QJsonObject service = value.toObject();
        Jumuah parsed;
        parsed.location = service.value(QStringLiteral("location")).toString();
        parsed.khutbah = parseTime(service.value(QStringLiteral("khutbah")));
        if (!parsed.khutbah.isValid())
            parsed.khutbah = parseTime(service.value(QStringLiteral("prayer")));   // the service time
        parsed.iqama = parseTime(service.value(QStringLiteral("azaan")));
        result.jumuah.append(parsed);
    }

    result.provenance = parseProvenance(payload, result.date);
    if (DayRules::isFriday(result.date))
        DayRules::applyJumuah(result);

    *day = result;
    return true;
}

bool IqamahParser::parseYearTimes(const QByteArray &json,
                                  QVector<PrayerDay> *days,
                                  MethodPreset *method,
                                  QString *error)
{
    if (!days) {
        fail(error, QStringLiteral("null output"));
        return false;
    }
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        fail(error, QStringLiteral("invalid JSON: ") + parseError.errorString());
        return false;
    }
    const QJsonObject payload = document.object();
    const QJsonArray array = payload.value(QStringLiteral("days")).toArray();
    if (array.isEmpty()) {
        fail(error, QStringLiteral("no days in payload"));
        return false;
    }

    QVector<PrayerDay> parsedDays;
    parsedDays.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject entry = value.toObject();
        PrayerDay day;
        day.date = parseDate(entry.value(QStringLiteral("date")));
        day.hijri = parseHijri(entry.value(QStringLiteral("hijri")));
        if (!day.date.isValid())
            continue;

        for (PrayerKey key : allPrayerKeys()) {
            Prayer prayer;
            prayer.key = key;
            prayer.adhan = parseTime(entry.value(prayerKeyId(key)));
            day.prayers.append(prayer);
        }
        day.provenance = parseProvenance(payload, day.date);
        parsedDays.append(day);
    }
    if (parsedDays.isEmpty()) {
        fail(error, QStringLiteral("no parsable day"));
        return false;
    }

    *days = parsedDays;
    if (method)
        *method = parseMethod(payload);
    return true;
}

} // namespace islamic
