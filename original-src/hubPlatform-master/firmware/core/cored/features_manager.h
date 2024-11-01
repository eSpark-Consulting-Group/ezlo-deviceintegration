#pragma once

#include "database_client/idbclient.h"
#include "base_utils/semantic_version.h"

namespace HubPlatform {
namespace Services {
class Service;
}
namespace CoreD {

class ControllerFeatureCheckError : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

struct ControllerFeature {
public:
    using SemanticVersion = Common::SemanticVersion< int >;

public:
    ControllerFeature() = default;
    ControllerFeature( std::string name, SemanticVersion version, bool status, bool editable, bool isCustom );

    bool fromJson( const std::string& baseName, const JsonObject& json );
    void fromRecordJson( const std::string& baseName, const JsonObject& json );
    static void checkFeature( const JsonObject& json );

    JsonObject toDbRecord() const;
    JsonObject toDbRecordBody() const;

    std::string getVersion();
    bool getStatus();
    bool getEditable();

public:
    std::string name;
    SemanticVersion version;
    bool status{ false };
    bool editable{ false };
    bool isCustom{ false };
    std::string featureVersionStr;
};

class IFeaturesManager {
public:
    virtual ~IFeaturesManager();

    virtual void setFeatureStatus( const std::string& feature, bool status ) = 0;
    virtual bool getFeatureStatus( const std::string& feature ) const = 0;
    virtual void setUpExternalFeatures() = 0;

    using Callback = std::function< void( std::string name, bool status ) >;
    virtual void onFeatureStatusUpdated( Callback callback ) = 0;
};

class FeaturesManager : public IFeaturesManager {
public:
    explicit FeaturesManager( IDbClient& dbClient );

    void setFeatureStatus( const std::string& feature, bool status ) override;
    bool getFeatureStatus( const std::string& feature ) const override;
    void onFeatureStatusUpdated( Callback callback ) override;

protected:
    void fixWebRTCFeature( const ControllerFeature& feature );
    void emitStatusUpdated( const std::string& feature, bool status );

protected:
    IDbClient& m_dbClient;
    Callback m_onStatusUpdated;
};

class ExternalFeaturesManager final : public FeaturesManager {
public:
    explicit ExternalFeaturesManager( Services::Service& host );
    void setUpExternalFeatures() override;
    static std::vector< std::string > getCustomPackagesFiles();
    void processPackageAdding( const JsonObject& fileData );
    static bool processPackageRemoving( const JsonObject& fileData, std::string& name );
    void addCustomFeature( const ControllerFeature& feature );
    void updateCustomFeature( const ControllerFeature& feature );
    void removeCustomFeature( const std::string& packageName );

private:
    void emitEventUpdated( const JsonObject& payload ) const;
    void emitEventAdded( const JsonObject& payload ) const;
    void emitEventRemoved( const JsonObject& payload ) const;

private:
    Services::Service& m_service;
};

}  // namespace CoreD
}  // namespace HubPlatform
