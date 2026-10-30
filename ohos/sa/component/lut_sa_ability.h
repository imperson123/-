// LUT-SA SystemAbility 绑定：把业务内核注册进 samgr（SA_ID=6901）。
// 编译条件 TMAC_SA_SAMGR_BINDING（需要 OH 平台源码树头文件；应用 SDK 不提供）。
// 已用 OH 平台头文件（safwk/samgr/ipc/c_utils/hilog）+ SDK clang 完成编译级验证；
// ILutSa 的 C++ 桩由 SDK 的 idl.exe 生成（--intf-type sa -c ILutSa.idl --gen-cpp）。
#ifdef TMAC_SA_SAMGR_BINDING

#ifndef LUT_SA_ABILITY_H
#define LUT_SA_ABILITY_H

#include <cstdint>
#include <string>
#include <vector>

#include "system_ability.h"   // OHOS::SystemAbility（safwk）
#include "lut_sa_stub.h"      // OHOS::LutSa::LutSaStub（由 ILutSa.idl 生成）

namespace OHOS {
class LutSystemAbility : public SystemAbility, public LutSa::LutSaStub {
    DECLARE_SYSTEM_ABILITY(LutSystemAbility);

public:
    LutSystemAbility(int32_t saId, bool runOnCreate)
        : SystemAbility(saId, runOnCreate), LutSa::LutSaStub(false) {}
    ~LutSystemAbility() override = default;

    // 生命周期：OnStart 里 Publish 自身（samgr 登记）；OnStop 里回收会话
    void OnStart() override;
    void OnStop() override;

    // hidumper -s 6901 的输出来源（该 OH 版本的 SA dump 钩子）
    int32_t OnSvcCmd(int32_t fd, const std::vector<std::u16string> &args) override;

    // ILutSa 的六个 IPC 入口（薄封装到业务内核 ::tmac_sa::*）
    ErrCode NativeVersion(std::string &result) override;
    ErrCode LoadModel(const std::string &path, int32_t threads, int32_t nCtx) override;
    ErrCode Generate(const std::string &prompt, int32_t nPredict, double temp, int32_t topK,
                     std::string &result) override;
    ErrCode GetMetrics(std::string &result) override;
    ErrCode SelfTest(std::string &result) override;
    ErrCode Release(std::string &result) override;
};
}  // namespace OHOS

#endif  // LUT_SA_ABILITY_H
#endif  // TMAC_SA_SAMGR_BINDING
