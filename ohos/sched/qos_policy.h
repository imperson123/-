#pragma once

namespace tmac_sched {

enum class QoSLevel {
  kBackground = 0,
  kUtility = 1,
  kDefault = 2,
  kUserInteractive = 3,
};

bool SetThreadQoS(QoSLevel level);
QoSLevel GetThreadQoS();
void LogSchedDecision(const char* signal, QoSLevel chosen);

QoSLevel DecideFromSignal(const char* signal);
bool ApplyPerceptionSignal(const char* signal);

}  // namespace tmac_sched
