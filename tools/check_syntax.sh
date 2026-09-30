#!/usr/bin/env bash
# ============================================================================
#  Проверка компилируемости исходников мода на любой платформе (Linux/macOS).
#  Обёртки игры заменяются заглушками (STALKER_SYNTAX_CHECK), Windows-код
#  исключается — проверяются типы, вызовы, включения и логика мода.
#
#  Использование:  tools/check_syntax.sh
# ============================================================================
set -u
cd "$(dirname "$0")/.."

CXX="${CXX:-g++}"
if ! command -v "$CXX" >/dev/null 2>&1; then
    echo "Компилятор $CXX не найден"; exit 1
fi

OUT=build/syntax-check
mkdir -p "$OUT"

FAILED=0
SRCS=$(find src -name '*.cpp' | sort)
for src in $SRCS; do
    obj="$OUT/$(echo "$src" | tr '/' '_').o"
    if "$CXX" -std=c++20 -c "$src" -o "$obj" \
        -DSTALKER_SYNTAX_CHECK \
        -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
        -Isrc 2>"$OUT/$(basename "$src").log"; then
        echo "  OK   $src"
    else
        echo "  FAIL $src (см. $OUT/$(basename "$src").log)"
        cat "$OUT/$(basename "$src").log" | head -40
        FAILED=1
    fi
done

if [ "$FAILED" -eq 0 ]; then
    echo "Синтаксис: все файлы скомпилированы ($(echo "$SRCS" | wc -l) шт.)"
else
    echo "ЕСТЬ ОШИБКИ"
fi
exit $FAILED
