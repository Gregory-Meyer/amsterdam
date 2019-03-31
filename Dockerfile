FROM gregjm/cpp-build

RUN ["zypper", "--non-interactive", "in", "git"]

WORKDIR /usr/src/app
COPY bin bin
COPY CMakeLists.txt ./
COPY external external
COPY include include
COPY src src
COPY test test
ENTRYPOINT ["./bin/docker-entrypoint.sh"]
