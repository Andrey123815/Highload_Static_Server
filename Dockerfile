FROM ubuntu:latest as build

USER root

EXPOSE 80

RUN apt-get update && \
    apt-get install -y \
       g++ \
      cmake \
      libboost-all-dev

COPY . /app

WORKDIR /app/build

RUN cmake .. && cmake --build .

ENTRYPOINT [ "/app/build/static_server.out" ]
