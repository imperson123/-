#pragma once
// Minimal dmlc/logging.h shim for T-MAC direct mode (no TVM).
// Injected ahead of include/ via ohos/shim include path — do not edit upstream wrapper.

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace dmlc {
namespace shim_detail {

class LogMessage {
 public:
  LogMessage(const char* /*file*/, int /*line*/, bool fatal) : fatal_(fatal) {}
  ~LogMessage() {
    if (fatal_) {
      std::cerr << stream_.str() << std::endl;
      std::abort();
    }
  }
  template <typename T>
  LogMessage& operator<<(const T& v) {
    stream_ << v;
    return *this;
  }

 private:
  bool fatal_;
  std::ostringstream stream_;
};

class NullStream {
 public:
  template <typename T>
  NullStream& operator<<(const T&) {
    return *this;
  }
};

}  // namespace shim_detail
}  // namespace dmlc

#ifndef LOG
#define LOG(severity) LOG_##severity
#define LOG_INFO ::dmlc::shim_detail::NullStream()
#define LOG_WARNING ::dmlc::shim_detail::NullStream()
#define LOG_ERROR ::dmlc::shim_detail::LogMessage(__FILE__, __LINE__, false)
#define LOG_FATAL ::dmlc::shim_detail::LogMessage(__FILE__, __LINE__, true)
#endif

#ifndef DCHECK
#ifdef NDEBUG
#define DCHECK(x) \
  while (false) ::dmlc::shim_detail::NullStream()
#else
#define DCHECK(x) \
  ((x) ? ::dmlc::shim_detail::NullStream() \
       : (::dmlc::shim_detail::LogMessage(__FILE__, __LINE__, true) << "DCHECK failed: " #x " "))
#endif
#endif

#ifndef ICHECK
#define ICHECK(x) \
  ((x) ? ::dmlc::shim_detail::NullStream() \
       : (::dmlc::shim_detail::LogMessage(__FILE__, __LINE__, true) << "ICHECK failed: " #x " "))
#endif

#ifndef DLOG
#define DLOG(severity) LOG_INFO
#endif
