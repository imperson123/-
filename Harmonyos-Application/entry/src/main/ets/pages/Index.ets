import { TMAC } from './TMAC'; // 导入TMAC模块
import { hilog } from '@kit.PerformanceAnalysisKit';
import testNapi from 'libentry.so';

const DOMAIN = 0x0000;

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    Row() {
      Column() {
        Text('测试T-MAC矩阵乘')
          .fontSize(30)
          .onClick(() => {
            // 初始化T-MAC
            const tmac = new TMAC();
            this.message = tmac.test(); // 调用测试方法
          })
      }
      .width('100%')
    }
    .height('100%')
  }
}
