#pragma once

#include <znet/znet_provisioning_list_entry.hpp>

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Zwave {
namespace ProvisioningListUtils {

using EntryAndError = std::pair< znet::ProvisioningListEntry, Json >;

Json idFromJson( const Json& json, uint32_t& id );
void idToJson( uint32_t id, Json& json );

Json dskFromJson( const Json& json, znet::DSK& dsk, bool dbFormat );
void dskToJson( const znet::DSK& dsk, Json& json, bool dbFormat );

Json entryFieldsFromJson( const Json& json, znet::ProvisioningListEntryFields& fields, bool strict );
void entryFieldsToJson( const znet::ProvisioningListEntryFields& fields, Json& json );
void entryFieldsToJsonUiFiltered( const znet::ProvisioningListEntryFields& fields, Json& json );

void genEntryFieldsFromJson( const Json& json, znet::ProvisioningListEntryFields& fields );
void genEntryFieldsToJson( const znet::ProvisioningListEntryFields& fields, Json& json );

bool entryFromDbJson( const Json& json, znet::ProvisioningListEntry& entry );
void entryToJson( const znet::ProvisioningListEntry& entry, Json& json, bool dbFormat );
void entryToJsonUiFiltered( const znet::ProvisioningListEntry& entry, Json& json, bool dbFormat );

znet::utils::optional< uint32_t > idFromString( const std::string& provisioningId );
std::string idToString( uint32_t provisioningId );

void dskFromString( const std::string& str, znet::DSK& dsk );
std::string dskToString( znet::DSK dsk );

znet::utils::optional< znet::ProvisioningListEntryFields::Status > statusFromString( const std::string& status );
const char* statusToString( znet::ProvisioningListEntryFields::Status status );

znet::utils::optional< znet::ProvisioningListEntryFields::AddingMode > addingModeFromString(
    const std::string& addingMode );

znet::utils::optional< znet::ProvisioningListEntryFields::InclusionSetting > inclusionSettingFromString(
    const std::string& inclusionSetting );

const char* addingModeToString( znet::ProvisioningListEntryFields::AddingMode addingMode );
const char* inclusionSettingToString( znet::ProvisioningListEntryFields::InclusionSetting setting );

uint8_t securityModeFromString( const std::string& securityMode );
Json securityModeToJson( uint8_t modes );

}  // namespace ProvisioningListUtils
}  // namespace Zwave
}  // namespace HubPlatform
