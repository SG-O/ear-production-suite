#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <adm/adm.hpp>
#include "objectautomationelement.h"
#include "mocks/projectelements.h"
#include "mocks/pluginsuite.h"
#include "mocks/reaperapi.h"
#include "blockbuilders.h"
#include "parameter.h"
#include "mocks/parameter.h"
#include "mocks/parametised.h"
#include "mocks/automationenvelope.h"

using namespace admplug;
using namespace admplug::testing;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Matcher;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::ByMove;

namespace {
std::shared_ptr<adm::AudioChannelFormat> objectChannel() {
    return adm::AudioChannelFormat::create(adm::AudioChannelFormatName{"test"}, adm::TypeDefinition::OBJECTS);
}
std::shared_ptr<adm::AudioPackFormat> objectPack() {
    return adm::AudioPackFormat::create(adm::AudioPackFormatName{"test"}, adm::TypeDefinition::OBJECTS);
}

}

TEST_CASE("ObjectAutomationElement") {
    auto fakeParent = std::make_shared<NiceMock<MockTakeElement>>();
    auto audioTrackUid = adm::AudioTrackUid::create();
    auto channelFormatMutable = adm::AudioChannelFormat::create(adm::AudioChannelFormatName{ "Test channelFormat" }, adm::TypeDefinition::OBJECTS);
    std::shared_ptr<const adm::AudioChannelFormat> channelFormat{ channelFormatMutable };
    auto packFormatMutable = adm::AudioPackFormat::create(adm::AudioPackFormatName{ "Test packFormat" }, adm::TypeDefinition::OBJECTS);
    std::shared_ptr<const adm::AudioPackFormat> packFormat{ packFormatMutable };

    SECTION("parentTake returns element provided on construction") {
        auto automationElement = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormat, packFormat, audioTrackUid}, fakeParent);
        auto aeParentTake = automationElement->parentTake();
        REQUIRE(aeParentTake == fakeParent);
    }

    SECTION("onCreateProjectElements()") {
        NiceMock<MockPluginSuite> pluginSet;
        NiceMock<MockReaperAPI> api;

        SECTION("Pluginset onObjectAutomation called with node ref") {
            auto automationElement = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormat, packFormat, audioTrackUid}, fakeParent);
            EXPECT_CALL(pluginSet, onObjectAutomation(Ref(*automationElement), Ref(api)));
            automationElement->createProjectElements(pluginSet, api);
        }

        SECTION("blocks() returns blocks referenced by object") {
            auto block = adm::AudioBlockFormatObjects{ adm::SphericalPosition{adm::Azimuth{1.0}} };
            channelFormatMutable->add(block);
            auto automationElement = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormatMutable, packFormatMutable, audioTrackUid}, fakeParent);
            REQUIRE(automationElement->blocks().size() == 1);
            REQUIRE(automationElement->blocks()[0].get<adm::SphericalPosition>().get<adm::Azimuth>() == block.get<adm::SphericalPosition>().get<adm::Azimuth>());
        }
    }

}
/*
TEST_CASE("Applying automation") {
    auto channelFormat = objectChannel();
    auto packFormat = objectPack();
    auto api = NiceMock<MockReaperAPI>{};
    using ns = std::chrono::nanoseconds;
    auto parentTake = std::make_shared<NiceMock<MockTakeElement>>();
    auto element = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormat, packFormat, adm::AudioTrackUid::create()}, parentTake);

    MockPlugin plugin;
    auto envelope = std::make_unique<MockAutomationEnvelope>();
    auto& envRef = *envelope;

    MockPluginParameter parameter;
    ON_CALL(parameter, admParameter()).WillByDefault(Return(AdmParameter::OBJECT_AZIMUTH));
    EXPECT_CALL(parameter, admParameter()).Times(AnyNumber());
    AutomationPoint point{ns::zero(), ns::zero(), 1.0};
    ON_CALL(parameter, forwardMap(Matcher<AutomationPoint>(_))).WillByDefault(Return(point));
    EXPECT_CALL(parameter, forwardMap(Matcher<AutomationPoint>(_))).Times(AnyNumber());
    ON_CALL(parameter, forwardMap(Matcher<double>(_))).WillByDefault(Return(point.value()));
    EXPECT_CALL(parameter, forwardMap(Matcher<double>(_))).Times(AnyNumber());
    ON_CALL(parameter, getEnvelope(_)).WillByDefault(Return(ByMove(std::move(envelope))));

    SECTION("Apply sets parameter directly rather than adding envelope when only one point") {
        auto block = initialSphericalBlock();
        channelFormat->add(static_cast<adm::AudioBlockFormatObjects>(block));
        EXPECT_CALL(parameter, set(Ref(plugin), point.value()));
        EXPECT_CALL(parameter, getEnvelope(Ref(plugin))).Times(0);
        element->apply(parameter, plugin);
    }

    SECTION("When more than one point") {
        auto blockRange = ObjectTypeBlockRange{}.with(initialSphericalBlock()).followedBy(SphericalCoordBlock{}.withAzimuth(60.0));
        for(auto& block : blockRange.asConstRange()) {
            channelFormat->add(block);
        }
        auto element = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormat, packFormat, adm::AudioTrackUid::create()}, parentTake);

        EXPECT_CALL(parameter, set(_, point.value())).Times(AnyNumber());
        EXPECT_CALL(parameter, getEnvelope(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, createPoints(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, addPoint(_)).Times(AnyNumber());

        SECTION("apply() sets parameter directly") {
          EXPECT_CALL(parameter, set(Ref(plugin), point.value()));
          element->apply(parameter, plugin);
        }
        SECTION("apply() creates envelope") {
          EXPECT_CALL(parameter, getEnvelope(Ref(plugin)));
          element->apply(parameter, plugin);
        }

        SECTION("apply() adds 2 points") {
          EXPECT_CALL(envRef, addPoint(_)).Times(2);
          element->apply(parameter, plugin);
        }

        SECTION("apply() creates points") {
          EXPECT_CALL(envRef, createPoints(_)).Times(1);
          element->apply(parameter, plugin);
        }
    }
}
TEST_CASE("JumpPosition Insertion"){

    auto channelFormat = objectChannel();
    auto packFormat = objectPack();
    auto api = NiceMock<MockReaperAPI>{};
    using ns = std::chrono::nanoseconds;
    auto parentTake = std::make_shared<NiceMock<MockTakeElement>>();
    auto element = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormat, packFormat, adm::AudioTrackUid::create()}, parentTake);

    MockPlugin plugin;
    auto envelope = std::make_unique<MockAutomationEnvelope>();
    auto& envRef = *envelope;

    MockPluginParameter parameter;
    ON_CALL(parameter, admParameter()).WillByDefault(Return(AdmParameter::OBJECT_AZIMUTH));
    EXPECT_CALL(parameter, admParameter()).Times(AnyNumber());
    AutomationPoint point{ns::zero(), ns::zero(), 1.0};
    ON_CALL(parameter, forwardMap(Matcher<AutomationPoint>(_))).WillByDefault(Return(point));
    EXPECT_CALL(parameter, forwardMap(Matcher<AutomationPoint>(_))).Times(AnyNumber());
    ON_CALL(parameter, forwardMap(Matcher<double>(_))).WillByDefault(Return(point.value()));
    EXPECT_CALL(parameter, forwardMap(Matcher<double>(_))).Times(AnyNumber());
    ON_CALL(parameter, getEnvelope(_)).WillByDefault(Return(ByMove(std::move(envelope))));

    SECTION("Jump Position Scenario 1: No jump position"){
        auto blockRange = ObjectTypeBlockRange{}.with(initialSphericalBlock().withDistance(0.0).withJumpPosition(false).withDuration(1.0))
                .followedBy(SphericalCoordBlock{}.withDistance(55.0).withJumpPosition(false).withDuration(1.0))
                .followedBy(SphericalCoordBlock{}.withDistance(20.0).withJumpPosition(false).withDuration(1.0));
        for(auto& block : blockRange.asConstRange()) {
            channelFormat->add(block);
        }
        auto element = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormat, packFormat, adm::AudioTrackUid::create()}, parentTake);

        EXPECT_CALL(parameter, set(_, point.value())).Times(1);
        EXPECT_CALL(parameter, getEnvelope(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, createPoints(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, addPoint(_)).Times(AnyNumber());

        SECTION("apply() adds 3 points") {
          EXPECT_CALL(envRef, addPoint(_)).Times(3);
          element->apply(parameter, plugin);
        }

        SECTION("apply() creates points") {
          EXPECT_CALL(envRef, createPoints(_)).Times(1);
          element->apply(parameter, plugin);
        }

        REQUIRE(element->blocks().size() == 3);

    }

    SECTION("Jump Position Scenario 2: With jump position"){
        auto blockRange = ObjectTypeBlockRange{}.with(initialSphericalBlock().withDistance(20.0).withJumpPosition(true).withDuration(1.0))
                .followedBy(SphericalCoordBlock{}.withDistance(55.0).withJumpPosition(true).withDuration(1.0))
                .followedBy(SphericalCoordBlock{}.withDistance(40.0).withJumpPosition(true).withDuration(1.0));
        for(auto& block : blockRange.asConstRange()) {
            channelFormat->add(block);
        }
        auto element = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormat, packFormat, adm::AudioTrackUid::create()}, parentTake);

        EXPECT_CALL(parameter, set(_, point.value())).Times(1);
        EXPECT_CALL(parameter, getEnvelope(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, createPoints(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, addPoint(_)).Times(AnyNumber());

        SECTION("apply() adds 6 points") {
          EXPECT_CALL(envRef, addPoint(_)).Times(6);
          element->apply(parameter, plugin);
        }

        SECTION("apply() creates points") {
          EXPECT_CALL(envRef, createPoints(_)).Times(1);
          element->apply(parameter, plugin);
        }

        REQUIRE(element->blocks().size() == 3);
    }


    SECTION("Jump Position Scenario 3: With jump position and interpolation"){
        auto blockRange = ObjectTypeBlockRange{}.with(initialSphericalBlock().withDistance(20.0).withJumpPosition(true, std::chrono::nanoseconds{300}).withDuration(1.0))
                .followedBy(SphericalCoordBlock{}.withDistance(55.0).withJumpPosition(true, std::chrono::nanoseconds{300}).withDuration(1.0))
                .followedBy(SphericalCoordBlock{}.withDistance(40.0).withJumpPosition(true, std::chrono::nanoseconds{300}).withDuration(1.0));
        for(auto& block : blockRange.asConstRange()) {
            channelFormat->add(block);
        }
        auto element = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormat, packFormat, adm::AudioTrackUid::create()}, parentTake);

        EXPECT_CALL(parameter, set(_, point.value())).Times(1);
        EXPECT_CALL(parameter, getEnvelope(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, createPoints(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, addPoint(_)).Times(AnyNumber());

        SECTION("apply() adds 6 points") {
          EXPECT_CALL(envRef, addPoint(_)).Times(6);
          element->apply(parameter, plugin);
        }

        SECTION("apply() creates points") {
          EXPECT_CALL(envRef, createPoints(_)).Times(1);
          element->apply(parameter, plugin);
        }

        REQUIRE(element->blocks().size() == 3);
    }

    SECTION("Jump Position Scenario 4: With jump position and zero length blocks"){
        auto blockRange = ObjectTypeBlockRange{}.with(initialSphericalBlock().withDistance(55.0).withJumpPosition(true).withDuration(0.0))
                .followedBy(SphericalCoordBlock{}.withDistance(20.0).withJumpPosition(false).withDuration(1.0))
                .followedBy(SphericalCoordBlock{}.withDistance(55.0).withJumpPosition(true).withDuration(0.0))
                .followedBy(SphericalCoordBlock{}.withDistance(20.0).withJumpPosition(false).withDuration(1.0))
                .followedBy(SphericalCoordBlock{}.withDistance(55.0).withJumpPosition(true).withDuration(0.0))
                .followedBy(SphericalCoordBlock{}.withDistance(20.0).withJumpPosition(false).withDuration(1.0));
        for(auto& block : blockRange.asConstRange()) {
            channelFormat->add(block);
        }
        auto element = std::make_unique<ObjectAutomationElement>(ADMChannel{channelFormat, packFormat, adm::AudioTrackUid::create()}, parentTake);

        EXPECT_CALL(parameter, set(_, point.value())).Times(1);
        EXPECT_CALL(parameter, getEnvelope(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, createPoints(_)).Times(AnyNumber());
        EXPECT_CALL(envRef, addPoint(_)).Times(AnyNumber());

        SECTION("apply() adds 5 points") {
          EXPECT_CALL(envRef, addPoint(_)).Times(6);
          element->apply(parameter, plugin);
        }


        SECTION("apply() creates points") {
          EXPECT_CALL(envRef, createPoints(_)).Times(1);
          element->apply(parameter, plugin);
        }


        REQUIRE(element->blocks().size() == 6);
    }
}
*/