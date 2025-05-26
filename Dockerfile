FROM ubuntu:22.04

# 필수 패키지 설치
RUN apt update && apt install -y build-essential curl ca-certificates

# monitoring 소스 복사
COPY monitoring.h /usr/src/monitoring.h
COPY test.c /usr/src/test.c

# 빌드
RUN gcc -o /test /usr/src/test.c -lpthread

# 실행
CMD ["/test"]
