// it-smoke — the T2 check: the real provider, the real TLS stack, no fakes.
//
//   it-smoke --lat 34.55 --lon 9.48 [--radius 10] [--year 2026] [--mosque <id>]

#include "net/QNetworkTransport.h"
#include "providers/IqamahClient.h"
#include "providers/IqamahParser.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

using namespace islamic;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption latOption(QStringLiteral("lat"), QStringLiteral("latitude"), QStringLiteral("deg"), QStringLiteral("34.55"));
    QCommandLineOption lonOption(QStringLiteral("lon"), QStringLiteral("longitude"), QStringLiteral("deg"), QStringLiteral("9.48"));
    QCommandLineOption radiusOption(QStringLiteral("radius"), QStringLiteral("km"), QStringLiteral("km"), QStringLiteral("10"));
    QCommandLineOption yearOption(QStringLiteral("year"), QStringLiteral("gregorian year"), QStringLiteral("year"), QStringLiteral("2026"));
    QCommandLineOption mosqueOption(QStringLiteral("mosque"), QStringLiteral("mosque id to fetch"), QStringLiteral("id"));
    parser.addOptions({ latOption, lonOption, radiusOption, yearOption, mosqueOption });
    parser.process(app);

    QNetworkTransport transport;
    IqamahClient client(&transport);
    QTextStream out(stdout);

    const double lat = parser.value(latOption).toDouble();
    const double lon = parser.value(lonOption).toDouble();
    const int radius = parser.value(radiusOption).toInt();

    QString error;
    QVector<MosqueInfo> mosques;
    const ClientError near = client.fetchNearby(lat, lon, radius, &mosques, nullptr, &error);
    out << "nearby(" << radius << "km): " << (near == ClientError::None ? "ok" : qPrintable(error))
        << " — " << mosques.size() << " mosque(s)\n";
    for (const MosqueInfo &mosque : mosques) {
        out << "  " << mosque.name << " · " << mosque.distanceKm << " km · "
            << mosque.city << " · has_source=" << (mosque.hasSource ? "true" : "false") << "\n";
    }

    QByteArray body;
    const ClientError yearResult = client.fetchYear(lat, lon, parser.value(yearOption).toInt(), &body, &error);
    out << "year: " << (yearResult == ClientError::None ? "ok" : qPrintable(error)) << "\n";
    if (yearResult == ClientError::None) {
        QVector<PrayerDay> days;
        MethodPreset method;
        if (IqamahParser::parseYearTimes(body, &days, &method, &error)) {
            out << "  " << days.size() << " days, method " << method.id << "\n";
            if (!days.isEmpty()) {
                const PrayerDay &day = days.first();
                out << "  " << day.date.toString(Qt::ISODate) << " (" << day.hijri.year << "-"
                    << day.hijri.month << "-" << day.hijri.day << "):";
                for (const Prayer &prayer : day.prayers)
                    out << " " << prayerKeyId(prayer.key) << " " << prayer.adhan.toString("HH:mm");
                out << "\n";
            }
        } else {
            out << "  parse failed: " << error << "\n";
        }
    }

    if (parser.isSet(mosqueOption)) {
        QByteArray mosqueBody;
        const ClientError mosqueResult = client.fetchMosqueDay(parser.value(mosqueOption), &mosqueBody, &error);
        out << "mosque day: " << (mosqueResult == ClientError::None ? "ok" : qPrintable(error)) << "\n";
        if (mosqueResult == ClientError::None) {
            PrayerDay day;
            if (IqamahParser::parseMosqueDay(mosqueBody, &day, &error)) {
                out << "  status=" << int(day.provenance.status)
                    << " iqama=" << (day.prayer(PrayerKey::Fajr)->hasIqama() ? "present" : "absent") << "\n";
            } else {
                out << "  parse failed: " << error << "\n";
            }
        }
    }

    out << "requests: " << client.requestCount() << "\n";
    return 0;
}
