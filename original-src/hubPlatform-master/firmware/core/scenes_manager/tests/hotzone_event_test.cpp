#include "scenes_manager/scenes_engine/events/hotzone_event.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/camera_hotzone_value.h"
#include "cored/model/values/value_factory.h"
#include "json_utils/json_wrapper.h"

namespace SceneEngine {
namespace UT {

class HotzoneEventTest : public testing::Test {
public:
    class HotzonesItemValueBuilder {
    public:
        void set( const std::string& hotzoneId ) {
            using namespace HubPlatform::CoreD::Model;
            auto value = ValueFactory::createDefaultItemValue( ValueType::Enum::CameraHotzone );
            JsonObject hotzone = value->toJson();
            m_value[ hotzoneId ] = std::move( hotzone );
        }

        void set( const std::string& hotzoneId, const std::string& objectType, const std::vector< std::string > ids ) {
            namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
            if( !m_value.count( hotzoneId ) )
                set( hotzoneId );
            m_value.at( hotzoneId ).at( HotzoneFields::Detected )[ objectType ] = ids;
        }

        const JsonObject& value() {
            return m_value;
        }

    private:
        JsonObject m_value = JSON_EMPTY_OBJ;
    };

    class HotzoneEventFiltersBuilder {
    public:
        void set( const std::string& objectType, std::string match, std::vector< std::string > list ) {
            namespace Fields = HubPlatform::CoreD::Model::Resources::HotzoneMatchValueFields;
            using namespace HubPlatform::CoreD::Model;

            JsonObject matchJson;
            matchJson[ Fields::Match ] = std::move( match );
            if( !list.empty() )
                matchJson[ Fields::List ] = std::move( list );

            auto value = HotzoneMatchValue::createValue( matchJson );
            auto it = m_value.find( objectType );
            if( it != m_value.cend() )
                it->second = std::move( *value );
            else
                m_value.emplace( objectType, std::move( *value ) );
        }

        const HotzoneEvent::Filters& value() {
            return m_value;
        }

    private:
        HotzoneEvent::Filters m_value;
    };

    const std::string keyHotzone1 = "hotzone1";
    const std::string keyHotzone2 = "hotzone2";

    const std::string keyObjectsType = "objects";
    const std::string keyPersonsType = "persons";

    const std::string keyObjectId1 = "object1";
    const std::string keyObjectId2 = "object2";

