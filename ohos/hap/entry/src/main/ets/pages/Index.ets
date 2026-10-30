import { hilog } from '@kit.PerformanceAnalysisKit';
import { common } from '@kit.AbilityKit';
import { backgroundTaskManager } from '@kit.BackgroundTasksKit';
import { picker, fileIo as fs } from '@kit.CoreFileKit';
import tmac from 'libtmac_hap.so';

// One chat bubble. role: 'user' | 'model'. meta = small caption (e.g. "18.62 tok/s").
interface ChatMsg {
  role: string;
  text: string;
  meta: string;
}

// @Builder params are only reactive when passed by reference — a by-value param
// freezes the builder subtree, so live values must travel as an object.
class ChipVM {
  label: string = '';
  value: string = '';
  wide: boolean = false;
}

const BG_PAGE: string = '#12182B';
const BG_CARD: string = '#1C2438';
const BG_INPUT: string = '#243044';
const C_BORDER: string = '#323E58';
const C_ACCENT: string = '#4FA8E8';
const C_TEXT: string = '#E8EEF7';
const C_SUB: string = '#8E9BB3';
const C_DIM: string = '#5A6A80';
const C_OK: string = '#34C77B';
const C_WARN: string = '#F5A623';
const C_ERR: string = '#E85B5B';

@Entry
@Component
struct Index {
  @State status: string = '就绪';
  @State detail: string = '先「模型」页导入/加载，再在「对话」页生成。';
  @State modelPath: string = '/data/local/tmp/llm-tmac/model.gguf';
  @State sandboxInfo: string = '';
  @State prompt: string = 'The capital of France is';
  @State nPredict: string = '32';
  @State threads: string = '4';
  @State useMmap: boolean = false;
  @State busy: boolean = false;
  @State loadInfo: string = '';
  @State output: string = '';
  // chat page
  @State msgs: Array<ChatMsg> = [];
  @State lastToks: string = '--';
  @State lastLoad: string = '--';
  @State lastTtft: string = '--';
  @State lastRss: string = '--';
  @State tabIdx: number = 0;
  // L2 perception-driven scheduling: foreground -> full tier (+auto warm-up), background -> release.
  @StorageLink('appFgState') @Watch('onFgChange') appFg: boolean = true;
  @State l2Enabled: boolean = true;
  @State autoWarmup: boolean = true;
  @State l2State: string = '前台 · 满载';
  @State modelLoaded: boolean = false;
  private bgReleaseTimer: number = -1;   // hysteresis: release only after staying backgrounded
  scroller: Scroller = new Scroller();

  aboutToAppear(): void {
    try {
      this.loadInfo = tmac.nativeVersion();
      hilog.info(0x0000, 'LUTSA', 'nativeVersion: %{public}s', this.loadInfo);
    } catch (e) {
      this.loadInfo = `native 模块加载失败: ${e}`;
    }
    // The app sandbox is the only place this process may read the model from on HarmonyOS
    // (/data/local/tmp is blocked by SELinux). chmod it so `hdc file send` can push the file in.
    const fd = this.filesDir();
    if (fd.length > 0) {
      this.modelPath = `${fd}/model.gguf`;
      try {
        this.sandboxInfo = tmac.prepareSandbox(fd);
      } catch (e) {
        this.sandboxInfo = `prepareSandbox 异常: ${e}`;
      }
    }
  }

  private filesDir(): string {
    try {
      const ctx = getContext(this) as common.UIAbilityContext;
      return ctx.filesDir;
    } catch (e) {
      return '';
    }
  }

  private num(s: string, dflt: number): number {
    const v = Number.parseInt(s);
    return Number.isNaN(v) ? dflt : v;
  }

  // status dot color by current status text
  private dotColor(): string {
    if (this.status.indexOf('❌') >= 0 || this.status.indexOf('⚠') >= 0) {
      return C_ERR;
    }
    if (this.status.indexOf('⏳') >= 0 || this.busy) {
      return C_WARN;
    }
    if (this.status.indexOf('✅') >= 0) {
      return C_OK;
    }
    return C_DIM;
  }

