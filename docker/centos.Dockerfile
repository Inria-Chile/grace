ARG MAIN
FROM ${MAIN} as source

FROM centos:8

RUN yum install -y graphviz && yum clean all
COPY --from=source /bin/grace /bin/grace

ENTRYPOINT ["/bin/grace"]
