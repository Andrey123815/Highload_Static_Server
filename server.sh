#!/usr/bin/bash
# Provides: static_server
# Required-View-Cmd: $SERVER_NAME $cmd $key
### END INIT INFO


export STATIC_SERVER_HOME=.
export SERVER_NAME=static_server
export PID_FILE=pid_file.txt
export DEFAULT_PATH_TO_CONFIG=.server.conf


get_pid() {
  PID_MASTER_PROCESS=$(head -n 1 "$PID_FILE")
}


get_has_server_started() {
  HAS_SERVER_STARTED=$(ps aux | grep ./static_server.out | wc -l)
}


start() {
  if [ ! -d cmake-build-debug ]; then
    echo "To start server you need to make 'sudo ./server.sh build'"
    exit 1
  fi

  get_has_server_started
  if [ "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has already started!"
    exit 1
  else
    if [ -f error.log ]; then
      rm error.log
    fi
    if [ -f access.log ]; then
      rm access.log
    fi
    touch access.log
    touch error.log
    echo "Starting $SERVER_NAME Server..."
    if [ -f static_server.out ]; then
      rm static_server.out
    fi
    cp ./cmake-build-debug/static_server.out static_server.out
    "$STATIC_SERVER_HOME"/static_server.out
    echo "Server started!"
    exit 0
  fi
}


stop() {
  get_has_server_started
  if [ ! "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Stopping $SERVER_NAME server..."
    get_pid
    kill -2 "$PID_MASTER_PROCESS"
    echo "Server stopped!"
    exit 0
  fi
}


status() {
  get_has_server_started
  if [ "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "$SERVER_NAME is running!"
  else
    echo "$SERVER_NAME is down!"
  fi
}


build() {
  if [ -d cmake-build-debug ]; then
    cd cmake-build-debug
    cmake ..
    make clean && make
    cd ..
  else
    mkdir cmake-build-debug
    cd cmake-build-debug
    cmake ..
    make clean && make
    cd ..
  fi
}


case $1 in
  start)
    start
  ;;

  stop)
    stop
  ;;

  status)
    status
  ;;

  build)
    build
  ;;

  *)
    if [ "$(ps aux | grep ./static_server.out | wc -l)" \> 1 ]; then
      echo "Usage : <stop|status>";
    else
      echo "Usage : <start|build|status>";
    fi
  ;;
esac
exit 0