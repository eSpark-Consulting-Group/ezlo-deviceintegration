#include "scenes_manager/scenes_engine/scene_blocks.h"
#include "log_utils/ezlo_log.h"
#include "database_client/db_tables_names.h"

namespace SceneEngine {

SceneBlocks::SceneBlocks( SaveItemCallback saveItem )
: m_saveItem( saveItem ) {}

bool SceneBlocks::setBlockSourceAndFilterItems( const std::string& sourceId,
                                                std::istream& stream,
                                                const CoredItemsList& items ) {
    JsonObject blocks;
    if( !getBlocks( stream, blocks ) ) {
        return false;
    }
    if( !validateBlocksSource( blocks ) ) {
        return false;
    }
    if( !setSceneBlockSource( sourceId ) ) {
        return false;
    }
    return saveFilteredItems( items, items, blocks, sourceId );
}

bool SceneBlocks::setSceneBlockSource( const std::string& sourceId ) {
    m_blockSources.emplace( sourceId );
    return true;
}

bool SceneBlocks::removeSceneBlocksSource( const std::string& sourceId ) {
    auto it = m_blockSources.find( sourceId );
    if( it == m_blockSources.end() ) {
        return false;
    }
    m_blockSources.erase( it );
    return true;
}

SceneBlocks::BlocksList SceneBlocks::getBlockList( const ItemsToGet& itemsToGet,
                                                   const std::vector< StreamUPtr >& blockStreams ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    if( blockStreams.empty() ) {
        return answer;
    }
    SortedBlocks sortedBlocks;
    std::string message = getSortedBlocks( blockStreams, sortedBlocks );
    if( !message.empty() ) {
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, message );
        return answer;
    }
    answer = JSON_EMPTY_ARR;
    auto updateAnswer = [&sortedBlocks, this]( JsonObject& answer,
                                               const ItemToGet& itemToGet,
                                               const DeviceCategory& category,
                                               const DeviceSubcategory& subcategory ) {
        auto blocks = sortedBlocks.find( SortedBlockKey( itemToGet.item.name, category, subcategory ) );
        if( blocks == sortedBlocks.end() ) {
            return false;
        }
        for( const BlockData& blockData : blocks->second ) {
            auto& forbiddenDevices = blockData.forbiddenDevices;
            auto& device = itemToGet.item.device;
            if( !forbiddenDevices.empty() &&
                forbiddenDevices.find( { device.category, device.subcategory } ) != forbiddenDevices.end() ) {
                continue;
            }
            auto& parent = itemToGet.item.parent;
            if( !parent.category.empty() && !parent.subcategory.empty() ) {
                auto& parents = blockData.parents;
                if( !parents.empty() && parents.find( { parent.category, parent.subcategory } ) == parents.end() ) {
                    continue;
                }
                auto& forbiddenParents = blockData.forbiddenParents;
                if( !forbiddenParents.empty() &&
                    forbiddenParents.find( { parent.category, parent.subcategory } ) != forbiddenParents.end() ) {
                    continue;
                }
            }
            Block block = blockData.block;
            setBlockFields( block, itemToGet );
            block.erase( "item_filter" );
            block.erase( "device_filter" );
            block.erase( "parent_device_filter" );
            answer.push_back( std::move( block ) );
        }
        return true;
    };
    for( const auto& itemToGet : itemsToGet ) {
        const Device& device = itemToGet.item.device;
        if( updateAnswer( answer, itemToGet, device.category, device.subcategory ) ) {
            continue;
        }
        if( updateAnswer( answer, itemToGet, device.category, "" ) ) {
            continue;
        }
        updateAnswer( answer, itemToGet, "", "" );
    }
    return answer;
}

const SceneBlocks::BlockSourcesContainer& SceneBlocks::getBlockSourceIds() {
    return m_blockSources;
}

bool SceneBlocks::filterOutNewItem( const CoredItem& item,
                                    const CoredItemsList& possibleOptionItems,
                                    const std::string& sourceId,
                                    std::istream& blockStream ) {
    JsonObject blocks;
    if( !getBlocks( blockStream, blocks ) ) {
        const std::string message = "bad block source";
        LOGGER_ERROR( message );
        return false;
    }
    return saveFilteredItems( { item }, possibleOptionItems, blocks, sourceId );
}

