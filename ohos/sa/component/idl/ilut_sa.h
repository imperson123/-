#ifndef OHOS_LUTSA_ILUTSA_H
#define OHOS_LUTSA_ILUTSA_H

#include <cstdint>
#include <iremote_broker.h>
#include <string_ex.h>

namespace OHOS {
namespace LutSa {

enum class ILutSaIpcCode {
    COMMAND_NATIVE_VERSION = MIN_TRANSACTION_ID,
    COMMAND_LOAD_MODEL,
    COMMAND_GENERATE,
    COMMAND_GET_METRICS,
    COMMAND_SELF_TEST,
    COMMAND_RELEASE,
};

class ILutSa : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.LutSa.ILutSa");

    virtual ErrCode NativeVersion(
        std::string& funcResult) = 0;

    virtual ErrCode LoadModel(
        const std::string& path,
        int32_t threads,
        int32_t nCtx) = 0;

    virtual ErrCode Generate(
        const std::string& prompt,
        int32_t nPredict,
        double temp,
        int32_t topK,
        std::string& funcResult) = 0;

    virtual ErrCode GetMetrics(
        std::string& funcResult) = 0;

    virtual ErrCode SelfTest(
        std::string& funcResult) = 0;

    virtual ErrCode Release(
        std::string& funcResult) = 0;
protected:
    const int VECTOR_MAX_SIZE = 102400;
    const int LIST_MAX_SIZE = 102400;
    const int SET_MAX_SIZE = 102400;
    const int MAP_MAX_SIZE = 102400;
};
} // namespace LutSa
} // namespace OHOS
#endif // OHOS_LUTSA_ILUTSA_H

