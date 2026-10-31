# syntax=docker/dockerfile:1
#
# Arduino compile service for the Fritzing web app; see server.py and docker/wasm/compose.yaml.
#
#   docker build -t fritzing-compile docker/wasm/compile-service

FROM debian:bookworm-slim

ARG ARDUINO_CLI_VERSION=1.1.1
ARG ARDUINO_AVR_VERSION=1.8.6
ARG TARGETARCH

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends ca-certificates curl python3 \
    && rm -rf /var/lib/apt/lists/*

RUN case "$TARGETARCH" in \
         arm64) asset=Linux_ARM64; sum=b6a18d39d574b94e8221b5d0afe79d20d846227b4959ff95e44279021593b199 ;; \
         amd64) asset=Linux_64bit; sum=2b807add5c3a518861bfd4d3e78de9e02b0a2723df3d90386df2200a767349c4 ;; \
         *) echo "unsupported architecture $TARGETARCH" >&2; exit 1 ;; \
       esac \
    && curl -fsSL -o /tmp/arduino-cli.tar.gz \
       "https://github.com/arduino/arduino-cli/releases/download/v${ARDUINO_CLI_VERSION}/arduino-cli_${ARDUINO_CLI_VERSION}_${asset}.tar.gz" \
    && echo "$sum  /tmp/arduino-cli.tar.gz" | sha256sum -c - \
    && tar xzf /tmp/arduino-cli.tar.gz -C /usr/local/bin arduino-cli \
    && rm /tmp/arduino-cli.tar.gz

# The board core and its compiler are installed at build time (arduino-cli checks the
# package index signature), so compiling needs no network access
ENV ARDUINO_DIRECTORIES_DATA=/opt/arduino \
    ARDUINO_DIRECTORIES_USER=/opt/arduino/user \
    ARDUINO_UPDATER_ENABLE_NOTIFICATION=false \
    ARDUINO_BUILD_CACHE_PATH=/tmp/arduino-cache
RUN arduino-cli core update-index \
    && arduino-cli core install "arduino:avr@${ARDUINO_AVR_VERSION}" \
    && mkdir -p /opt/arduino/user

RUN useradd --system --uid 10001 --home-dir /nonexistent compile
WORKDIR /app
COPY server.py test_server.py ./

USER compile
EXPOSE 8000
CMD ["python3", "server.py"]
