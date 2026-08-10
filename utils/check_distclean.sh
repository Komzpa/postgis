#!/bin/sh

TMPBASE="${TMPDIR:-/tmp}"
mkdir -p "${TMPBASE}"
TMPBASE=$(cd "${TMPBASE}" && pwd -P)
TMPDIR="${TMPBASE}/postgis_check_distclean_$$"
BASELINE="$1"

cleanup()
{
  #echo "${TMPDIR} has things"
  rm -rf "${TMPDIR}"
}

trap 'cleanup' 0

mkdir -p "${TMPDIR}"


CHECKDIR=$(pwd -P)
case "${TMPDIR}/" in
  "${CHECKDIR}/"*)
    TMPREL="./${TMPDIR#"${CHECKDIR}"/}"
    find . -path "${TMPREL}" -prune -o -type f -print | sort > "${TMPDIR}/files_after_distclean"
    ;;
  *)
    find . -type f | sort > "${TMPDIR}/files_after_distclean"
    ;;
esac
if test -n "${BASELINE}"; then
  sort "${BASELINE}" > "${TMPDIR}/baseline_files"
  comm -13 \
    "${TMPDIR}/baseline_files" \
    "${TMPDIR}/files_after_distclean" > \
    "${TMPDIR}/leftover_files_after_distclean"
else
  cp "${TMPDIR}/files_after_distclean" "${TMPDIR}/leftover_files_after_distclean"
fi
cat <<EOF > "${TMPDIR}/leftover_files_after_distclean.expected"
./doc/postgis_comments.sql
./doc/raster_comments.sql
./doc/sfcgal_comments.sql
./doc/topology_comments.sql
./liblwgeom/lwin_wkt_lex.c
./liblwgeom/lwin_wkt_parse.c
./liblwgeom/lwin_wkt_parse.h
./postgis_revision.h
EOF

grep -F -v -f \
  "${TMPDIR}/leftover_files_after_distclean.expected" \
  "${TMPDIR}/leftover_files_after_distclean" > \
  "${TMPDIR}/unexpected_leftovers"

if test -s "${TMPDIR}/unexpected_leftovers"; then
  echo "Unexpected left over files after distclean:" >&2
  cat "${TMPDIR}/unexpected_leftovers" >&2
  false
fi