  // One rolling console log, with a timestamped separator per operation — so the load report
  // and each generation report stay distinguishable instead of overwriting each other.
  private appendLog(tag: string, text: string): void {
    let stamp = '';
    try {
      stamp = new Date().toTimeString().slice(0, 8);
    } catch (e) {
      stamp = '--:--:--';
    }
    this.output = `${this.output}\n===== ${tag} @ ${stamp} =====\n${text}\n`;
    if (this.output.length > 100000) {
      this.output = this.output.slice(this.output.length - 100000);
    }
  }

  private pushMsg(m: ChatMsg): void {
    this.msgs = this.msgs.concat([m]);
    setTimeout(() => {
      this.scroller.scrollEdge(Edge.Bottom);
    }, 60);
  }

  // The native report is: sampler header, blank line, prompt echo + generated text, then a
  // timing block ("llama_perf_sampler_print" / "[[timings]]" / "llama_perf:"). Cut at the
  // earliest timing marker, strip the header (first blank line) and the echoed prompt.
  private splitReport(r: string, asked: string): string[] {
    let text = r;
    let cut = r.indexOf('llama_perf_sampler_print');
    const alt = r.indexOf('[[timings]]');
    if (alt >= 0 && (cut < 0 || alt < cut)) {
      cut = alt;
    }
    if (cut < 0) {
      cut = r.indexOf('llama_perf:');
    }
    if (cut >= 0) {
      text = r.substring(0, cut);
    }
    const nl = text.indexOf('\n\n');
    if (nl >= 0) {
      text = text.substring(nl + 2);
    }
    if (asked.length > 0 && text.startsWith(asked)) {
      text = text.substring(asked.length);
    }
    text = text.trim();
    let paren = '';
    const lines = r.split('\n');
    for (let i = lines.length - 1; i >= 0; i--) {
      if (lines[i].indexOf('eval time') >= 0) {
        const p = lines[i].indexOf('(');
        const q = lines[i].lastIndexOf(')');
        if (p >= 0 && q > p) {
          paren = lines[i].substring(p + 1, q);
        }
        break;
      }
    }
    let toks = paren;
    const comma = paren.lastIndexOf(',');
    if (comma >= 0) {
      toks = paren.substring(comma + 1).trim().replace('tokens per second', 'tok/s');
    }
    return [text, toks, paren];
  }

  // L2 rule engine with hysteresis: quick app-switch keeps the engine (and its chained KV)
  // alive; only a STAY in background (>60 s) releases it. Foreground reloads only if released.
  private onFgChange(): void {
    if (!this.l2Enabled) {
      return;
    }
    if (!this.appFg) {
      this.l2State = '后台 · 计时降载';
      if (this.bgReleaseTimer >= 0) {
        clearTimeout(this.bgReleaseTimer);
        this.bgReleaseTimer = -1;
      }
      this.bgReleaseTimer = setTimeout(() => {
        this.bgReleaseTimer = -1;
        if (!this.appFg && this.l2Enabled && this.modelLoaded && !this.busy) {
          const r: string = tmac.release();
          this.modelLoaded = false;
          this.appendLog('L2 调度 · 后台停留 >60s → 引擎释放（降载）', r);
          this.status = '🌙 已后台降载（~1GB 内存归还系统）';
          this.lastToks = '--';
          this.lastLoad = '--';
          this.lastTtft = '--';
          this.lastRss = '--';
        }
      }, 60000);
    } else {
      if (this.bgReleaseTimer >= 0) {   // came back within grace: engine + KV chain intact
        clearTimeout(this.bgReleaseTimer);
        this.bgReleaseTimer = -1;
        this.l2State = '前台 · 满载';
        this.appendLog('L2 调度 · 快速切回（<60s）', '引擎与 KV 链保持，未降载');
        return;
      }
      this.l2State = '前台 · 满载';
      this.status = '☀️ 已回前台';
      if (this.autoWarmup && !this.modelLoaded && !this.busy) {
        this.appendLog('L2 调度 · 回前台 → 预热（自动重载模型）', '');
        this.doLoad();
      }
    }
  }

