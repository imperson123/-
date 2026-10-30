# P4 · 感知 → 调度演示脚本（宿主 / 板上 stub）

设备上替换为 Background Tasks + 真实 `OH_QoS_*`。当前在进程内模拟前后台切换。

```powershell
# After building libtmac_sa.so, link a tiny driver or call from Demo App.
# This script documents the expected log sequence for video capture.
```

Expected demo log:

```
[tmac_sched] signal=foreground -> qos=3
[tmac_sched] signal=notification_burst -> qos=3
[tmac_sched] signal=background -> qos=0
[tmac_sched] signal=low_battery -> qos=0
```

QoSLevel: 0=Background 1=Utility 2=Default 3=UserInteractive
