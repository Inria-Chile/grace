ARG MAIN
FROM ${MAIN} as source

FROM debian:10

RUN apt-get update && apt-get install -y graphviz && apt-get clean
COPY --from=source /bin/grace /bin/grace

ENTRYPOINT ["/bin/grace"]
