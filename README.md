# gRPC Chat Streaming Service

### Install gRPC
[Setup gRPC](https://grpc.io/docs/languages/cpp/quickstart/#setup)


### How to build this project?

1. `export MY_INSTALL_DIR=<YOUR_gRPC_INSTALL_DIRECTORY> && mkdir -p cmake/build` (Locally installed gRPC can be found in `$HOME/.local`)

2. `pushd cmake/build`

3. `cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR  ../..`

4. `make -j 4` (or any number of processors which can be found via `nproc` command on linux)

5. `./chat_server`

6. `./chat_client`
