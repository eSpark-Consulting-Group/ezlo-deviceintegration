#pragma once

#include "networkd/gsm/nm_gsm.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Network {
namespace Mobile {

using namespace GSM;

class ModemInfoProxy final {
public:
    void setSignalQuality( GSMModem::SignalQuality signalQuality );
    Json getStabilizedSignalQuality() const;

    void setAccessTechnology( GSMModem::AccessTechnology accessTechnology );
    GSMModem::AccessTechnology getCachedAccessTechnology() const;

    void setFrequencyBand( GSMModem::FrequencyBand frequencyBand );
    GSMModem::FrequencyBand getCachedFrequencyBand() const;

    static Json getSignalQuality( const GSM::GSMModem::SignalQuality& signalQuality );

private:
    GSMModem::SignalQuality m_signalQuality;
    GSMModem::AccessTechnology m_accessTechnology;
    GSMModem::FrequencyBand m_frequencyBand;
};

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
