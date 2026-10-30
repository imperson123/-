#ifndef OHOS_LUTSA_LUTSASTUB_H
#define OHOS_LUTSA_LUTSASTUB_H

#include <iremote_stub.h>
#include "ilut_sa.h"

namespace OHOS {
namespace LutSa {

class LutSaStub : public IRemoteStub<ILutSa> {
public:
    LutSaStub(bool serialInvokeFlag = false): IRemoteStub(serialInvokeFlag){};
    int32_t OnRemoteRequest(
        uint32_t code,
        MessageParcel& data,
        MessageParcel& reply,
        MessageOption& option) override;
};
} // namespace LutSa
} // namespace OHOS
#endif // OHOS_LUTSA_LUTSASTUB_H

