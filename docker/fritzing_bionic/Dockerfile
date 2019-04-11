FROM registry.gitlab.com/fritzing/fritzing-app_playground/build:bionic

MAINTAINER Kjell kjell@gmx.de

ARG user_id=1000
ENV env_user_id=${user_id}

RUN groupadd -g ${env_user_id} builder && \
    useradd -r -u ${env_user_id} -g builder builder

USER builder
WORKDIR fritzing
