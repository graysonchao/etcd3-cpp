#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "include/etcd3.h"

namespace etcd3 {

Client::Client(std::shared_ptr<grpc::ChannelInterface> channel)
    : kv_stub_(pb::KV::NewStub(channel)),
      lease_stub_(pb::Lease::NewStub(channel)),
      lock_stub_(pb::Lock::NewStub(channel)),
      watch_stub_(pb::Watch::NewStub(channel)) {}

Client::Client(pb::KV::StubInterface* allocated_kv_stub,
               pb::Watch::StubInterface* allocated_watch_stub,
               pb::Lease::StubInterface* allocated_lease_stub,
               pb::Lock::StubInterface* allocated_lock_stub)
    : kv_stub_(std::unique_ptr<pb::KV::StubInterface>(allocated_kv_stub)),
      watch_stub_(std::unique_ptr<pb::Watch::StubInterface>(
          allocated_watch_stub)),
      lease_stub_(std::unique_ptr<pb::Lease::StubInterface>(
          allocated_lease_stub)),
      lock_stub_(std::unique_ptr<pb::Lock::StubInterface>(
          allocated_lock_stub)) {}

/**
 * Put a value to a key in etcd
 * @param status
 * @param key
 * @param value
 * @param lease
 * @param prev_key
 * @param ignore_value
 * @param ignore_lease
 * @return
 */
grpc::Status Client::Put(const pb::PutRequest& req, pb::PutResponse* res) {
  grpc::ClientContext context;
  return kv_stub_->Put(&context, req, res);
}

/**
 * Query etcd for a key or a range of keys.
 * Returns a std::unique_ptr to a RangeResponse.
 * @param status a grpc::Status that will be overwritten with the reply's status.
 * @param key
 * @param range_end
 * @param revision (latest by default)
 * @return
 */
grpc::Status Client::Range(const pb::RangeRequest& req,
                                 pb::RangeResponse* res) const {
  grpc::ClientContext context;
  return kv_stub_->Range(&context, req, res);
}

grpc::Status Client::LeaseGrant(const pb::LeaseGrantRequest& req,
                                      pb::LeaseGrantResponse* res) {
  grpc::ClientContext context;
  return lease_stub_->LeaseGrant(&context, req, res);
}

grpc::Status Client::LeaseKeepAlive(
    const pb::LeaseKeepAliveRequest& req,
    pb::LeaseKeepAliveResponse* res
) {
  grpc::ClientContext context;
  auto stream = lease_stub_->LeaseKeepAlive(&context);
  stream->Write(req);
  stream->WritesDone();
  stream->Read(res);
  return stream->Finish();
}

grpc::Status Client::Lock(const pb::LockRequest& req, pb::LockResponse* res) {
  grpc::ClientContext context;
  return lock_stub_->Lock(&context, req, res);
}

grpc::Status Client::Unlock(const pb::UnlockRequest& req,
                                  pb::UnlockResponse* res) {
  grpc::ClientContext context;
  return lock_stub_->Unlock(&context, req, res);
}

grpc::Status Client::Transaction(const pb::TxnRequest& req,
                                       pb::TxnResponse* res) {
  grpc::ClientContext context;
  return kv_stub_->Txn(&context, req, res);
}

WatchStreamPtr Client::MakeWatchStream(const pb::WatchRequest& req) {
  auto context = new grpc::ClientContext();
  auto watch_stream = watch_stub_->Watch(context);
  watch_stream->Write(req);
  return watch_stream;
}

void Client::WatchCancel(int64_t watch_id) {
  grpc::ClientContext context;
  auto watch_stream = watch_stub_->Watch(&context);
  auto wcr = new pb::WatchCancelRequest();
  wcr->set_watch_id(watch_id);
  pb::WatchRequest req;
  req.set_allocated_cancel_request(wcr);
  watch_stream->Write(req);
  watch_stream->WritesDone();
}

// Utility functions, mostly for working with transactions

void util::MakeKeyExistsCompare(
    const std::string& key,
    pb::Compare* compare
) {
  compare->set_key(key);
  compare->set_result(pb::Compare_CompareResult_GREATER);
  compare->set_target(pb::Compare_CompareTarget_CREATE);
  compare->set_create_revision(0);
}

void util::MakeKeyNotExistsCompare(
    const std::string& key,
    pb::Compare* compare
) {
  compare->set_key(key);
  compare->set_result(pb::Compare_CompareResult_LESS);
  compare->set_target(pb::Compare_CompareTarget_CREATE);
  compare->set_create_revision(1);
}

// Convenience method to construct a RangeRequest that gets all keys where
// KEY is a prefix.
std::string util::RangePrefix(const std::string& key) {
  // If the last char is \xff the prefix "wraps"
  // https://coreos.com/etcd/docs/latest/learning/api.html#key-value-api
  const auto last_data_pos = key.find_last_not_of(static_cast<char>('\xFF'));
  assert(last_data_pos != 0 && last_data_pos != std::string::npos);
  auto prefix = key.substr(0, last_data_pos + 1);
  prefix.back() += 1;
  return prefix;
}


// Given a function returning a grpc::Status, repeatedly call the function
// until timeout elapses or the returned status == OK.
// WARNING: makes no attempt to ensure that elapsed_ms follows wall time.
grpc::Status util::ExponentialBackoff(std::function<grpc::Status()> job,
                                            util::BackoffOpts opts) {
  int elapsed_ms = 0;
  while (true) {
    auto status = job();
    std::this_thread::sleep_for(std::chrono::milliseconds(opts.interval_ms));
    elapsed_ms += opts.interval_ms;
    opts.interval_ms *= opts.multiplier;
    if (elapsed_ms > opts.timeout_ms || status.ok()) {
      return status;
    }
  }
}

grpc::Status util::ExponentialBackoff(std::function<grpc::Status()> job) {
  return util::ExponentialBackoff(job, util::BackoffOpts());
}

}
