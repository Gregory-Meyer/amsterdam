FROM gregjm/cpp-build

RUN ["zypper", "--non-interactive", "in", "git"]

WORKDIR /usr/src
RUN ["git", "clone", "https://github.com/catchorg/Catch2.git"]
WORKDIR /usr/src/Catch2/build
RUN ["cmake", "..", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release"]
RUN ["cmake", "--build", "."]
RUN ["cmake", "--build", ".", "--target", "install"]
RUN ["rm", "-rf", "/usr/src/Catch2"]

WORKDIR /usr/src/app
COPY bin bin
COPY CMakeLists.txt ./
COPY include include
COPY src src
COPY test test
ENTRYPOINT ["./bin/docker-entrypoint.sh"]
