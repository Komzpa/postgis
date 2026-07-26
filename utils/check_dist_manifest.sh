#!/usr/bin/env bash

set -euo pipefail

usage()
{
  echo "Usage: $0 PACKAGE.tar.gz [SOURCE_DIR]" >&2
}

if test "$#" -lt 1 || test "$#" -gt 2; then
  usage
  exit 2
fi

package=$1
source_dir=${2:-.}

if test ! -f "$package"; then
  echo "Package not found: $package" >&2
  exit 1
fi

if ! git -C "$source_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "Source directory is not a Git worktree: $source_dir" >&2
  exit 1
fi

manifest_tmp=${TMPDIR:-/tmp}/postgis-dist-manifest-$$
cleanup()
{
  rm -rf "$manifest_tmp"
}
trap cleanup EXIT

mkdir -p "$manifest_tmp"

tar -tzf "$package" > "$manifest_tmp/tar.raw"

prefix=$(sed -n '1s#/.*##p' "$manifest_tmp/tar.raw")
if test -z "$prefix"; then
  echo "Could not determine package top-level directory" >&2
  exit 1
fi

sed "s#^${prefix}/##" "$manifest_tmp/tar.raw" |
  sed '/^$/d; /\/$/d' |
  LC_ALL=C sort -u > "$manifest_tmp/tar.files"

git -C "$source_dir" ls-files |
  awk '
    function intentional_release_omission(path) {
      return \
        path == "AGENTS.md" ||
        path == "make_dist.sh" ||
        path == ".clang-format" ||
        path == ".gitlab-ci.yml" ||
        path == "utils/check_releases_md5.sh" ||
        path ~ /^ci\// ||
        path ~ /^\.woodpecker\// ||
        path ~ /^\.github\// ||
        path ~ /^\.dron.*\.yml$/ ||
        path ~ /^doc\/development\// ||
        path ~ /^doc\/skills\// ||
        path ~ /^doc\/rfc\//
    }

    function release_relevant(path) {
      return \
        path ~ /\.(c|cc|cpp|h|hpp|in|ac|m4|mk|pl|py|sh|xslt?)$/ ||
        path ~ /\.(sql|control)(\.in)?$/ ||
        path ~ /^extensions\/.*\.(control|sql)(\.in)?$/ ||
        path ~ /^doc\/.*\.(xml|xsl|css|js|html|png|jpg|jpeg|svg|txt|po)$/ ||
        path ~ /(^|\/)(tests?\.mk\.in|README)$/ ||
        path ~ /^regress\// ||
        path ~ /^raster\/test\// ||
        path ~ /^topology\/test\// ||
        path ~ /^sfcgal\/regress\// ||
        path ~ /^liblwgeom\/cunit\// ||
        path ~ /^libpgcommon\/cunit\// ||
        path ~ /^postgis\/cunit\// ||
        path ~ /^loader\/cunit\// ||
        path ~ /^utils\/test_/
    }

    !intentional_release_omission($0) && release_relevant($0) { print }
  ' |
  LC_ALL=C sort -u > "$manifest_tmp/tracked.expected"

comm -23 "$manifest_tmp/tracked.expected" "$manifest_tmp/tar.files" \
  > "$manifest_tmp/missing"

if test -s "$manifest_tmp/missing"; then
  echo "Tracked release-relevant files missing from source distribution:" >&2
  cat "$manifest_tmp/missing" >&2
  exit 1
fi

checked=$(wc -l < "$manifest_tmp/tracked.expected" | tr -d ' ')
echo "Source distribution manifest includes all checked release-relevant tracked files (${checked} files)."
