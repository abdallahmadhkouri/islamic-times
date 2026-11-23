pragma Singleton
import QtQuick

// Every colour, size and Arabic string the widgets use lives here, so the six of
// them cannot drift apart. Arabic is QML's job; the C++ library only sends ids.
QtObject {
    readonly property color bg0: "#070B12"
    readonly property color bg1: "#101B2B"
    readonly property color ink: "#F4EEE1"
    readonly property color inkDim: "#A9B2BE"
    readonly property color inkFaint: "#6C7787"
    readonly property color gold: "#DDB36B"
    readonly property color goldSoft: "#8E7440"
    readonly property color amber: "#E0A84A"
    readonly property color teal: "#3E8F86"
    readonly property color line: "#26DDB36B"

    readonly property string display: "Amiri"
    readonly property string ui: "Tajawal"

    readonly property var prayerNames: ({
        "fajr": "الفجر", "sunrise": "الشروق", "dhuhr": "الظهر",
        "asr": "العصر", "maghrib": "المغرب", "isha": "العشاء", "jumuah": "الجمعة"
    })

    readonly property var monthNames: ({
        1: "محرم", 2: "صفر", 3: "ربيع الأول", 4: "ربيع الثاني",
        5: "جمادى الأولى", 6: "جمادى الثانية", 7: "رجب", 8: "شعبان",
        9: "رمضان", 10: "شوال", 11: "ذو القعدة", 12: "ذو الحجة"
    })

    readonly property var eventNames: ({
        "ramadan": "رمضان", "lastten": "العشر الأواخر", "qadr": "ليلة القدر",
        "fitr": "عيد الفطر", "arafah": "يوم عرفة", "adha": "عيد الأضحى",
        "newyear": "رأس السنة", "ashura": "عاشوراء", "mawlid": "المولد النبوي"
    })

    readonly property var stateNames: ({
        "waitingAdhan": "إلى الأذان", "waitingIqama": "إلى الإقامة",
        "inPrayer": "الصلاة", "idle": "—"
    })

    function arabicDigits(value) {
        var map = ["٠", "١", "٢", "٣", "٤", "٥", "٦", "٧", "٨", "٩"]
        return String(value).replace(/[0-9]/g, function (d) { return map[d] })
    }

    // Western digits on purpose: at three metres 05:36 is unambiguous and ٠٥:٣٦
    // is not — the reference app in Saif's hand uses Western digits too.
    function num(value) { return String(value) }

    function time(text) {
        return text && text.length ? text : "—"
    }

    function pad(n) { return n < 10 ? "0" + n : "" + n }

    // Seconds as the tile shows them: minutes while it is close, hours when it is
    // not, because "٦ س ٣٥ د" is more honest at three metres than "06:35:12".
    function countdown(seconds) {
        seconds = Math.max(0, Math.round(seconds))
        var h = Math.floor(seconds / 3600)
        var m = Math.floor((seconds % 3600) / 60)
        var s = seconds % 60
        var out
        if (h > 0)
            out = h + " س " + pad(m) + " د"
        else
            out = pad(m) + ":" + pad(s)
        return out
    }

    function days(n) {
        if (n === 0) return "اليوم"
        if (n === 1) return "غدًا"
        if (n === 2) return "بعد يومين"
        if (n <= 10) return "بعد " + n + " أيام"
        return "بعد " + n + " يومًا"
    }
}
