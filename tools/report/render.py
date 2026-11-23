import json, html, datetime, sys

src = sys.argv[1] if len(sys.argv) > 1 else "report.json"
dst = sys.argv[2] if len(sys.argv) > 2 else "report.html"
data = json.load(open(src))
day = data["day"]; method = data["method"]; states = data["states"]; month = data["month"]
events = data.get("events", {}); offsets = data.get("iqamaOffsets", {}); mosque = data.get("mosque")

AR = {"fajr": "الفجر", "sunrise": "الشروق", "dhuhr": "الظهر", "asr": "العصر",
      "maghrib": "المغرب", "isha": "العشاء"}
STATE_COL = {"waitingAdhan": "#33414d", "waitingIqama": "#c9962f",
             "inPrayer": "#1f6f68", "idle": "#151a20"}
STATE_NAME = {"waitingAdhan": "counting to adhan", "waitingIqama": "counting to iqama",
              "inPrayer": "prayer in progress", "idle": "nothing to count"}
EVENT_AR = {"ramadan": "رمضان", "lastten": "العشر الأواخر", "qadr": "ليلة القدر",
            "fitr": "عيد الفطر", "arafah": "يوم عرفة", "adha": "عيد الأضحى",
            "newyear": "رأس السنة", "ashura": "عاشوراء", "mawlid": "المولد"}


def iso(d):
    y, m, dd = d.split("-")
    return dd + "/" + m + "/" + y


def samp(hh, mm):
    target = hh * 60 + mm
    return min(states, key=lambda s: abs(s["minute"] - target))


def countdown(seconds):
    seconds = abs(int(seconds))
    h, r = divmod(seconds, 3600)
    m, s = divmod(r, 60)
    return ("%02d:%02d:%02d" % (h, m, s)) if h else ("%02d:%02d" % (m, s))


W, H = 1180, 120
step = states[1]["minute"] - states[0]["minute"]

bars = []
for s in states:
    x = s["minute"] / 1440.0 * W
    w = step / 1440.0 * W + 0.6
    bars.append('<rect x="%.2f" y="26" width="%.2f" height="46" fill="%s"/>' % (x, w, STATE_COL[s["state"]]))

ticks = []
for p in day["prayers"]:
    for field, colour, dy, hh in (("adhan", "#f0dca8", 72, 14), ("iqama", "#7fa8a3", 88, 8)):
        t = p.get(field) or ""
        if not t:
            continue
        h, m = t.split(":")
        x = (int(h) * 60 + int(m)) / 1440.0 * W
        ticks.append('<line x1="%.2f" y1="%d" x2="%.2f" y2="%d" stroke="%s" stroke-width="2"/>' % (x, dy, x, dy + hh, colour))
        if field == "adhan":
            ticks.append('<text x="%.2f" y="%d" fill="#f0dca8" font-size="13" text-anchor="middle" font-family="monospace">%s</text>' % (x, dy + 30, t))
            ticks.append('<text x="%.2f" y="%d" fill="#8b969e" font-size="12" text-anchor="middle">%s</text>' % (x, dy + 48, AR.get(p["key"], p["key"])))

hours = ""
for h in range(0, 23, 2):
    x = h / 24.0 * W
    hours += '<line x1="%.1f" y1="22" x2="%.1f" y2="72" stroke="#20262d"/>' % (x, x)
    hours += '<text x="%.1f" y="16" fill="#5c666d" font-size="11" text-anchor="middle">%02d</text>' % (x, h)

cards = ""
for hh, mm in [(5, 0), (5, 52), (6, 5), (12, 0), (19, 40), (22, 0)]:
    s = samp(hh, mm)
    p = next((x for x in day["prayers"] if x["key"] == s["key"]), None)
    if s["state"] == "waitingAdhan":
        big = AR.get(s["key"], s["key"]); sub = "adhan in"; cd = countdown(s["seconds"])
        det = ("at " + p["adhan"]) if p else ""
    elif s["state"] == "waitingIqama":
        big = "الإقامة"; sub = AR.get(s["key"], s["key"]) + " · iqama in"
        cd = countdown(s["seconds"]); det = ("at " + p["iqama"]) if p else ""
    elif s["state"] == "inPrayer":
        big = "الصلاة"; sub = AR.get(s["key"], s["key"]) + " · since iqama"
        cd = "+" + countdown(s["seconds"]); det = ""
    else:
        big = "—"; sub = "no data"; cd = ""; det = ""
    cards += ('<div class="card"><div class="t">%s</div>'
              '<div class="big" lang="ar" dir="rtl">%s</div><div class="sub">%s</div>'
              '<div class="cd">%s</div><div class="det">%s</div></div>') % (s["at"], big, sub, cd, det)

