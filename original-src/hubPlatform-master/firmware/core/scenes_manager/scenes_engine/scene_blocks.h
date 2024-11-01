#pragma once

#include <string>
#include <vector>
#include <set>

#include "json_utils/json_wrapper.h"
#include "jsonrpc/jsonrpc.h"

namespace SceneEngine {
namespace UT {
class SceneBlocksTest;
}  // namespace UT

class SceneBlocks {
    friend class UT::SceneBlocksTest;

public:
    using StreamUPtr = std::unique_ptr< std::istream >;
    using CharType = std::istream::char_type;
    using PosType = std::istream::pos_type;
    using BlockSourcesContainer = std::set< std::string >;
    using CoredItem = JsonObject;
    using CoredItemsList = std::vector< CoredItem >;
    using Filters = std::set< std::string >;
    using EnumList = std::set< std::string >;
    using ItemId = std::string;
    using ItemIds = std::vector< ItemId >;
    using ItemName = std::string;
    using DeviceId = std::string;
    using ValueType = std::string;
    using SourceId = std::string;
    using SourceIds = std::vector< std::string >;
    using BlockId = std::string;
    using Block = JsonObject;
    using BlocksList = JsonArray;
    using ConstCoredItemPtrList = std::vector< const CoredItem* >;
    using SortedItemsByDeviceId = std::map< DeviceId, ConstCoredItemPtrList >;
    using SortedItems = std::map< ItemName, SortedItemsByDeviceId >;
    using DeviceCategory = std::string;
    using DeviceSubcategory = std::string;
    using DeviceSubcategories = std::set< DeviceSubcategory >;
    using SortedBlockKey = std::tuple< ItemName, DeviceCategory, DeviceSubcategory >;

    class Device {
    public:
        Device() = default;
        Device( const DeviceCategory& category, const DeviceSubcategory& subcategory )
        : category( category )
        , subcategory( subcategory ) {}
        bool operator<( const Device& other ) const {
            return std::tie( category, subcategory ) < std::tie( other.category, other.subcategory );
        }

        DeviceCategory category;
        DeviceSubcategory subcategory;
    };

    using Devices = std::set< Device >;

    struct OptionItem {
        ItemName name;
        std::string value;
        std::string fieldName;
    };
    using SortedOptionItemsByBlockId = std::map< BlockId, OptionItem >;

    struct BlockData {
        Devices forbiddenDevices;
        Block block;
        Devices parents;
        Devices forbiddenParents;
    };

    using SortedBlocks = std::map< SortedBlockKey, std::vector< BlockData > >;

    struct Item {
        ItemId id;
        ItemName name;
        DeviceId deviceId;
        EnumList enumList;
        Device device;
        ValueType type;
        Device parent;
    };

    struct ItemToSave {
        Item item;
        SourceId sourceId;
        BlockId blockId;
        OptionItem optionItem;
    };

    struct ItemToGet {
        Item item;
        SortedOptionItemsByBlockId sortedOptionItemsByBlockId;
    };

    using ItemsList = std::vector< Item >;
    using ItemsToGet = std::vector< ItemToGet >;
    using SaveItemCallback = std::function< void( const ItemToSave& item ) >;
    using OptionItemsToUpdate = std::map< std::tuple< DeviceId, ItemName >, ItemToSave >;

public:
    SceneBlocks( SaveItemCallback saveItem );
    bool setBlockSourceAndFilterItems( const std::string& sourceId, std::istream& stream, const CoredItemsList& items );
    bool setSceneBlockSource( const std::string& sourceId );
    bool removeSceneBlocksSource( const std::string& sourceId );
    BlocksList getBlockList( const ItemsToGet& items, const std::vector< StreamUPtr >& blockStreams );
    const BlockSourcesContainer& getBlockSourceIds();
    bool filterOutNewItem( const CoredItem& item,
                           const CoredItemsList& possibleOptionItems,
                           const std::string& sourceId,
                           std::istream& blockStream );
    void checkForOptionItemUpdate( const CoredItem& item );

private:
    class EmptyField : public std::exception {
    public:
        EmptyField( const std::vector< JsonObject >& keyPath )
        : m_what( "Empty field: " ) {
            for( const auto& key : keyPath ) {
                m_what += key.dump() + ".";
            }
        }
        virtual const char* what() const noexcept override {
            return m_what.c_str();
        }

    private:
        std::string m_what;
    };

private:
    using DeviceCallback = std::function< void( const std::string& category, const std::string& subcategory ) >;

    static bool validateBlocksSource( const JsonObject& blocks );
    static bool validateWhenBlocks( const JsonArray& blocks );
    static bool validateWhenBlock( const JsonArray& block );

    template < typename T >
    static void validateField( const JsonObject& json, const std::vector< JsonObject >& keyPath ) {
        T field;
        if( !json_get( json, keyPath, field ) || field.empty() ) {
            throw EmptyField( keyPath );
        }
    }

    bool saveFilteredItems( const CoredItemsList& coredItemsToSave,
                            const CoredItemsList& possibleOptionItems,
                            const JsonObject& blocks,
                            const std::string& sourceId );

    bool getBlocks( std::istream& stream, JsonObject& blocks );
    bool getWhenBlocks( JsonObject& blocks, JsonObject& whenBlocks );
    std::string getSortedBlocks( const std::vector< StreamUPtr >& blockStreams, SortedBlocks& sortedBlocks );
    std::string getSortedBlocks( const JsonObject& blocks, SortedBlocks& sortedBlocks );
    EnumList getEnumList( const CoredItem& item );
    OptionItem getOptionItem( const DeviceId& deviceId, const Block& whenBlock, const SortedItems& sortedItems );
    SortedItems getSortedItems( const CoredItemsList& items );

    bool forEachWhenBlocks( JsonObject& blocks, std::function< void( JsonObject& whenBlock ) > callback );
    bool forEachItemFilter(
        const JsonObject& blocks,
        std::function< void( const SortedBlockKey& filter, const BlockData& blockData ) > callback );

    void handleDeviceFilters( const Json& whenBlock,
                              const std::string& filterName,
                              DeviceCallback callback,
                              DeviceCallback forbiddenDevicesCallback );

    void forEachDevice( const JsonArray& categorySubcategoryArray, DeviceCallback callback );

    void setBlockFields( Block& block, const ItemToGet& item );

    bool needAlignOptions( const ValueType& valueType ) const;

    void alignOptions( JsonObject& field, const std::set< std::string >& availableOptions );

    std::string convertToItemField( const std::string& fieldName );

private:
    BlockSourcesContainer m_blockSources;
    SaveItemCallback m_saveItem;
    OptionItemsToUpdate m_optionItemsToUpdate;
};

}  // namespace SceneEngine
