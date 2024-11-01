#include "vera_utils/g150_reader_impl.h"

namespace HubPlatform {
namespace VeraProvision {

ProvisionReaderBase::ValueInfo G150ReaderImpl::getInfoMacAddress() const {
    return { 0x8000, 0x10 };
}

ProvisionReaderBase::ValueInfo G150ReaderImpl::getInfoSerialNumber() const {
    return { 0x8010, 0x30 };
}

ProvisionReaderBase::ValueInfo G150ReaderImpl::getInfoLanguage() const {
    return { 0x8050, 0x10 };
}

ProvisionReaderBase::ValueInfo G150ReaderImpl::getInfoWifiDomain() const {
    return { 0x8060, 0x10 };
}

ProvisionReaderBase::ValueInfo G150ReaderImpl::getInfoPassPhrase() const {
    return { 0x80A0, 0x40 };
}

ProvisionReaderBase::ValueInfo G150ReaderImpl::getInfoWifiSsid() const {
    return { 0x8120, 0x20 };
}

ProvisionReaderBase::ValueInfo G150ReaderImpl::getInfoHwKey() const {
    return { 0x8140, 0x40 };
}

}  // namespace VeraProvision

}  // namespace HubPlatform
