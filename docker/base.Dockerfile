ARG BASE=ocaml/opam2:alpine
FROM ${BASE} AS build

# Pre-create workdir as the 'opam' user
RUN mkdir -p /home/opam/app
WORKDIR /home/opam/app

COPY apk-packages .
RUN set -eux; \
    sudo apk add --update --no-cache $(cat apk-packages); \
    opam switch 4.10; \
    eval $(opam env); \
    opam update

COPY --chown=opam:nogroup grace.opam .
RUN eval $(opam env) && opam install --deps-only .