  private doLoad(): void {
    if (this.busy) {
      return;
    }
    this.busy = true;
    this.status = '⏳ 加载中（首次含内核变换，可能几十秒）…';
    this.detail = '';
    tmac.loadModelAsync(this.modelPath, this.filesDir(), this.num(this.threads, 4), 1024, this.useMmap)
      .then((r: string) => {
        this.status = r.startsWith('OK') || r.includes('llm_load_print_meta')
                        ? '✅ 模型已加载' : '❌ 加载失败';
        this.modelLoaded = r.includes('llm_load_print_meta');
        this.detail = '';
        const lines = r.split('\n');
        for (let i = 0; i < lines.length; i++) {
          const li = lines[i].indexOf('load time');
          const le = lines[i].indexOf('load=');
          if (li >= 0 || le >= 0) {
            const nums = lines[i].replace(/[^0-9.]/g, ' ').trim().split(' ');
            const ms = this.num(nums[0], 0);
            if (ms > 0) {
              this.lastLoad = `${(ms / 1000).toFixed(2)}s`;
            }
            break;
          }
        }
        if (r.includes('llm_load_print_meta')) {
          this.appendLog('加载模型 · 完整日志', r);   // full llama.cpp verbose report
        } else {
          this.appendLog('加载模型 · 失败', r);
        }
        hilog.info(0x0000, 'LUTSA', 'loadModel: %{public}s', r);
      })
      .catch((e: Object) => {
        this.status = '❌ 加载异常';
        this.detail = `loadModelAsync 异常: ${e}`;
      })
      .finally(() => { this.busy = false; });
  }

  // Product-grade model import: system file picker -> copy into the app sandbox -> load.
  private async pickAndImportModel(): Promise<void> {
    if (this.busy) {
      return;
    }
    try {
      const ctx = getContext(this) as common.UIAbilityContext;
      const doc = new picker.DocumentViewPicker(ctx);
      const uris = await doc.select({ maxSelectNumber: 1, fileSuffixFilters: ['.gguf'] } as picker.DocumentSelectOptions);
      if (!uris || uris.length === 0) {
        this.status = '已取消选择';
        return;
      }
      this.busy = true;
      this.status = '⏳ 正在把模型拷进应用沙箱…（请保持应用在前台）';
      const dest = `${ctx.filesDir}/model.gguf`;
      const srcFile = fs.openSync(uris[0], fs.OpenMode.READ_ONLY);
      const dstFile = fs.openSync(dest, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC);
      const srcSize = fs.statSync(srcFile.fd).size;
      const copied = await this.copyWithGuard(srcFile.fd, dstFile.fd);
      const dstSize = fs.statSync(dstFile.fd).size;
      fs.closeSync(srcFile);
      fs.closeSync(dstFile);
      if (dstSize !== srcSize) {
        // truncated copy (usually out of disk space) — do NOT load a broken model
        this.status = `⚠️ 导入不完整：${Math.round(dstSize / 1048576)} / ${Math.round(srcSize / 1048576)} MB`;
        this.detail = `${copied}\n文件没拷全（多半是设备存储空间不足，请清理后重试）。已保留原模型未变。`;
        hilog.error(0x0000, 'LUTSA', 'import truncated: %{public}d of %{public}d', dstSize, srcSize);
        return;
      }
      this.modelPath = dest;
      this.status = `✅ 模型已导入（${Math.round(dstSize / 1048576)} MB）`;
      this.detail = `source: ${uris[0]}\ndest:   ${dest}\n${copied}`;
      hilog.info(0x0000, 'LUTSA', 'imported model, %{public}d MB', Math.round(dstSize / 1048576));
      this.busy = false;
      this.doLoad();   // chain straight into loading
    } catch (e) {
      this.busy = false;
      this.status = '❌ 导入失败';
      this.detail = `文件选择/拷贝异常: ${e}`;
      hilog.error(0x0000, 'LUTSA', 'import failed: %{public}s', JSON.stringify(e));
    }
  }

