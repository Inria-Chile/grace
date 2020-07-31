ARG BASE=ocaml/opam2:alpine
FROM ${BASE} AS build

RUN mkdir -p /home/opam/app
WORKDIR /home/opam/app

# Install opam requirements alongside GRACE's 'optional' requirements
RUN sudo apk add --update --no-cache m4 graphviz && \
  opam switch 4.10 && \
  eval $(opam env) && \
  opam update

# Copy the whole directory for proper testing
COPY --chown=opam:nogroup . .
RUN eval $(opam env) && opam install --deps-only .
