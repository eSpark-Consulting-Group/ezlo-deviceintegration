#ifndef FO_H
#define FO_H

#include "bus/messagesclasses.h"
#include "base_utils/fo_utils.h"
#include "json_utils/json_wrapper.h"

class Instance {
public:
    bool getAdd();
    void updateItem( JsonObject& item );
    void set( bool show,
              bool showAsRule,
              bool hasSetter,
              bool hasGetter,
              const std::string& label,
              const std::string& valueType,
              const JsonObject& valuesEnum );
    std::string getPageId( const std::string& id );

private:
    std::string label_;
    std::string type_;
    bool hasGetter_ = false;
    bool hasSetter_ = false;
    bool show_ = false;
    bool add_ = false;
    JsonObject enum_;
};

// Viktro Y: could it me moved to the namespace with static functions inside the cpp file ?????
class Fo {
public:
    //!@param[in] query Filter for searching (usually empty, which means to query for *all* data)
    //!@param[in] restricted_user User identifier(email) who does not have EDIT rights, otherwise parameter should be
    //! empty
    static std::vector< JsonObject > getItems( JsonObject query = JSON_EMPTY_OBJ,
                                               const std::string& restrictedUser = std::string() );

    //!@param[in] query Filter for searching (usually empty, which means to query for *all* data)
    //!@param[in] restricted_user User identifier(email) who does not have EDIT rights, otherwise parameter should be
    //! empty
    //!@param[in] includeSpec Projections with include specifications; can be empty to return results with default
    //! projections (exclude specs)
    static std::vector< JsonObject > getDevices( JsonObject query = JSON_EMPTY_OBJ,
                                                 const std::string& restricted_user = std::string(),
                                                 const std::string& includeSpec = std::string() );

    //!@param[in] restricted_user User identifier(email) who does not have EDIT rights, otherwise parameter should be
    //! empty
    static std::vector< JsonObject > getItemsByDeviceId( const std::string& deviceId,
                                                         const std::string& restrictedUser = std::string() );

    static void setPageId( const std::string& deviceId, const std::string& pageId );
    static void setParentId( const std::string& dataitemId, const std::string& pageId );
    static void renameDevice( const std::string& deviceId, const std::string& name );
    static bool hasDeviceWithName( const std::string& name, const std::string& id );
    static bool hasItem( const std::string& id );
    static void updateItemFields( const std::string& itemId, const JsonObject& fields );
    static std::vector< std::string > getOnUnhiddenItems();

private:
    static JsonObject getUpdatedFields( const JsonObject& item, const JsonObject& fields );
    static void foUpdateFields( const std::string& dbName,
                                const JsonObject& query,
                                const JsonObject& fields,
                                const MessageSubClasses msgSubclass );
    static std::vector< JsonObject > getAvailableDeviceIds( const JsonObject& query,
                                                            const std::string& restrictedUser );
    // converter
    static JsonObject getButtonMethod( const std::string& label,
                                       const std::string& description,
                                       const std::string& method );
};

#endif  // FO_H