  // Big file copies must not run on the JS thread (a synchronous 1 GB copy freezes the UI and the
  // system kills the app: appfreeze / THREAD_BLOCK_6S). Copy on a NAPI worker thread instead, and
  // hold a transient task so the process survives going to the background.
  private async copyWithGuard(srcFd: number, dstFd: number): Promise<string> {
    let requestId = -1;
    try {
      const info = backgroundTaskManager.requestSuspendDelay('model-copy', () => {
        hilog.warn(0x0000, 'LUTSA', 'suspend delay expired during copy');
      });
      requestId = info.requestId;
    } catch (e) {
      hilog.warn(0x0000, 'LUTSA', 'requestSuspendDelay failed: %{public}s', JSON.stringify(e));
    }
    try {
      return await tmac.copyFdAsync(srcFd, dstFd);
    } finally {
      if (requestId >= 0) {
        try {
          backgroundTaskManager.cancelSuspendDelay(requestId);
        } catch (e) {
          hilog.warn(0x0000, 'LUTSA', 'cancelSuspendDelay failed');
        }
      }
    }
  }

  // Export the sandbox model to phone storage via the system SAVE dialog.
  private async exportModel(): Promise<void> {
    if (this.busy) {
      return;
    }
    try {
      const ctx = getContext(this) as common.UIAbilityContext;
      const src = `${ctx.filesDir}/model.gguf`;
      const st = fs.statSync(src);
      if (st.size <= 0) {
        this.status = '⚠️ 沙箱里没有模型可导出';
        return;
      }
      const doc = new picker.DocumentViewPicker(ctx);
      const fileName = `bitnet-3b-tmac-${Date.now()}.gguf`;   // unique: save dialog refuses overwrite
      const uris = await doc.save({ newFileNames: [fileName] } as picker.DocumentSaveOptions);
      if (!uris || uris.length === 0) {
        this.status = '已取消导出';
        return;
      }
      this.busy = true;
      this.status = `⏳ 正在导出模型（${Math.round(st.size / 1024 / 1024)} MB，请保持应用在前台）…`;
      const srcFile = fs.openSync(src, fs.OpenMode.READ_ONLY);
      const dstFile = fs.openSync(uris[0], fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC);
      const srcSize = fs.statSync(srcFile.fd).size;
      const copied = await this.copyWithGuard(srcFile.fd, dstFile.fd);
      const dstSize = fs.statSync(dstFile.fd).size;
      fs.closeSync(srcFile);
      fs.closeSync(dstFile);
      if (dstSize !== srcSize) {
        this.status = `⚠️ 导出不完整：${Math.round(dstSize / 1048576)} / ${Math.round(srcSize / 1048576)} MB`;
        this.detail = `${copied}\n多半是目标存储空间不足。`;
        return;
      }
      this.status = `✅ 模型已导出（${Math.round(dstSize / 1048576)} MB）`;
      this.detail = `dest: ${uris[0]}\n${copied}\n可用「📂 选择模型文件」选回来（验证导入链路）`;
      hilog.info(0x0000, 'LUTSA', 'exported model, %{public}d MB', Math.round(dstSize / 1048576));
    } catch (e) {
      this.status = '❌ 导出失败';
      this.detail = `导出异常: ${e}`;
      hilog.error(0x0000, 'LUTSA', 'export failed: %{public}s', JSON.stringify(e));
    } finally {
      this.busy = false;
    }
  }

  // first number on the line containing `key` (e.g. "ttft =  123.45 ms" -> "123.45 ms")
  private extractNum(r: string, key: string): string {
    const lines = r.split('\n');
    for (let i = lines.length - 1; i >= 0; i--) {
      const k = lines[i].indexOf(key);
      if (k >= 0) {
        const after = lines[i].substring(k + key.length).replace(/[=]/g, ' ').trim();
        const parts = after.split(/\s+/);
        if (parts.length >= 2) {
          return `${parts[0]} ${parts[1]}`;
        }
      }
    }
    return '';
  }

