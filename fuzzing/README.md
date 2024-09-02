# How to run fuzzing

## Install AFL++

```
docker pull aflplusplus/aflplusplus:latest
```

## Build with fuzzing instrumentation

From the root of the repo:
```
docker run -ti --mount type=tmpfs,destination=/ramdisk --mount type=bind,source=.,destination=/src -e AFL_TMPDIR=/ramdisk -e AFL_USE_ASAN=1 aflplusplus/aflplusplus
cd /src
mkdir build_fuzzing
cd build_fuzzing
cmake .. -DCMAKE_CXX_COMPILER=afl-clang-fast++ -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j12
```

## Start fuzzing

Within the same container as above:
```
afl-fuzz -i fuzzing/input -o fuzzing/output build_fuzzing/fuzzing/runner/jsonexpr-fuzzing-runner @@
```
