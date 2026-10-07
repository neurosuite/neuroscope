# Reproducible Linux build and test of NeuroScope, driven by the Makefile.
#
#   make docker                                     # or: docker build .
#   make docker WITH_WEBENGINE=OFF                  # without Qt WebEngine (QTextBrowser help viewer)
#   make docker LIBNEUROSUITE_REF=v3.0.0
#
# libneurosuite is fetched from GitHub at LIBNEUROSUITE_REF and built as part of NeuroScope.
# The artifact stage holds the installed files and the .deb package, which make docker copies to dist/.

FROM ubuntu:24.04 AS deps
ARG WITH_WEBENGINE=ON
RUN apt-get update && apt-get install -y --no-install-recommends make && rm -rf /var/lib/apt/lists/*
COPY Makefile /tmp/
RUN make -f /tmp/Makefile ubuntu-deps SUDO= WITH_WEBENGINE=${WITH_WEBENGINE} && rm -rf /var/lib/apt/lists/*

FROM deps AS build
ARG WITH_WEBENGINE=ON
ARG LIBNEUROSUITE_REF=main
ENV BUILD_DIR=/build \
    PREFIX=/install \
    PACKAGE_DIR=/packages \
    BUNDLE_NEUROSUITE=ON \
    WITH_WEBENGINE=${WITH_WEBENGINE} \
    LIBNEUROSUITE_REF=${LIBNEUROSUITE_REF}
WORKDIR /src
COPY . .
RUN make build

FROM build AS test
RUN make check package

FROM scratch AS artifact
COPY --from=test /install /install
COPY --from=test /packages/*.deb /packages/