first = datetime.date.fromisoformat(month[0]["date"])
lead = first.weekday() % 7
cells = '<div class="cell empty"></div>' * lead
for d in month:
    cls = "cell"
    if d["friday"]:
        cls += " fri"
    if d.get("event"):
        cls += " ev"
    badge = ('<div class="badge" lang="ar" dir="rtl">%s</div>' % EVENT_AR.get(d["event"], d["event"])) if d.get("event") else ""
    cells += ('<div class="%s"><div class="g">%d</div><div class="h">%s</div>%s</div>'
              % (cls, d["dayOfMonth"], d["hijri"], badge))

prow = ""
for p in day["prayers"]:
    off = offsets.get(p["key"])
    prow += ('<tr><td lang="ar" dir="rtl">%s</td><td class="mono">%s</td><td class="mono">%s</td>'
             '<td class="mute">%s</td></tr>') % (AR.get(p["key"], p["key"]), p["adhan"] or "—",
                                                 p["iqama"] or "—", "—" if off is None else off)

tbl = ""
for s in states[::6]:
    tbl += ('<tr><td class="mono">%s</td><td><span class="pill" style="background:%s">%s</span></td>'
            '<td lang="ar" dir="rtl">%s</td><td class="mono">%s</td></tr>') % (
        s["at"], STATE_COL[s["state"]], STATE_NAME[s["state"]],
        AR.get(s["key"], s["key"]) if s["key"] else "—",
        countdown(s["seconds"]) if s["state"] != "idle" else "")

adjust = ", ".join("%s %+d" % (k, v) for k, v in method.get("adjustments", {}).items()) or "none"

