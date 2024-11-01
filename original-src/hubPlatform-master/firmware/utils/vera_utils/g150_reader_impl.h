#pragma once
#include "vera_utils/provision_reader_base.h"

namespace HubPlatform {
namespace VeraProvision {

class G150ReaderImpl : public ProvisionReaderBase {
protected:
    ValueInfo getInfoMacAddress() const override;
    ValueInfo getInfoSerialNumber() const override;
    ValueInfo getInfoLanguage() const override;
    ValueInfo getInfoWifiDomain() const override;
    ValueInfo getInfoPassPhrase() const override;
    ValueInfo getInfoWifiSsid() const override;
    ValueInfo getInfoHwKey() const override;
};
}  // namespace VeraProvision
}  // namespace HubPlatform
