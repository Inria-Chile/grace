ARG BASE=ocaml/opam2:alpine
FROM ${BASE} AS build

# Pre-create workdir as the 'opam' user
RUN mkdir -p /home/opam/app
WORKDIR /home/opam/app

# Install opam requirements alongside GRACE's 'optional' requirements
RUN set -eux; \
    sudo apk add --update --no-cache m4 graphviz; \
    opam switch 4.10; \
    eval $(opam env); \
    opam update

# Copy the package file initially
COPY --chown=opam:nogroup grace.opam .
RUN eval $(opam env) && opam install --deps-only .

# Then copy the whole project for proper testing
COPY --chown=opam:nogroup . .

# By default, run the application
ENV API_LISTEN_ADDRESS 0.0.0.0
ENV LOG_LEVEL debug
CMD [ \
  "sh", \
  "-c", \
  "eval $(opam env) && dune build bin/grace.exe && _build/default/bin/grace.exe" \
]
