#!/usr/bin/env python3
# ============================================================================
#  Сверка адресов из src/Gta/GtaAddresses.h с исходниками gta-reversed.
#
#  Как это работает:
#   * берёт все адреса из GtaAddresses.h (константы вида `0x...`);
#   * ищет их в gta-reversed/source/game_sa/**/*.cpp (RH_ScopedInstall и
#     аннотации `// 0x...`) и *.h (StaticRef);
#   * выводит, какие адреса подтверждены, а какие нет.
#
#  Использование:
#    python3 tools/verify_addresses.py [путь_к_gta-reversed]
#    (по умолчанию ./gta-reversed — клонируется run_stalker.bat)
# ============================================================================
import re
import sys
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ADDR_H = ROOT / "src" / "Gta" / "GtaAddresses.h"


def parse_addresses():
    """Имя константы -> 0xадрес из GtaAddresses.h"""
    out = {}
    for m in re.finditer(r"constexpr\s+uint32_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+)", ADDR_H.read_text(encoding="utf-8")):
        out[m.group(1)] = int(m.group(2), 16)
    return out


def collect_game_addresses(re_root: Path):
    """Множество всех адресов, упомянутых в gta-reversed"""
    addrs = set()
    text_by_file = {}
    for pattern in ("**/*.cpp", "**/*.h"):
        for f in (re_root / "source").rglob(pattern.name if False else "*"):
            if not f.is_file() or f.suffix not in (".cpp", ".h"):
                continue
            try:
                text = f.read_text(encoding="utf-8", errors="replace")
            except Exception:
                continue
            text_by_file[str(f)] = text
            for m in re.finditer(r"\b0x[0-9A-Fa-f]{5,6}\b", text):
                try:
                    addrs.add(int(m.group(0), 16))
                except ValueError:
                    pass
    return addrs, text_by_file


def main():
    re_root = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "gta-reversed"
    if not re_root.exists():
        print(f"gta-reversed не найден: {re_root}")
        print("Склонируйте: git clone --depth 1 "
              "https://github.com/gta-reversed/gta-reversed gta-reversed")
        sys.exit(1)

    ours = parse_addresses()
    theirs, texts = collect_game_addresses(re_root)

    verified, missing = [], []
    for name, addr in sorted(ours.items()):
        if addr in theirs:
            verified.append((name, addr))
        else:
            missing.append((name, addr))

    print(f"Адресов в GtaAddresses.h : {len(ours)}")
    print(f"Подтверждено в gta-reversed: {len(verified)} ({100*len(verified)//max(1,len(ours))}%)")
    if missing:
        print(f"\nНЕ найдены в исходниках gta-reversed (проверить вручную!):")
        for name, addr in missing:
            print(f"  0x{addr:08X}  {name}")

    # RW-функции берутся из plugin-sdk (RenderWare.cpp) — отдельная проверка
    rw = {"RwStreamOpen": 0x7ECEF0, "RwStreamClose": 0x7ECE20}
    print("\nRenderWare (из plugin-sdk, RenderWare.cpp):")
    for name, addr in rw.items():
        print(f"  0x{addr:08X}  {name} — сверить с "
              "https://github.com/DK22Pac/plugin-sdk/blob/master/plugin_sa/game_sa/RenderWare.cpp")

    sys.exit(0 if not missing else 2)


if __name__ == "__main__":
    main()
