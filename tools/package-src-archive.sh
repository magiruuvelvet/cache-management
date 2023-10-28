#!/bin/sh

set -e

GIT_TAG="$(git describe --tags --abbrev=0)"
PKG_NAME="cachemgr-src-${GIT_TAG}-dev"

# package files
git ls-files --recurse-submodules | command tar -caf \
    "${PKG_NAME}.tar.xz" \
    --owner=wheel --group=wheel --no-acls --no-xattrs \
    --xform 's:^:'"${PKG_NAME}"'/:' --verbatim-files-from -T-

# create hashsum
sha512sum "${PKG_NAME}.tar.xz" > "${PKG_NAME}.tar.xz.sha512sum"
