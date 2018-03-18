#ifndef INCLUDE_ETCD3_H_
#define INCLUDE_ETCD3_H_

#include <grpcpp/grpcpp.h>
#include "../proto-src/rpc.grpc.pb.h"
#include "../proto-src/v3lock.grpc.pb.h"
#include "../proto-src/kv.grpc.pb.h"

using std::string;

namespace etcd3 {

namespace pb {
  using namespace ::etcdserverpb;
  using namespace ::v3lockpb;
  using namespace ::mvccpb;
}
// namespace etcd::pb

typedef std::unique_ptr<
    grpc::ClientReaderWriterInterface<pb::WatchRequest, pb::WatchResponse>>
    WatchStreamPtr;

// A thin client wrapper for the etcd3 grpc interface.
class Client {
 public:
  explicit Client(std::shared_ptr<grpc::ChannelInterface> channel);
  Client(
      pb::KV::StubInterface* allocated_kv_stub,
      pb::Watch::StubInterface* allocated_watch_stub,
      pb::Lease::StubInterface* allocated_lease_stub,
      pb::Lock::StubInterface* allocated_lock_stub
  );

  grpc::Status Put(const pb::PutRequest &req, pb::PutResponse *res);

  // Get a range of keys, which can also be a single key or the set of all keys
  // matching a prefix.
  grpc::Status Range(const pb::RangeRequest &request,
                     pb::RangeResponse *response) const;

  // Create a watch stream, which is a bidirectional GRPC stream where the
  // client receives all change events to the requested keys.
  // The first event received contains the result of the connection attempt.
  WatchStreamPtr MakeWatchStream(const pb::WatchRequest& req);

  void WatchCancel(int64_t watch_id);

  // Request a lease, which is a session with etcd kept alive by
  // LeaseKeepAlive requests. It can be associated with keys and locks to
  // delete or release them when the session times out, respectively.
  grpc::Status LeaseGrant(const pb::LeaseGrantRequest &req,
                          pb::LeaseGrantResponse *res);

  grpc::Status LeaseKeepAlive(const pb::LeaseKeepAliveRequest &req,
                              pb::LeaseKeepAliveResponse *res);

  // Wait until the specified lock can be acquired. As long as the caller
  // holds the lock, a certain key (given in the response) will exist in etcd.
  grpc::Status Lock(const pb::LockRequest &req, pb::LockResponse *res);

  grpc::Status Unlock(const pb::UnlockRequest &req,
                      pb::UnlockResponse *res);

  // Perform a transaction, which is a set of boolean predicates and two sets
  // of operations: one set to do if the predicates are all true, and one set
  // to do otherwise.
  grpc::Status Transaction(const pb::TxnRequest& req,
                           pb::TxnResponse* res);

 private:
  std::unique_ptr<pb::KV::StubInterface> kv_stub_;
  std::unique_ptr<pb::Lease::StubInterface> lease_stub_;
  std::unique_ptr<pb::Lock::StubInterface> lock_stub_;
  std::unique_ptr<pb::Watch::StubInterface> watch_stub_;
};

namespace util {
// Helper functions for transactions
void MakeKeyExistsCompare(const std::string &key, pb::Compare *compare);

void MakeKeyNotExistsCompare(const std::string &key, pb::Compare *compare);

void AllocatePutRequest(const std::string &key, const std::string &value,
                        pb::RequestOp *requestOp);

std::unique_ptr<pb::RequestOp> BuildRangeRequest(
    const std::string &key,
    const std::string &range_end
);

std::unique_ptr<pb::RequestOp>
BuildPutRequest(const std::string &key, const std::string &value);

std::unique_ptr<pb::RequestOp> BuildGetRequest(const std::string &key);

std::unique_ptr<pb::RequestOp> BuildDeleteRequest(const std::string &key);

std::string RangePrefix(const std::string &key);

class BackoffOpts {
 public:
  int interval_ms = 500;
  int timeout_ms = 30 * 1000;
  float multiplier = 2;
};
grpc::Status ExponentialBackoff(std::function<grpc::Status()> job,
                                BackoffOpts opts);
grpc::Status ExponentialBackoff(std::function<grpc::Status()> job);

}

}
#endif //INCLUDE_ETCD3_H_
