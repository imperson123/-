#include "qos_policy.h"

#include <atomic>
#include <cstdio>
#include <cstring>

// OH_QoS_* needs system lib not always in app NDK link set.
// Enable with -DTMAC_USE_OH_QOS=ON when building into full system image.
#if defined(TMAC_USE_OH_QOS)
#if __has_include(<qos/qos.h>)
#include <qos/qos.h>
#define TMAC_HAVE_OH_QOS 1
#endif
#endif

namespace tmac_sched {
namespace {
std::atomic<int> g_level{static_cast<int>(QoSLevel::kDefault)};
}

bool SetThreadQoS(QoSLevel level) {
  g_level.store(static_cast<int>(level));
#if defined(TMAC_HAVE_OH_QOS)
  QoS_Level oh = QOS_DEFAULT;
  switch (level) {
    case QoSLevel::kBackground:
      oh = QOS_BACKGROUND;
      break;
    case QoSLevel::kUtility:
      oh = QOS_UTILITY;
      break;
    case QoSLevel::kUserInteractive:
      oh = QOS_USER_INTERACTIVE;
      break;
    default:
      oh = QOS_DEFAULT;
      break;
  }
  return OH_QoS_SetThreadQoS(oh) == 0;
#else
  return true;
#endif
}

QoSLevel GetThreadQoS() {
#if defined(TMAC_HAVE_OH_QOS)
  QoS_Level oh = QOS_DEFAULT;
  if (OH_QoS_GetThreadQoS(&oh) == 0) {
    switch (oh) {
      case QOS_BACKGROUND:
        return QoSLevel::kBackground;
      case QOS_UTILITY:
        return QoSLevel::kUtility;
      case QOS_USER_INTERACTIVE:
        return QoSLevel::kUserInteractive;
      default:
        return QoSLevel::kDefault;
    }
  }
#endif
  return static_cast<QoSLevel>(g_level.load());
}

void LogSchedDecision(const char* signal, QoSLevel chosen) {
  std::printf("[tmac_sched] signal=%s -> qos=%d\n", signal ? signal : "?", static_cast<int>(chosen));
}

QoSLevel DecideFromSignal(const char* signal) {
  if (!signal) {
    return QoSLevel::kDefault;
  }
  if (std::strcmp(signal, "foreground") == 0 || std::strcmp(signal, "notification_burst") == 0) {
    return QoSLevel::kUserInteractive;
  }
  if (std::strcmp(signal, "background") == 0 || std::strcmp(signal, "low_battery") == 0) {
    return QoSLevel::kBackground;
  }
  return QoSLevel::kDefault;
}

bool ApplyPerceptionSignal(const char* signal) {
  QoSLevel lvl = DecideFromSignal(signal);
  LogSchedDecision(signal, lvl);
  return SetThreadQoS(lvl);
}

}  // namespace tmac_sched
