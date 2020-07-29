ARG MAIN
FROM ${MAIN} as source

FROM alpine:3.12

RUN apk add --update --no-cache graphviz
COPY --from=source /bin/grace /bin/grace

ENTRYPOINT ["/bin/grace"]
