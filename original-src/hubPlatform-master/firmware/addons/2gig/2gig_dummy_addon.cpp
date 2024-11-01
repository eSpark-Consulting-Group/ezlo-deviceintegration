#include "2gig/2gig_dummy_addon.h"

#include "2gig/version/2gig_addon_version.h"
#include "common_utils/lock_helper.h"
#include "common_utils/backup/backup_helper.h"

namespace HubPlatform {
namespace TwoGig {

const char* const DummyAddon::Name = "2gig";
const char* const DummyAddon::Version = "1.0";

DummyAddon::DummyAddon()
: Addons::Addon( Name ) {}

DummyAddon::~DummyAddon() = default;

CommonUtils::AddonVersion DummyAddon::getVersion() const {
    return Addons::TwoGig::getVersion();
}

void DummyAddon::setUp() {
    Base::setUp();
    join( GroupsNames::SYSTEM_GROUP );
    LOGGER_INFO_RAW( cc::info( "Dummy 2GIG addon has started" ) );
}

void DummyAddon::tearDown() {
    Base::tearDown();
}

JsonObject DummyAddon::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case GLOBAL_LOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.tryLock( request );
        return lockHelper.getResponse();
    }
    case GLOBAL_UNLOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.unlock( request );
        return lockHelper.getResponse();
    }
    case BACKUP_CREATE: {
        CommonUtils::BackupHelper helper( getDBClient(), request );
        return helper.getResponse();
    }
    default:
        return Service::handleRequestFromBus( request, requestType );
    };

    return JSON_EMPTY_OBJ;
}

}  // namespace TwoGig
}  // namespace HubPlatform