  private doGenerate(): void {
    if (this.busy) {
      return;
    }
    if (this.prompt.length === 0) {
      this.status = '⚠️ 请先输入提示词';
      return;
    }
    this.busy = true;
    this.status = '⏳ 推理中…';
    this.output = '';   // each generation starts a fresh console report
    const asked = this.prompt;
    this.pushMsg({ role: 'user', text: asked, meta: '' });
    const streamIdx = this.msgs.length;   // the model bubble that will type live
    this.pushMsg({ role: 'model', text: '', meta: '' });
    const onToken = (piece: string): void => {
      // reassign the array (not just the element) so @State refreshes per token
      const arr = this.msgs.slice();
      arr[streamIdx] = { role: 'model', text: arr[streamIdx].text + piece, meta: '' };
      this.msgs = arr;
      this.status = `⏳ 生成中… 已输出 ${arr[streamIdx].text.length} 字`;   // live progress
    };
    tmac.generateStreamAsync(asked, this.num(this.nPredict, 16), 0.8, 40, onToken)
      .then((r: string) => {
        this.status = '✅ 生成完成';
        const parts = this.splitReport(r, asked);
        this.lastToks = parts[1].length > 0 ? parts[1] : '--';
        const ttft = this.extractNum(r, 'ttft');
        const rss = this.extractNum(r, 'peak_rss');
        if (ttft.length > 0) {
          this.lastTtft = ttft.replace('ms', '').trim() + ' ms';
        }
        if (rss.length > 0) {
          this.lastRss = rss.replace('MB', '').trim() + ' MB';
        }
        // finalize the streamed bubble: clean continuation text + perf caption
        const arr = this.msgs.slice();
        const streamed = arr[streamIdx].text;
        let finalText = parts[0].length > 0 ? parts[0] : streamed;
        if (finalText.length === 0) {
          // diagnosis: never leave the user with a silent empty bubble
          finalText = r.length > 0
            ? `（无续写输出，原始响应前 400 字：）\n${r.substring(0, 400)}`
            : '（native 返回空响应——请到「控制台」页查看日志）';
        }
        arr[streamIdx] = { role: 'model', text: finalText, meta: parts[2] };
        this.msgs = arr;
        this.appendLog('生成 · 完整报告', r);   // console page keeps the verbatim report
        hilog.info(0x0000, 'LUTSA', 'generate done, %{public}d chars', r.length);
      })
      .catch((e: Object) => {
        this.status = '❌ 推理异常';
        this.pushMsg({ role: 'model', text: `${e}`, meta: '异常' });
        this.appendLog('生成 · 异常', `${e}`);
      })
      .finally(() => { this.busy = false; });
  }

  // ---------------- small building blocks ----------------

  @Builder
  chip(vm: ChipVM) {
    Column({ space: 2 }) {
      Text(vm.label)
        .fontSize(10).fontColor(C_DIM).textAlign(TextAlign.Center)
      Text(vm.value)
        .fontSize(15).fontWeight(FontWeight.Bold).fontColor(C_ACCENT)
        .maxLines(1).textOverflow({ overflow: TextOverflow.Ellipsis })
    }
    .justifyContent(FlexAlign.Center)
    .layoutWeight(vm.wide ? 1.4 : 1)
    .height(52)
    .backgroundColor(BG_CARD)
    .borderRadius(10)
    .border({ width: 1, color: C_BORDER })
  }

  @Builder
  cardTitle(icon: string, t: string) {
    Text(`${icon} ${t}`)
      .fontSize(13).fontWeight(FontWeight.Medium).fontColor(C_SUB)
      .width('100%').textAlign(TextAlign.Start)
  }

  @Builder
  bubble(m: ChatMsg) {
    Column({ space: 4 }) {
      Text(m.text)
        .fontSize(15)
        .fontColor(m.role === 'user' ? '#FFFFFF' : C_TEXT)
        .textAlign(TextAlign.Start)
      if (m.meta.length > 0) {
        Text(m.meta)
          .fontSize(10).fontColor(C_DIM).textAlign(TextAlign.End)
      }
    }
    .constraintSize({ maxWidth: '82%' })
    .padding(12)
    .backgroundColor(m.role === 'user' ? '#2E6DA8' : BG_CARD)
    .borderRadius(14)
    .border({ width: 1, color: m.role === 'user' ? '#3D7FC0' : C_BORDER })
  }

