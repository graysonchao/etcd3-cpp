# etcd3-cpp
A thin C++ wrapper for the etcd3 gRPC API. IN EARLY DEVELOPMENT.

## What do you mean by thin?
etcd3-cpp (currently) makes no attempt to hide gRPC from the user. Rather, 
the library aims to provide utility functions and a single namespace under 
which the various etcdv3 APIs can be accessed. Methods return grpc::Status, 
and the user is meant to check this to detect failures and retry.

## Usage

```
#include "etcd3/include/etcd3.h"
etcd::pb::PutRequest request;
request.set_key("key");
request.set_value("value");

etcd::pb::PutResponse response;
auto status = etcd_client.Put(request, &response);

if (!status.ok()) {
 freak_out(); 
} else {
 rest_easy();
}
```
[Full documentation](https://coreos.com/etcd/docs/latest/dev-guide/api_reference_v3.html) is available.

## Building
gRPC and Protobufs must be installed. A suitable version of gRPC is provided as a submodule.

First, install gRPC:
```
$ pwd
/home/gchao/code/etcd3-cpp
$ git submodule update --init
$ cd grpc
$ make
$ make install
```

Then, build etcd3-cpp:
```
$ pwd
/home/gchao/code/etcd3-cpp
$ git submodule update --init
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Notes
This repo provides prebuilt C++ source files that provide the etcd3 API 
protobufs types because it's complicated and error-prone to extract 
these definitions from the etcd repo.


gRPC is included in this repo under `third_party/grpc`. The submodule just
points to the same commit as the latest 1.10.x release, nothing special.


