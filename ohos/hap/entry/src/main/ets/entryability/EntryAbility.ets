import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { window } from '@kit.ArkUI';

export default class EntryAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    hilog.info(0x0000, 'LUTSA', 'EntryAbility onCreate');
  }

  onDestroy(): void {
    hilog.info(0x0000, 'LUTSA', 'EntryAbility onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    windowStage.loadContent('pages/Index', (err) => {
      if (err.code) {
        hilog.error(0x0000, 'LUTSA', 'Failed to load Index: %{public}s', JSON.stringify(err));
        return;
      }
      hilog.info(0x0000, 'LUTSA', 'Index loaded');
    });
  }

  onWindowStageDestroy(): void {
    hilog.info(0x0000, 'LUTSA', 'onWindowStageDestroy');
  }

  // L2 perception source: publish foreground/background transitions through AppStorage so the
  // page-level scheduler can react (throttle on background, warm up on foreground).
  onForeground(): void {
    AppStorage.setOrCreate('appFgState', true);
    hilog.info(0x0000, 'LUTSA', 'L2 perception: FOREGROUND');
  }

  onBackground(): void {
    AppStorage.setOrCreate('appFgState', false);
    hilog.info(0x0000, 'LUTSA', 'L2 perception: BACKGROUND');
  }
}
