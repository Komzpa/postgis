#!/usr/bin/env bash
#
# Minimal regression test for PostGIS Trac #5645.
# Ensures translated HTML keeps operators like "=>" intact by building
# chunked manuals and checking for spurious whitespace around "&gt;".
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
TOP_DIR=$(cd "${SCRIPT_DIR}/../.." && pwd)
DOC_DIR="${TOP_DIR}/doc"

if [[ ! -f "${DOC_DIR}/Makefile" ]]; then
  echo "SKIP: documentation Makefile missing (run ./configure first)." >&2
  exit 0
fi

if grep -q '@[A-Z][A-Z0-9_]*@' "${DOC_DIR}/Makefile"; then
  echo "SKIP: documentation Makefile still contains autoconf placeholders." >&2
  exit 0
fi

if ! command -v xsltproc >/dev/null 2>&1; then
  echo "SKIP: xsltproc not available; documentation HTML cannot be generated." >&2
  exit 0
fi

make -C "${DOC_DIR}" html DOCSUFFIX=-en >/dev/null
make -C "${DOC_DIR}" html DOCSUFFIX=-zh_Hans >/dev/null
make -C "${DOC_DIR}" chunked-html DOCSUFFIX=-zh_Hans >/dev/null

TARGET=$(find "${DOC_DIR}" -path '*zh_Hans*/RT_ST_Clip.html' -print -quit)
if [[ -z "${TARGET}" ]]; then
  echo "Unable to locate zh_Hans RT_ST_Clip.html after building documentation." >&2
  exit 1
fi

grep -q 'scalex =&gt; 1\.0' "${TARGET}"
if grep -qE '=\s+&gt;' "${TARGET}"; then
  echo "Broken arrow operator spacing detected in ${TARGET}" >&2
  exit 1
fi

echo "OK: arrows are intact (${TARGET})"