void SceneBlocks::checkForOptionItemUpdate( const CoredItem& item ) {
    auto deviceId = json_get_str( item, "deviceId" );
    auto name = json_get_str( item, "name" );
    LOGGER_TRACE( "deviceId: " + deviceId + " name: " + name );
    auto optionItemIt = m_optionItemsToUpdate.find( OptionItemsToUpdate::key_type( deviceId, name ) );
    if( optionItemIt != m_optionItemsToUpdate.end() ) {
        ItemToSave& itemToSave = optionItemIt->second;
        itemToSave.optionItem.value = json_get_str( item, convertToItemField( itemToSave.optionItem.fieldName ) );
        LOGGER_TRACE( "optionItem.name: " + itemToSave.optionItem.name );
        LOGGER_TRACE( "optionItem.value: " + itemToSave.optionItem.value );
        LOGGER_TRACE( "optionItem.fieldName: " + itemToSave.optionItem.fieldName );
        m_saveItem( itemToSave );
        m_optionItemsToUpdate.erase( optionItemIt );
    }
}

bool SceneBlocks::validateBlocksSource( const JsonObject& blocks ) {
    auto it = blocks.find( "when" );
    if( it == blocks.end() ) {
        LOGGER_ERROR( "when-block is not found" );
        return false;
    }
    if( !validateWhenBlocks( *it ) ) {
        LOGGER_ERROR( "invalid block" );
        return false;
    }
    return true;
}

bool SceneBlocks::validateWhenBlocks( const JsonArray& blocks ) {
    for( const auto& block : blocks ) {
        if( !validateWhenBlock( block ) ) {
            return false;
        }
    }
    return true;
}