  build() {
    Column({ space: 0 }) {
      // ---- header ----
      Row({ space: 8 }) {
        Text('鸿蒙玲珑核')
          .fontSize(21).fontWeight(FontWeight.Bold).fontColor(C_ACCENT)
        Text('LUT-SA · 低比特 LLM · 鸿蒙原生')
          .fontSize(10).fontColor(C_SUB).layoutWeight(1).maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        Column()
          .width(9).height(9).borderRadius(5).backgroundColor(this.dotColor())
      }
      .width('100%')
      .padding({ left: 16, right: 16, top: 12, bottom: 6 })

      Row() {
        Text(this.loadInfo)
          .fontSize(10).fontColor(C_DIM).layoutWeight(1).maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        Text(this.status)
          .fontSize(11).fontColor(C_SUB).maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .width('100%')
      .padding({ left: 16, right: 16, bottom: 8 })

      // ---- three pages ----
      Tabs({ index: this.tabIdx, barPosition: BarPosition.Start }) {
        // ============ page 1: chat ============
        TabContent() {
          Column({ space: 10 }) {
            Row({ space: 6 }) {
              this.chip({ label: '生成速度', value: this.lastToks, wide: true })
              this.chip({ label: 'TTFT', value: this.lastTtft, wide: false })
              this.chip({ label: '加载', value: this.lastLoad, wide: false })
              this.chip({ label: '峰值内存', value: this.lastRss, wide: false })
            }
            .width('92%')

            Scroll(this.scroller) {
              Column({ space: 10 }) {
                if (this.msgs.length === 0) {
                  Column({ space: 6 }) {
                    Text('💬')
                      .fontSize(34)
                    Text('加载模型后，输入提示词开始对话')
                      .fontSize(13).fontColor(C_SUB)
                    Text('base 模型：按提示词续写（非问答）；英文语料为主')
                      .fontSize(10).fontColor(C_DIM)
                  }
                  .width('100%')
                  .padding({ top: 60, bottom: 20 })
                  .alignItems(HorizontalAlign.Center)
                }
                ForEach(this.msgs, (m: ChatMsg) => {
                  Row() {
                    if (m.role === 'user') {
                      Blank()
                      this.bubble(m)
                    } else {
                      this.bubble(m)
                      Blank()
                    }
                  }
                  .width('100%')
                }, (m: ChatMsg, idx: number) => `${idx}-${m.role}-${m.text.length}-${m.meta.length}`)
              }
              .width('92%')
              .alignItems(HorizontalAlign.Start)
            }
            .layoutWeight(1)
            .width('100%')
            .align(Alignment.Center)
            .scrollBar(BarState.Auto)

            Row({ space: 8 }) {
              TextInput({ text: this.prompt, placeholder: '输入提示词…' })
                .layoutWeight(1)
                .height(46)
                .fontSize(14)
                .fontColor('#FFFFFF')
                .placeholderColor(C_DIM)
                .backgroundColor(BG_INPUT)
                .borderRadius(10)
                .border({ width: 1.2, color: this.busy ? C_BORDER : C_ACCENT })
                .padding({ left: 12, right: 12 })
                .enterKeyType(EnterKeyType.Go)
                .enabled(!this.busy)
                .onChange((v: string) => { this.prompt = v; })
                .onSubmit(() => { this.doGenerate(); })
              Button(this.busy ? '…' : '生成')
                .width(76).height(46).fontSize(14)
                .backgroundColor(this.busy ? C_BORDER : C_ACCENT)
                .borderRadius(10)
                .enabled(!this.busy)
                .onClick(() => { this.doGenerate(); })
            }
            .width('92%')
          }
          .width('100%').height('100%')
          .padding({ bottom: 10 })
        }
        .tabBar('对话')

        // ============ page 2: console ============
        TabContent() {
          Column({ space: 8 }) {
            Scroll() {
              Text((this.detail.length > 0 ? `» ${this.detail}\n\n` : '') +
                   (this.output.length > 0
                      ? this.output
                      : '（控制台输出：加载模型 → 完整日志；生成 → 文本与计时；内核自测/基准 → 内核结果。与 llama.cpp CLI 同格式，可直接对拍）'))
                .fontSize(11)
                .fontFamily('monospace')
                .textAlign(TextAlign.Start)
                .fontColor('#CFE0F5')
            }
            .width('94%')
            .layoutWeight(1)
            .padding(10)
            .backgroundColor('#161D30')
            .borderRadius(10)
            .border({ width: 1, color: C_BORDER })
          }
          .width('100%').height('100%')
          .padding({ top: 8, bottom: 10 })
        }
        .tabBar('控制台')

        // ============ page 3: model ============
        TabContent() {
          Scroll() {
            Column({ space: 12 }) {
              // -- model files --
              Column({ space: 10 }) {
                this.cardTitle('📦', '模型文件')
                TextInput({ text: this.modelPath })
                  .width('100%').height(42).fontSize(11).fontFamily('monospace')
                  .fontColor('#CFE0F5').backgroundColor(BG_INPUT)
                  .borderRadius(8).border({ width: 1, color: C_BORDER })
                  .padding({ left: 10, right: 10 })
                  .onChange((v: string) => { this.modelPath = v; })
                Row({ space: 8 }) {
                  Button('📂 选择模型文件')
                    .height(40).layoutWeight(1).fontSize(13).enabled(!this.busy)
                    .backgroundColor(BG_CARD).fontColor(C_TEXT)
                    .borderRadius(8).border({ width: 1, color: C_BORDER })
                    .onClick(() => { this.pickAndImportModel(); })
                  Button('📤 导出到手机')
                    .height(40).layoutWeight(1).fontSize(13).enabled(!this.busy)
                    .backgroundColor(BG_CARD).fontColor(C_TEXT)
                    .borderRadius(8).border({ width: 1, color: C_BORDER })
                    .onClick(() => { this.exportModel(); })
                }
                .width('100%')
                Button('⚡ 加载模型')
                  .width('100%').height(44).fontSize(14).enabled(!this.busy)
                  .backgroundColor(C_ACCENT).fontColor('#0B1526')
                  .borderRadius(10)
                  .onClick(() => { this.doLoad(); })
                Toggle({ type: ToggleType.Switch, isOn: this.useMmap })
                  .width('100%')
                  .height(30)
                  .padding({ left: 12, right: 12 })
                  .onChange((v: boolean) => { this.useMmap = v; })
              }
              .width('92%')
              .padding(14)
              .backgroundColor(BG_CARD)
              .borderRadius(12)
              .border({ width: 1, color: C_BORDER })
              .alignItems(HorizontalAlign.Start)

              // -- params --
              Column({ space: 10 }) {
                this.cardTitle('⚙️', '推理参数')
                Row({ space: 10 }) {
                  Column({ space: 4 }) {
                    Text('线程').fontSize(11).fontColor(C_SUB)
                    TextInput({ text: this.threads })
                      .width(64).height(40).fontSize(13).textAlign(TextAlign.Center)
                      .fontColor(C_TEXT).backgroundColor(BG_INPUT).borderRadius(8)
                      .border({ width: 1, color: C_BORDER })
                      .onChange((v: string) => { this.threads = v; })
                  }
                  Column({ space: 4 }) {
                    Text('生成长度').fontSize(11).fontColor(C_SUB)
                    TextInput({ text: this.nPredict })
                      .width(72).height(40).fontSize(13).textAlign(TextAlign.Center)
                      .fontColor(C_TEXT).backgroundColor(BG_INPUT).borderRadius(8)
                      .border({ width: 1, color: C_BORDER })
                      .onChange((v: string) => { this.nPredict = v; })
                  }
                  Blank()
                }
                .width('100%')
              }
              .width('92%')
              .padding(14)
              .backgroundColor(BG_CARD)
              .borderRadius(12)
              .border({ width: 1, color: C_BORDER })
              .alignItems(HorizontalAlign.Start)

              // -- L2 perception & scheduling --
              Column({ space: 10 }) {
                this.cardTitle('🧠', '感知调度（L2）')
                Row({ space: 8 }) {
                  Text(this.l2State)
                    .fontSize(13).fontWeight(FontWeight.Medium)
                    .fontColor(this.appFg ? C_OK : C_WARN)
                    .layoutWeight(1)
                  Column() {
                    Text('启用调度').fontSize(11).fontColor(C_SUB)
                    Toggle({ type: ToggleType.Switch, isOn: this.l2Enabled })
                      .onChange((v: boolean) => { this.l2Enabled = v; })
                  }
                  Column({ space: 4 }) {
                    Text('回前台预热').fontSize(11).fontColor(C_SUB)
                    Toggle({ type: ToggleType.Switch, isOn: this.autoWarmup })
                      .onChange((v: boolean) => { this.autoWarmup = v; })
                  }
                }
                .width('100%')
                Text('规则：切后台 → 释放引擎（约 1GB 内存归还系统，生成中不打断）；回前台 → 自动重载模型（约 2s 恢复满载）。关闭开关即得「无调度」对照组，可直接演示差异。')
                  .fontSize(10).fontColor(C_DIM).width('100%').textAlign(TextAlign.Start)
              }
              .width('92%')
              .padding(14)
              .backgroundColor(BG_CARD)
              .borderRadius(12)
              .border({ width: 1, color: C_BORDER })
              .alignItems(HorizontalAlign.Start)

              // -- kernel tools --
              Column({ space: 10 }) {
                this.cardTitle('🔬', '内核工具（不依赖模型）')
                Row({ space: 8 }) {
                  Button('自测')
                    .height(38).layoutWeight(1).fontSize(13).enabled(!this.busy)
                    .backgroundColor(BG_INPUT).fontColor(C_TEXT)
                    .borderRadius(8).border({ width: 1, color: C_BORDER })
                    .onClick(() => {
                      const r: string = tmac.selfTest();
                      this.status = r.includes('PASS') ? '✅ 内核自测通过' : '❌ 内核自测失败';
                      this.appendLog('内核自测', r);
                    })
                  Button('基准')
                    .height(38).layoutWeight(1).fontSize(13).enabled(!this.busy)
                    .backgroundColor(BG_INPUT).fontColor(C_TEXT)
                    .borderRadius(8).border({ width: 1, color: C_BORDER })
                    .onClick(() => {
                      const r: string = tmac.bench(200);
                      this.appendLog('内核基准', r);
                      this.status = '⏱ 基准完成';
                    })
                  Button('释放')
                    .height(38).layoutWeight(1).fontSize(13).enabled(!this.busy)
                    .backgroundColor(BG_INPUT).fontColor(C_TEXT)
                    .borderRadius(8).border({ width: 1, color: C_BORDER })
                    .onClick(() => {
                      const r: string = tmac.release();
                      this.modelLoaded = false;
                      this.appendLog('释放模型', r);
                      this.status = '已释放模型（内存已回收）';
                      this.lastToks = '--';
                      this.lastLoad = '--';
                      this.lastTtft = '--';
                      this.lastRss = '--';
                    })
                }
                .width('100%')
                Text('自测/基准直接调用 LUT 内核验证正确性与速度；模型加载失败时可用它排查内核本身。')
                  .fontSize(10).fontColor(C_DIM).width('100%').textAlign(TextAlign.Start)
              }
              .width('92%')
              .padding(14)
              .backgroundColor(BG_CARD)
              .borderRadius(12)
              .border({ width: 1, color: C_BORDER })
              .alignItems(HorizontalAlign.Start)

              Text(this.sandboxInfo)
                .fontSize(10).fontColor(C_DIM).maxLines(2)
                .textOverflow({ overflow: TextOverflow.Ellipsis })
                .width('92%')
            }
            .width('100%')
          }
          .width('100%').height('100%')
        }
        .tabBar('模型')
      }
      .layoutWeight(1)
      .barMode(BarMode.Fixed)
      .onChange((i: number) => { this.tabIdx = i; })
    }
    .width('100%')
    .height('100%')
    .backgroundColor(BG_PAGE)
  }
}
