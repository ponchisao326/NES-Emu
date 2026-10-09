#!/bin/sh
# Compares a trace with nestest.log and reports the first line that differs
# usage: tests/compare_nestest.sh mine.log [roms/nestest.log]
mine="$1"
ref="${2:-roms/nestest.log}"
color=0
[ -t 1 ] && color=1

awk -v color="$color" '
# b with the characters that differ from a in red (only on a terminal)
function mark(a, b,    i, out, on) {
    out = ""
    on = 0
    for (i = 1; i <= length(b); i++) {
        if (color && substr(a, i, 1) != substr(b, i, 1) && !on) { out = out "\033[1;31m"; on = 1 }
        if (color && substr(a, i, 1) == substr(b, i, 1) && on) { out = out "\033[0m"; on = 0 }
        out = out substr(b, i, 1)
    }
    if (on) out = out "\033[0m"
    return out
}
NR == FNR { sub(/\r$/, ""); ref[FNR] = $0; next }
$0 != ref[FNR] {
    printf "%d lines identical\nline %d differs\n\n", FNR - 1, FNR
    if (FNR > 1) printf "%s  %-9s%s%s\n", (color ? "\033[2m" : ""), "#" (FNR - 1), prev, (color ? "\033[0m" : "")
    printf "  %-9s%s\n  %-9s%s\n", "nestest", mark($0, ref[FNR]), "mine", mark(ref[FNR], $0)
    bad = 1
    exit 1
}
{ prev = $0; lines = FNR }
END {
    if (!bad) printf "%s%d lines identical to nestest.log%s\n", (color ? "\033[1;32m" : ""), lines, (color ? "\033[0m" : "")
}' "$ref" "$mine"