doc = """<!doctype html><html><head><meta charset="utf-8"><style>
@page { size: 1300px 1900px; margin: 0; }
* { box-sizing: border-box; }
body { margin:0; padding:34px 40px 40px; width:1300px; background:#0d1319; color:#e9e4d8;
  font-family: "DejaVu Sans", system-ui, sans-serif; }
h1 { font-size: 26px; margin:0 0 4px; font-weight:600; }
h1 span { color:#c9962f; }
.lead { color:#8b969e; font-size:14px; margin-bottom:22px; }
.panel { background:#121a21; border:1px solid #1e2831; border-radius:14px; padding:20px 22px; margin-bottom:18px; }
.panel h2 { font-size:13px; text-transform:uppercase; letter-spacing:1.6px; color:#8b969e; margin:0 0 14px; font-weight:600; }
.chips span { display:inline-block; padding:4px 10px; border-radius:999px; font-size:12px; margin:0 6px 6px 0;
  border:1px solid #2a3742; color:#c3ccd2; }
.chips b { color:#e9e4d8; font-weight:600; }
.cards { display:flex; gap:12px; }
.card { flex:1; background:#10171d; border:1px solid #1e2831; border-radius:12px; padding:12px 10px; text-align:center; }
.card .t { font:11px monospace; color:#5c666d; }
.card .big { font-size:24px; margin:6px 0 2px; color:#f0dca8; font-family:"Noto Naskh Arabic", serif; }
.card .sub { font-size:10.5px; color:#8b969e; }
.card .cd { font:600 15px monospace; color:#fff; margin-top:6px; }
.card .det { font:11px monospace; color:#5c666d; }
table { width:100%%; border-collapse:collapse; font-size:12.5px; }
td, th { padding:5px 6px; border-bottom:1px solid #1a232b; text-align:left; }
th { color:#7b868e; font-weight:600; font-size:11px; text-transform:uppercase; letter-spacing:1px; }
.mono { font-family:monospace; }
.mute { color:#6d777e; }
.pill { padding:2px 8px; border-radius:999px; font-size:11px; color:#fff; }
.grid { display:grid; grid-template-columns:repeat(7, 1fr); gap:6px; }
.cell { min-height:62px; border:1px solid #1c252d; border-radius:8px; padding:5px 7px; position:relative; }
.cell.empty { border:none; }
.cell .g { font-size:13px; color:#e9e4d8; }
.cell .h { font:10.5px monospace; color:#6d777e; }
.cell.fri { border-color:#2e3d49; background:#141d25; }
.cell.ev { border-color:#c9962f; background:#1d1a12; }
.badge { position:absolute; bottom:5px; right:6px; font-size:10.5px; color:#f0dca8; font-family:"Noto Naskh Arabic", serif; }
.legend span { display:inline-block; margin-right:16px; font-size:12px; color:#b8c1c8; }
.sw { display:inline-block; width:11px; height:11px; border-radius:3px; margin-right:5px; vertical-align:-1px; }
.foot { color:#5c666d; font-size:11.5px; line-height:1.55; }
</style></head><body>
<h1>islamic-times <span>· behaviour report</span></h1>
<div class="lead">__ISO__ · Hijri __HIJRI__ · __METHOD__ · every number below produced by the library's own output</div>

<div class="panel"><h2>Where the numbers came from</h2><div class="chips">
<span>day <b>__ISO__</b></span>
<span>hijri <b>__HIJRI__</b></span>
<span>method <b>__MID__</b> · fajr __FAJR__° · isha __ISHA__° · asr __ASR__ · high-lat __HL__</span>
<span>adjustments <b>__ADJ__</b></span>
<span>defaults <b>__DEF__</b></span>
<span>iqama <b>__IQSRC__</b></span>
</div></div>

<div class="panel"><h2>The day, as the library walks it</h2>
<svg width="__W__" height="__H__" viewBox="0 0 __W__ __H__">__HOURS__</svg>
<div class="legend" style="margin-top:14px">
  <span><i class="sw" style="background:__C1__"></i>counting to adhan</span>
  <span><i class="sw" style="background:__C2__"></i>counting to iqama</span>
  <span><i class="sw" style="background:__C3__"></i>prayer in progress (20 min)</span>
  <span><i class="sw" style="background:__C4__"></i>nothing to count</span>
  <span>tall tick = adhan, short = iqama</span>
</div></div>

<div class="panel"><h2>What a tile would show at six moments</h2><div class="cards">__CARDS__</div></div>

<div class="panel"><h2>The five prayers, adhan and iqama</h2>
<table><tr><th>prayer</th><th>adhan</th><th>iqama</th><th>offset (min)</th></tr>__PROW__</table></div>

<div class="panel"><h2>Sampled every 30 minutes</h2>
<table><tr><th>time</th><th>state</th><th>counting to</th><th>remaining</th></tr>__TBL__</table></div>

<div class="panel"><h2>The month __MONTH__ — Hijri days and the events in it</h2>
<div class="grid">__CELLS__</div>
<div style="margin-top:10px" class="foot">Gold = an Islamic event starts that day. Shaded = Friday. Gregorian day big, Hijri day as month/day small.</div></div>

<div class="panel"><h2>Events, as the library resolves them</h2><div class="chips">
<span>current <b>__ECUR__</b></span>
<span>next <b>__ENEXT__</b></span>
<span>days to next <b>__EDAYS__</b></span>
</div></div>

<div class="panel"><h2>How to read this</h2><div class="foot">
Every band, tick and countdown above comes from <span class="mono">islamic-times</span> — the same C++ the widgets will call, driven by the same fixtures the tests use.
The day is <b>1 Ramadan 1448</b>: calculated times from Iqamah's year payload for Tunis, with iqama offsets taken from a real mosque day (Grande Mosquée de Paris: +8, +8, +8, 0, +8). That is the "the mosque published today, we extrapolate" path, so the report also shows what a predicted iqama looks like.
</div></div>
</body></html>"""

repl = {
    "__ISO__": iso(day["date"]), "__HIJRI__": day["hijri"],
    "__METHOD__": html.escape(method["name"] or method["id"]),
    "__MID__": html.escape(method["id"] or "—"), "__FAJR__": str(method["fajrAngle"]),
    "__ISHA__": str(method["ishaAngle"]), "__ASR__": method["asr"] or "—",
    "__HL__": method["highLat"] or "—", "__ADJ__": adjust,
    "__DEF__": html.escape(method.get("defaultsVersion") or "—"),
    "__IQSRC__": ("offsets from " + mosque["date"] + " (predicted)") if mosque else "no mosque",
    "__W__": str(W), "__H__": str(H), "__HOURS__": hours + "".join(bars) + "".join(ticks),
    "__C1__": STATE_COL["waitingAdhan"], "__C2__": STATE_COL["waitingIqama"],
    "__C3__": STATE_COL["inPrayer"], "__C4__": STATE_COL["idle"],
    "__CARDS__": cards, "__PROW__": prow, "__TBL__": tbl,
    "__MONTH__": day["date"][:7], "__CELLS__": cells,
    "__ECUR__": events.get("current", {}).get("id", "—"),
    "__ENEXT__": events.get("next", {}).get("id", "—"),
    "__EDAYS__": str(events.get("next", {}).get("daysAway", "—")),
}
for k, v in repl.items():
    doc = doc.replace(k, v)

open(dst, "w").write(doc)
print("html bytes:", len(doc))
