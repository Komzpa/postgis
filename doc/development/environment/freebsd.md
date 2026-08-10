---
title: "FreeBSD Development Environment"
date: 2026-06-27
weight: 30
geekdocHidden: false
---

The current FreeBSD pull-request build is defined in
`.github/workflows/ci-freebsd.yml`. Use that workflow as the source of truth
when checking platform-specific dependencies or flags.

At the time of this note, the job runs FreeBSD 14.4 through
`vmactions/freebsd-vm`, switches the package repository from quarterly to
latest, installs PostgreSQL 17 packages, and builds with clang through ccache.
It builds the topology, SFCGAL, and protobuf paths, but disables raster and GUI:

```sh
sed -i.bak \
  -e 's,pkg+http://pkg.FreeBSD.org/${ABI}/quarterly,pkg+http://pkg.FreeBSD.org/${ABI}/latest,' \
  /etc/pkg/FreeBSD.conf
ASSUME_ALWAYS_YES=yes pkg bootstrap -f
env IGNORE_OSVERSION=yes pkg update -f
env IGNORE_OSVERSION=yes pkg install -y \
  autoconf automake bison ccache-static cunit docbook \
  gdal geos gmake iconv json-c libtool libxml2 libxslt \
  pkgconf postgresql17-contrib postgresql17-server \
  proj protobuf-c sfcgal
projsync --system-directory --source-id us_noaa
projsync --system-directory --source-id ch_swisstopo
```

The workflow sets the compiler and ccache environment before configuring:

```sh
export CC="ccache clang"
export CXX="ccache clang++"
export CCACHE_STATIC_PREFIX="/usr/local"
export MAKEJOBS="-j2"
```

Then it runs:

```sh
find . -name "*.pl" | xargs sed -i '' 's|/usr/bin/perl|/usr/bin/env perl|'

./autogen.sh
./configure \
  PKG_CONFIG=/usr/local/bin/pkgconf \
  CFLAGS="-isystem /usr/local/include -Wall -fno-omit-frame-pointer -Werror" \
  LDFLAGS="-L/usr/local/lib" \
  --with-libiconv-prefix=/usr/local \
  --without-gui \
  --with-topology \
  --without-raster \
  --with-sfcgal=/usr/local/bin/sfcgal-config \
  --with-protobuf

service postgresql oneinitdb
service postgresql onestart
su postgres -c "createuser -s $(whoami)"

gmake ${MAKEJOBS}
gmake ${MAKEJOBS} install
gmake ${MAKEJOBS} check RUNTESTFLAGS="-v --extension --dumprestore"
service postgresql onestop
```

If local behavior differs from CI, first compare the FreeBSD release, package
repository, PostgreSQL package major version, `pkgconf` path, and configure
flags against `.github/workflows/ci-freebsd.yml`.

## Woodie FreeBSD local-backend runner

PostGIS also has a draft FreeBSD KVM recipe in the buildbot repository. It
lives on the `freebsd14-runner-iac` branch, not on `master`, so the branch has
to be named when cloning:

```sh
git clone -b freebsd14-runner-iac https://gitea.osgeo.org/postgis/postgis-buildbots.git
cd postgis-buildbots/freebsd/freebsd14_runner
cp config/runner.env config/runner.local.env
$EDITOR config/runner.local.env
make all
```

The recipe pins `FREEBSD_VERSION="14.3-RELEASE"`, and the last recorded geocint
guest reported `14.3-RELEASE-p16`. **FreeBSD 14.3 reached end of life on 30 June
2026.** Moving to a supported release means rebuilding the guest from a newer
image, not upgrading in place through this recipe. Until that happens, the
recipe cannot provide a current FreeBSD CI signal. The GitHub workflow is a
separate matter and pins supported FreeBSD 14.4. Check the
[FreeBSD security support table](https://www.freebsd.org/security/) before the
next version bump.

The recipe is project-owned infrastructure-as-code. It fetches the FreeBSD cloud
image, seeds SSH access, installs the package set, records the resolved package
lock, and runs the complete PostGIS suite. Generated VM images, logs, runtime
artifacts, SSH keys, and Woodpecker secrets stay out of git.

The recorded geocint deployment used 16 vCPU and 16 GiB of configured RAM. The
complete suite should be budgeted at about 41 minutes including boot, SSH wait,
source copy, and wrapper overhead. A persistent Woodie
local-backend VM avoids most boot/copy overhead but should still be budgeted as
an integration-branch signal rather than a default for every pull request.

The corresponding Woodie workflow is `.woodpecker/freebsd.yml`. It is
manual-only while the available recipe pins an unsupported release. Do not add
push or pull-request events until a supported guest and matching live Woodie
agent have completed a clean exact-commit run. If maintainers later enable
pull-request FreeBSD runs, keep the status context present and gate the
expensive body with the checks described in
[Pull request CI gating](../testing/ci-gating.md).

### Package and link traps

The full-suite VM recipe intentionally uses PostgreSQL 18 packages together with
`gdal`:

```sh
pkg install -y \
  postgresql18-client postgresql18-server postgresql18-contrib \
  geos proj gdal sfcgal json-c protobuf-c libxml2 pcre2 sqlite3 cunit
```

Do not substitute an older PostgreSQL package major just because it looks closer
to another CI job. On the verified FreeBSD package set, `gdal` resolves against
PostgreSQL 18 client packages; installing another major first lets `pkg` remove
that client/server set while satisfying `gdal`.

Keep `/usr/local` explicit in the build environment:

```sh
export CPPFLAGS="-I/usr/local/include ${CPPFLAGS:-}"
export LDFLAGS="-L/usr/local/lib ${LDFLAGS:-}"
./configure --with-pgconfig=/usr/local/bin/pg_config
```

Without `-L/usr/local/lib`, the loader build can fail to link GNU libiconv
symbols such as `libiconv_open`, `libiconv`, `libiconv_close`, and
`libiconvctl`. This is a FreeBSD recipe requirement, not a PostGIS source
failure.

### Reproducing a FreeBSD-only failure

After the recipe has been updated to a supported release, reproduce a manual
Woodie failure by first reading the job's commit SHA and branch. Check out that
exact PostGIS commit locally, then point `POSTGIS_SRC` in
`config/runner.local.env` at that checkout and run the buildbot recipe:

```sh
git -C /path/to/postgis fetch https://gitea.osgeo.org/postgis/postgis.git
git -C /path/to/postgis checkout <commit-sha-from-woodie>
cd postgis-buildbots/freebsd/freebsd14_runner
printf 'POSTGIS_SRC=%s\n' /path/to/postgis >> config/runner.local.env
make verify
```

The runner writes host-side logs and result bundles under
`freebsd/freebsd14_runner/logs/` and `freebsd/freebsd14_runner/artifacts/`.
Inside an adopted persistent VM, access is through the SSH host, port, user, and
key declared in `config/runner.local.env`; the checked-in template uses
localhost port forwarding and does not contain real host credentials. If the VM
is scheduled in Woodie, the live agent and scheduling state are owned by the
OSGeo SAC Woodie configuration and the Woodie admin UI, not by this source tree.