    const std::string keyPersonId1 = "person1";
    const std::string keyPersonId2 = "person2";
};

TEST_F( HotzoneEventTest, TestFixtureHotzonesBuilder ) {
    HotzonesItemValueBuilder builder;

    {  // set empty hotzone
        builder.set( keyHotzone1 );
        auto expectedJson = R"({"hotzone1":{"detected":{},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // add first hotzone
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        auto expectedJson = R"({"hotzone1":{"detected":{"objects":["object1"]},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // add second hotzone
        builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
        auto expectedJson =
            R"({"hotzone1":{"detected":{"objects":["object1"]},"name":""},"hotzone2":{"detected":{"persons":["person1"]},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // add second object to first hotzone
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        auto expectedJson =
            R"({"hotzone1":{"detected":{"objects":["object1","object2"]},"name":""},"hotzone2":{"detected":{"persons":["person1"]},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // clear first hotzone
        builder.set( keyHotzone1 );
        auto expectedJson =
            R"({"hotzone1":{"detected":{},"name":""},"hotzone2":{"detected":{"persons":["person1"]},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // clear second hotzone
        builder.set( keyHotzone2 );
        auto expectedJson = R"({"hotzone1":{"detected":{},"name":""},"hotzone2":{"detected":{},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
}

TEST_F( HotzoneEventTest, TestFixtureEventFilter ) {
    {  // add filter without list
        HotzoneEventFiltersBuilder builder;
        builder.set( keyObjectsType, "any", {} );
        auto& filter = builder.value().at( keyObjectsType );
        EXPECT_EQ( filter.getMatchPolicy(), HotzoneEvent::Filter::MatchPolicy::Any );
        EXPECT_EQ( filter.getMatchList(), HotzoneEvent::Filter::MatchList() );
    }
    {  // add filter with list
        HotzoneEventFiltersBuilder builder;
        builder.set( keyObjectsType, "anyOf", { keyObjectId1 } );
        auto& filter = builder.value().at( keyObjectsType );
        EXPECT_EQ( filter.getMatchPolicy(), HotzoneEvent::Filter::MatchPolicy::AnyOf );
        EXPECT_EQ( filter.getMatchList(), HotzoneEvent::Filter::MatchList( { keyObjectId1 } ) );
    }
}

TEST_F( HotzoneEventTest, AnyHotzoneMatchAny ) {
    HotzoneEvent::Hotzones hotzones;
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Any;

    {  // motion detected
        HotzoneEvent event( hotzones, matchPolicy, {} );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }
    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, {} );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, AnyHotzoneMatchNone ) {
    HotzoneEvent::Hotzones hotzones;
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::None;

    {  // motion detected
        HotzoneEvent event( hotzones, matchPolicy, {} );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }
    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, {} );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, AnyHotzoneMatchFilter ) {
    HotzoneEvent::Hotzones hotzones;
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Filter;

    HotzoneEventFiltersBuilder filter;
    filter.set( keyObjectsType, "anyOf", { keyObjectId1 } );

    {  // motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }
    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, SelectedHotzoneMatchFilter ) {
    HotzoneEvent::Hotzones hotzones{ keyHotzone1 };
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Filter;

    HotzoneEventFiltersBuilder filter;
    filter.set( keyObjectsType, "anyOf", { keyObjectId1 } );

    {  // no hotzones was created
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // no needed hotzone
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( "dummyHotzone" );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected in another hotzone
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone2, keyObjectsType, { keyObjectId1 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected in targeted hotzone but not for targeted object
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId2 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected in targeted hotzone for targeted object
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, MultipleHotzonesMatchFilter ) {
    HotzoneEvent::Hotzones hotzones{ keyHotzone1, keyHotzone2 };
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Filter;

    HotzoneEventFiltersBuilder filter;
    filter.set( keyObjectsType, "anyOf", { keyObjectId1 } );

    {  // no hotzones was created
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // no needed hotzone
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( "dummyHotzone" );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected in targeted hotzones but not for targeted object
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId2 } );
        builder.set( keyHotzone2, keyObjectsType, { keyObjectId2 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected in first targeted hotzone for targeted object
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // motion detected in first targeted hotzone for targeted object + other objects
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // motion detected in second targeted hotzone for targeted object
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone2, keyObjectsType, { keyObjectId1 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // motion detected in second targeted hotzone for targeted object + other objects
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone2, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        builder.set( keyHotzone2, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    filter.set( keyObjectsType, "anyOf", { keyObjectId1, keyObjectId2 } );

    {  // motion detected for both hotzones for targeted objects
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone2, keyObjectsType, { keyObjectId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, FilterAnyObjects ) {
    HotzoneEvent::Hotzones hotzones;
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Filter;

    HotzoneEventFiltersBuilder filter;
    filter.set( keyObjectsType, "any", {} );

    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // wrong type detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, FilterAnyOfObjects ) {
    HotzoneEvent::Hotzones hotzones;
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Filter;

    HotzoneEventFiltersBuilder filter;
    filter.set( keyObjectsType, "anyOf", { keyObjectId1, keyObjectId2 } );

    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // wrong type detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected for first id
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // motion detected for second id
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // motion detected for both ids
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, FilterAllOfObjects ) {
    HotzoneEvent::Hotzones hotzones;
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Filter;

    HotzoneEventFiltersBuilder filter;
    filter.set( keyObjectsType, "allOf", { keyObjectId1, keyObjectId2 } );

    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // wrong type detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected for first id
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected for second id
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected for both ids
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, FilterOneOfObjects ) {
    HotzoneEvent::Hotzones hotzones;
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Filter;

    HotzoneEventFiltersBuilder filter;
    filter.set( keyObjectsType, "oneOf", { keyObjectId1, keyObjectId2 } );

    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // wrong type detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected for first id
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // motion detected for second id
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // motion detected for both ids
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, FilterNoneOfObjects ) {
    HotzoneEvent::Hotzones hotzones;
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Filter;

    HotzoneEventFiltersBuilder filter;
    filter.set( keyObjectsType, "noneOf", { keyObjectId1, keyObjectId2 } );

    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // wrong type detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1 } );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // motion detected for first id
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected for second id
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }

    {  // motion detected for both ids
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }
}

TEST_F( HotzoneEventTest, FilterNoneObjects ) {
    HotzoneEvent::Hotzones hotzones;
    HotzoneEvent::MatchPolicy matchPolicy = HotzoneEvent::MatchPolicy::Filter;

    HotzoneEventFiltersBuilder filter;
    filter.set( keyObjectsType, "none", {} );

    {  // no motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1 );
        EXPECT_TRUE( event.compare( builder.value() ) );
    }

    {  // motion detected
        HotzoneEvent event( hotzones, matchPolicy, filter.value() );
        HotzonesItemValueBuilder builder;
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        builder.set( keyHotzone1, keyPersonsType, { keyPersonId1, keyPersonId2 } );
        EXPECT_FALSE( event.compare( builder.value() ) );
    }
}

}  // namespace UT
}  // namespace SceneEngine
