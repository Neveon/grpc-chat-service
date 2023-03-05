# gRPC Chat Streaming Service

### How to build this project?

1. `mkdir -p cmake/build`

2. `pushd cmake/build`

3. `cmake -DgRPC_INSTALL=ON \ -DgRPC_BUILD_TESTS=OFF \ -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \ ../..`

4. `make -j 4` (or any number of processors which can be found via `nproc` on linux)

5. `make install`

6. `popd`
