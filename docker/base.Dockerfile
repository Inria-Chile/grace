ARG BASE=ocaml/opam2:alpine
FROM ${BASE} AS build

RUN mkdir -p /home/opam/app
WORKDIR /home/opam/app

RUN sudo apk add --update --no-cache m4 && \
  opam switch 4.10 && \
  eval $(opam env) && \
  opam update

COPY --chown=opam:nogroup grace.opam .
RUN eval $(opam env) && opam install --deps-only .