bool SceneBlocks::validateWhenBlock( const JsonArray& block ) {
    try {
        validateField< JsonArray >( block, { "item_filter", "names" } );
        validateField< std::string >( block, { "blockOptions", "method", "name" } );
        validateField< std::string >( block, { "blockOptions", "method", "args", "item" } );
        auto fields = block.at( "fields" );
        bool isItemIdFound = false;
        for( const auto& field : fields ) {
            auto fieldName = field.at( "name" );
            if( fieldName.get< std::string >() == "item" ) {
                isItemIdFound = true;
            }
        }
        if( !isItemIdFound ) {
            LOGGER_ERROR( "Item id is not found!" );
            return false;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        return false;
    }
    return true;
}

bool SceneBlocks::saveFilteredItems( const CoredItemsList& coredItemsToSave,
                                     const CoredItemsList& possibleOptionItems,
                                     const JsonObject& blocks,
                                     const std::string& sourceId ) {
    SortedItems sortedItems = getSortedItems( possibleOptionItems );
    SortedBlocks sortedBlocks;
    if( !getSortedBlocks( blocks, sortedBlocks ).empty() ) {
        return false;
    }
    auto saveFilteredItems =
        [&sortedBlocks, &sortedItems, this](
            const SortedBlockKey& sortedBlockKey, const Device& parent, const Device& device, ItemToSave& itemToSave ) {
            auto blockIt = sortedBlocks.find( sortedBlockKey );
            if( blockIt == sortedBlocks.end() ) {
                return;
            }
            auto& blockDataList = blockIt->second;
            for( const BlockData& blockData : blockDataList ) {
                auto& forbiddenDevices = blockData.forbiddenDevices;
                if( !forbiddenDevices.empty() &&
                    forbiddenDevices.find( { device.category, device.subcategory } ) != forbiddenDevices.end() ) {
                    continue;
                }
                if( !parent.category.empty() && !parent.subcategory.empty() ) {
                    auto& parents = blockData.parents;
                    if( !parents.empty() && parents.find( { parent.category, parent.subcategory } ) == parents.end() ) {
                        continue;
                    }
                    auto& forbiddenParents = blockData.forbiddenParents;
                    if( !forbiddenParents.empty() &&
                        forbiddenParents.find( { parent.category, parent.subcategory } ) != forbiddenParents.end() ) {
                        continue;
                    }
                }
                auto& whenBlock = blockData.block;
                itemToSave.blockId = json_get_str( whenBlock, "blockId" );
                itemToSave.optionItem = getOptionItem( itemToSave.item.deviceId, whenBlock, sortedItems );
                if( !itemToSave.optionItem.name.empty() && itemToSave.optionItem.value.empty() ) {
                    m_optionItemsToUpdate.insert(
                        { OptionItemsToUpdate::key_type( itemToSave.item.deviceId, itemToSave.optionItem.name ),
                          itemToSave } );
                }
                m_saveItem( itemToSave );
            }
        };
    for( const auto& coredItem : coredItemsToSave ) {
        ItemToSave itemToSave;
        Item& item = itemToSave.item;
        item.name = json_get_str( coredItem, "name" );
        item.id = json_get_str( coredItem, "_id" );
        if( item.name.empty() || item.id.empty() ) {
            continue;
        }
        itemToSave.sourceId = sourceId;
        Device& device = item.device;
        device.category = json_get_str( coredItem, "deviceCategory" );
        device.subcategory = json_get_str( coredItem, "deviceSubcategory" );
        item.deviceId = json_get_str( coredItem, "deviceId" );
        item.enumList = getEnumList( coredItem );
        item.type = json_get_str( coredItem, "type" );
        Device parent( json_get_str( coredItem, "parentCategory" ), json_get_str( coredItem, "parentSubcategory" ) );
        item.parent = parent;

        saveFilteredItems( SortedBlockKey{ item.name, "", "" }, parent, device, itemToSave );
        if( !device.category.empty() ) {
            saveFilteredItems( SortedBlockKey{ item.name, device.category, "" }, parent, device, itemToSave );
            if( !device.subcategory.empty() ) {
                saveFilteredItems(
                    SortedBlockKey{ item.name, device.category, device.subcategory }, parent, device, itemToSave );
            }
        }
    }
    return true;
}

bool SceneBlocks::getBlocks( std::istream& stream, JsonObject& blocks ) {
    try {
        stream.clear();
        stream.seekg( 0, stream.beg );
        stream >> blocks;
    }
    catch( std::exception& ex ) {
        return false;
    }
    return true;
}

bool SceneBlocks::forEachWhenBlocks( JsonObject& blocks, std::function< void( JsonObject& whenBlock ) > callback ) {
    auto it = blocks.find( "when" );
    if( it == blocks.end() ) {
        return false;
    }
    for( auto& whenBlock : *it ) {
        callback( whenBlock );
    }
    return true;
}

bool SceneBlocks::forEachItemFilter(
    const JsonObject& blocks,
    std::function< void( const SortedBlockKey& filter, const BlockData& blockData ) > callback ) {
    auto it = blocks.find( "when" );
    if( it == blocks.end() ) {
        LOGGER_ERROR( "when-block is not found" );
        return false;
    }
    for( auto& whenBlock : *it ) {
        auto filters = whenBlock.find( "item_filter" );
        if( filters == whenBlock.end() ) {
            LOGGER_ERROR( "item_filter is not found" );
            continue;
        }
        auto names = filters->find( "names" );
        if( names == filters->end() ) {
            LOGGER_ERROR( "names are not found" );
            continue;
        }
        BlockData blockData;
        Devices devices;

        handleDeviceFilters( whenBlock,
                             "device_filter",
                             [&devices]( const std::string& category, const std::string& subcategory ) {
                                 devices.insert( { category, subcategory } );
                             },
                             [&blockData]( const std::string& category, const std::string& subcategory ) {
                                 blockData.forbiddenDevices.insert( { category, subcategory } );
                             } );

        handleDeviceFilters( whenBlock,
                             "parent_device_filter",
                             [&blockData]( const std::string& category, const std::string& subcategory ) {
                                 blockData.forbiddenParents.insert( { category, subcategory } );
                             },
                             [&blockData]( const std::string& category, const std::string& subcategory ) {
                                 blockData.forbiddenParents.insert( { category, subcategory } );
                             } );

        blockData.block = whenBlock;
        if( devices.empty() ) {
            for( const auto& itemName : *names ) {
                callback( SortedBlockKey( json_get_str( itemName ), "", "" ), blockData );
            }
        }
        else {
            for( const auto& itemName : *names ) {
                for( const Device& device : devices ) {
                    callback( SortedBlockKey( json_get_str( itemName ), device.category, device.subcategory ),
                              blockData );
                }
            }
        }
    }
    return true;
}

void SceneBlocks::handleDeviceFilters( const Json& whenBlock,
                                       const std::string& filterName,
                                       DeviceCallback callback,
                                       DeviceCallback forbiddenDevicesCallback ) {
    auto deviceFiltersIt = whenBlock.find( filterName );
    if( deviceFiltersIt != whenBlock.end() ) {
        auto deviceCategoryIt = deviceFiltersIt->find( "category" );
        if( deviceCategoryIt != deviceFiltersIt->end() ) {
            forEachDevice( *deviceCategoryIt, callback );
        }
        else {
            auto deviceNotCategoryIt = deviceFiltersIt->find( "not" );
            if( deviceNotCategoryIt != deviceFiltersIt->end() ) {
                auto deviceCategoryIt = deviceNotCategoryIt->find( "category" );
                if( deviceCategoryIt != deviceNotCategoryIt->end() ) {
                    forEachDevice( *deviceCategoryIt, forbiddenDevicesCallback );
                }
            }
        }
    }
}

void SceneBlocks::forEachDevice( const JsonArray& categorySubcategoryArray,
                                 std::function< void( const std::string&, const std::string& ) > callback ) {
    if( categorySubcategoryArray.empty() ) {
        callback( "", "" );
    }
    for( const auto& categorySubcategory : categorySubcategoryArray ) {
        for( auto categorySubcategoriesIt = categorySubcategory.begin();
             categorySubcategoriesIt != categorySubcategory.end();
             ++categorySubcategoriesIt ) {
            if( categorySubcategoriesIt->empty() ) {
                callback( categorySubcategoriesIt.key(), "" );
            }
            else {
                for( auto subcategoriesIt = categorySubcategoriesIt->begin();
                     subcategoriesIt != categorySubcategoriesIt->end();
                     ++subcategoriesIt ) {
                    callback( categorySubcategoriesIt.key(), json_get_str( *subcategoriesIt ) );
                }
            }
        }
    }
}

void SceneBlocks::setBlockFields( Block& block, const ItemToGet& itemToGet ) {
    auto fields = block.find( "fields" );
    if( fields == block.end() ) {
        return;
    }
    for( auto& field : *fields ) {
        auto optionItemValueIt = field.find( "optionItemValue" );
        if( optionItemValueIt != field.end() ) {
            JsonObject& optionItemValue = *optionItemValueIt;
            auto itemFinder = optionItemValue.find( "itemFinder" );
            if( itemFinder != optionItemValue.end() ) {
                optionItemValue.erase( "itemFinder" );
                auto blockId = json_get_str( block, "blockId" );
                auto optionItemIt = itemToGet.sortedOptionItemsByBlockId.find( blockId );
                if( optionItemIt != itemToGet.sortedOptionItemsByBlockId.end() ) {
                    const OptionItem& optionItem = optionItemIt->second;
                    optionItemValue[ optionItem.fieldName ] = optionItem.value;
                }
                else {
                    LOGGER_ERROR( "Wrong block id: " + blockId );
                }
            }
        }
        auto fieldName = field.find( "name" );
        if( fieldName->get< std::string >() == "item" ) {
            field[ "value" ] = itemToGet.item.id;
        }
        else if( fieldName->get< std::string >() == "value" && needAlignOptions( itemToGet.item.type ) ) {
            alignOptions( field, itemToGet.item.enumList );
        }
    }
}

bool SceneBlocks::needAlignOptions( const ValueType& valueType ) const {
    return valueType == "token" || valueType == "button_state";
}

void SceneBlocks::alignOptions( JsonObject& field, const std::set< std::string >& availableOptions ) {
    auto optionsIt = field.find( "options" );
    if( optionsIt == field.end() ) {
        return;
    }
    if( availableOptions.empty() ) {
        optionsIt->clear();
        return;
    }
    JsonArray options = JSON_EMPTY_ARR;
    for( auto& option : *optionsIt ) {
        std::string value = json_get_str( option, "value" );
        if( availableOptions.find( value ) != availableOptions.end() ) {
            options.push_back( std::move( option ) );
        }
    }
    *optionsIt = std::move( options );
}

std::string SceneBlocks::convertToItemField( const std::string& fieldName ) {
    static std::map< std::string, std::string > convertToItemField = { { "itemId", "_id" } };
    auto itemFieldIt = convertToItemField.find( fieldName );
    if( itemFieldIt != convertToItemField.end() ) {
        return itemFieldIt->second;
    }
    return fieldName;
}

std::string SceneBlocks::getSortedBlocks( const std::vector< StreamUPtr >& blockStreams,
                                          SceneBlocks::SortedBlocks& sortedBlocks ) {
    for( const auto& blockStream : blockStreams ) {
        JsonObject blocks;
        if( !getBlocks( *blockStream, blocks ) ) {
            const std::string message = "bad block source";
            LOGGER_ERROR( message );
            return message;
        }
        const std::string message = getSortedBlocks( blocks, sortedBlocks );
        if( !message.empty() ) {
            return message;
        }
    }
    return std::string();
}

std::string SceneBlocks::getSortedBlocks( const JsonObject& blocks, SceneBlocks::SortedBlocks& sortedBlocks ) {
    bool isWhenBlocksFound =
        forEachItemFilter( blocks, [&sortedBlocks]( const SortedBlockKey& filter, const BlockData& blockData ) {
            sortedBlocks[ filter ].push_back( blockData );
        } );
    if( !isWhenBlocksFound ) {
        const std::string message = "when-block is not found";
        LOGGER_ERROR( message );
        return message;
    }
    return std::string();
}

SceneBlocks::EnumList SceneBlocks::getEnumList( const SceneBlocks::CoredItem& item ) {
    EnumList enumList;
    auto enumListIt = item.find( "enum" );
    if( enumListIt != item.end() ) {
        for( const auto& token : *enumListIt ) {
            enumList.emplace( json_get_str( token ) );
        }
    }
    return enumList;
}

SceneBlocks::OptionItem SceneBlocks::getOptionItem( const DeviceId& deviceId,
                                                    const Block& whenBlock,
                                                    const SceneBlocks::SortedItems& sortedItems ) {
    OptionItem optionItem;
    auto fieldsIt = whenBlock.find( "fields" );
    if( fieldsIt == whenBlock.end() ) {
        LOGGER_ERROR( "Empty fields" );
        return optionItem;
    }
    for( const auto& field : *fieldsIt ) {
        JsonObject itemFinder = JSON_EMPTY_OBJ;
        if( !json_get( field, { "optionItemValue", "itemFinder" }, itemFinder ) ) {
            continue;
        }
        optionItem.name = json_get_str( itemFinder, "item" );
        optionItem.fieldName = json_get_str( itemFinder, "resultField" );
        auto sortedItemsIt = sortedItems.find( optionItem.name );
        if( sortedItemsIt == sortedItems.end() ) {
            continue;
        }
        const SortedItemsByDeviceId& sortedItemsByDeviceId = sortedItemsIt->second;
        auto itemIt = sortedItemsByDeviceId.find( deviceId );
        if( itemIt == sortedItemsByDeviceId.end() ) {
            continue;
        }
        const ConstCoredItemPtrList& items = itemIt->second;
        if( items.empty() ) {
            continue;
        }
        optionItem.value = json_get_str( *items.front(), convertToItemField( optionItem.fieldName ) );
    }
    return optionItem;
}

SceneBlocks::SortedItems SceneBlocks::getSortedItems( const SceneBlocks::CoredItemsList& items ) {
    SortedItems sortedItems;
    for( const CoredItem& item : items ) {
        std::string name = json_get_str( item, "name" );
        std::string deviceId = json_get_str( item, "deviceId" );
        if( name.empty() || deviceId.empty() ) {
            LOGGER_ERROR( "Empty name: " + name + " or deviceId: " + deviceId );
            continue;
        }
        sortedItems[ name ][ deviceId ].push_back( &item );
    }
    return sortedItems;
}

}  // namespace SceneEngine
