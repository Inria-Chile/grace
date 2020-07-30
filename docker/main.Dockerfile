ARG BASE
FROM ${BASE} as build

COPY --chown=opam:nogroup . .

ARG VERSION=local
ARG SHA1=local
ARG BUILD=local
RUN sed -e "s/%%VERSION%%/${VERSION}/g" \
        -e "s/%%SHA1%%/${SHA1}/g" \
        -e "s/%%BUILD%%/${BUILD}/g" \
        -i lib/Version.re

RUN eval $(opam env) && dune build bin/grace.exe

FROM scratch
COPY --from=build /home/opam/app/_build/default/bin/grace.exe /bin/grace
ENTRYPOINT ["/bin/grace"]
