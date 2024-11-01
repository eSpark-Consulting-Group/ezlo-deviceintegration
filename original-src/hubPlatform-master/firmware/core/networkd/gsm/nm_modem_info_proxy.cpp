#include "networkd/gsm/nm_modem_info_proxy.h"

#include "json_utils/json_wrapper.h"

#include <cmath>

namespace HubPlatform {
namespace Network {
namespace Mobile {

using namespace GSM;

namespace {
const int signalQualityLevelThresholdLTE = 15;
const int signalQualityLevelThresholdNonLTE = 15;

int evaluateLevel( const GSMModem::SignalQuality& signalQuality ) {
    int level;

    if( signalQuality.m_accessTechnology.isLTE() ) {
        // for LTE based networks signal level is calculated based on an rsrp (-110 dBm <= rsrp <= -70 dBm)
        level = 99.0 * ( signalQuality.m_rsrp + 110 ) / 40.0;
        level = level > 100 ? 100 : level;
        return level >= 0 ? level : 0;
    }
    else {
        // for non LTE based networks signal level is calculated based on an rssi (-103 dBm <= rssi <= -61 dBm)
        level = 99.0 * ( signalQuality.m_rssi + 103 ) / 42.0;
        level = level > 100 ? 100 : level;
        return level >= 0 ? level : 0;
    }
}

bool isAccessTechnologyEqual( const GSMModem::SignalQuality& s, const GSM::GSMModem::SignalQuality& q ) {
    return s.m_accessTechnology.m_accessTechnology == q.m_accessTechnology.m_accessTechnology;
}

int getsignalQualityLevelThreshold( const GSMModem::SignalQuality& signalQuality ) {
    return signalQuality.m_accessTechnology.isLTE() ? signalQualityLevelThresholdLTE :
                                                      signalQualityLevelThresholdNonLTE;
}
}  // namespace

void ModemInfoProxy::setSignalQuality( GSMModem::SignalQuality signalQuality ) {
    if( !signalQuality ) {
        return;
    }

    if( !m_signalQuality || !isAccessTechnologyEqual( signalQuality, m_signalQuality ) ) {
        m_signalQuality = std::move( signalQuality );
    }
    else {
        if( std::abs( evaluateLevel( signalQuality ) - evaluateLevel( m_signalQuality ) ) >=
            getsignalQualityLevelThreshold( m_signalQuality ) ) {
            m_signalQuality = std::move( signalQuality );
        }
    }
}

Json ModemInfoProxy::getStabilizedSignalQuality() const {
    return getSignalQuality( m_signalQuality );
}

void ModemInfoProxy::setAccessTechnology( GSMModem::AccessTechnology accessTechnology ) {
    m_accessTechnology = std::move( accessTechnology );
}

GSMModem::AccessTechnology ModemInfoProxy::getCachedAccessTechnology() const {
    return m_accessTechnology;
}

void ModemInfoProxy::setFrequencyBand( GSMModem::FrequencyBand frequencyBand ) {
    m_frequencyBand = std::move( frequencyBand );
}

GSMModem::FrequencyBand ModemInfoProxy::getCachedFrequencyBand() const {
    return m_frequencyBand;
}

Json ModemInfoProxy::getSignalQuality( const GSMModem::SignalQuality& signalQuality ) {
    Json result = JSON_EMPTY_OBJ;

    if( signalQuality ) {
        result = JSON( "rssi" << signalQuality.m_rssi << "level" << evaluateLevel( signalQuality ) );

        if( signalQuality.m_accessTechnology.isLTE() ) {
            result[ "rsrp" ] = signalQuality.m_rsrp;
            result[ "rsrq" ] = signalQuality.m_rsrq;
        }
    }

    return result;
}

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
