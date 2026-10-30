#include "lut_sa_proxy.h"

namespace OHOS {
namespace LutSa {

ErrCode LutSaProxy::NativeVersion(
    std::string& funcResult)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return ERR_INVALID_VALUE;
    }

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(ILutSaIpcCode::COMMAND_NATIVE_VERSION), data, reply, option);
    if (FAILED(result)) {
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        return errCode;
    }

    funcResult = Str16ToStr8(reply.ReadString16());
    return ERR_OK;
}

ErrCode LutSaProxy::LoadModel(
    const std::string& path,
    int32_t threads,
    int32_t nCtx)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return ERR_INVALID_VALUE;
    }

    if (!data.WriteString16(Str8ToStr16(path))) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteInt32(threads)) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteInt32(nCtx)) {
        return ERR_INVALID_DATA;
    }

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(ILutSaIpcCode::COMMAND_LOAD_MODEL), data, reply, option);
    if (FAILED(result)) {
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        return errCode;
    }

    return ERR_OK;
}

ErrCode LutSaProxy::Generate(
    const std::string& prompt,
    int32_t nPredict,
    double temp,
    int32_t topK,
    std::string& funcResult)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return ERR_INVALID_VALUE;
    }

    if (!data.WriteString16(Str8ToStr16(prompt))) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteInt32(nPredict)) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteDouble(temp)) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteInt32(topK)) {
        return ERR_INVALID_DATA;
    }

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(ILutSaIpcCode::COMMAND_GENERATE), data, reply, option);
    if (FAILED(result)) {
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        return errCode;
    }

    funcResult = Str16ToStr8(reply.ReadString16());
    return ERR_OK;
}

ErrCode LutSaProxy::GetMetrics(
    std::string& funcResult)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return ERR_INVALID_VALUE;
    }

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(ILutSaIpcCode::COMMAND_GET_METRICS), data, reply, option);
    if (FAILED(result)) {
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        return errCode;
    }

    funcResult = Str16ToStr8(reply.ReadString16());
    return ERR_OK;
}

ErrCode LutSaProxy::SelfTest(
    std::string& funcResult)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return ERR_INVALID_VALUE;
    }

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(ILutSaIpcCode::COMMAND_SELF_TEST), data, reply, option);
    if (FAILED(result)) {
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        return errCode;
    }

    funcResult = Str16ToStr8(reply.ReadString16());
    return ERR_OK;
}

ErrCode LutSaProxy::Release(
    std::string& funcResult)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return ERR_INVALID_VALUE;
    }

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(ILutSaIpcCode::COMMAND_RELEASE), data, reply, option);
    if (FAILED(result)) {
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        return errCode;
    }

    funcResult = Str16ToStr8(reply.ReadString16());
    return ERR_OK;
}
} // namespace LutSa
} // namespace OHOS
