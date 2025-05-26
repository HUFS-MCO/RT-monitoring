FROM ubuntu:22.04

RUN apt update && apt install -y build-essential curl ca-certificates

COPY monitoring.h /usr/src/monitoring.h
COPY test.c /usr/src/test.c

RUN gcc -o /test /usr/src/test.c -lpthread

CMD ["/test"]
