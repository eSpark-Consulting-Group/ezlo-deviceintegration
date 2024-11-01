#pragma once

#include "opkg_utils/feeds/feeds_configuration.h"
#include <memory>

namespace HubPlatform {
namespace Opkg {

class IFeedsUpdater {
public:
    enum class Feeds : int { All = 0, Common = 1, Custom = 8 };

public:
    virtual ~IFeedsUpdater() = default;

    virtual bool update( const std::string& source, int snapshot ) = 0;
    virtual void update( const std::string& imageUrl ) = 0;

    virtual void resetCurrent() = 0;
    virtual void resetToCurrent() = 0;

    virtual std::string getFeedsSource() const = 0;
    virtual std::string getExpectedUpdateVersion() const = 0;
    virtual std::string getFeedsUpdateVersion() const = 0;
    virtual int getFeedsSnapshot() const = 0;
    virtual std::string generateImageUrl() const = 0;
    virtual std::string generateRecoveryImageUrl() const = 0;
    virtual Feeds getFeedsType() const {
        return Feeds::All;
    }
};
using IFeedUpdaterUPtr = std::unique_ptr< Opkg::IFeedsUpdater >;

}  // namespace Opkg
}  // namespace HubPlatform
