#ifndef OHOS_LUTSA_LUTSAPROXY_H
#define OHOS_LUTSA_LUTSAPROXY_H

#include <iremote_proxy.h>
#include "ilut_sa.h"

namespace OHOS {
namespace LutSa {

class LutSaProxy : public IRemoteProxy<ILutSa> {
public:
    explicit LutSaProxy(
        const sptr<IRemoteObject>& remote)
        : IRemoteProxy<ILutSa>(remote)
    {}

    virtual ~LutSaProxy()
    {}

    ErrCode NativeVersion(
        std::string& funcResult) override;

    ErrCode LoadModel(
        const std::string& path,
        int32_t threads,
        int32_t nCtx) override;

    ErrCode Generate(
        const std::string& prompt,
        int32_t nPredict,
        double temp,
        int32_t topK,
        std::string& funcResult) override;

    ErrCode GetMetrics(
        std::string& funcResult) override;

    ErrCode SelfTest(
        std::string& funcResult) override;

    ErrCode Release(
        std::string& funcResult) override;

private:
    static inline BrokerDelegator<LutSaProxy> delegator_;
};
} // namespace LutSa
} // namespace OHOS
#endif // OHOS_LUTSA_LUTSAPROXY_H

