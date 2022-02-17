// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "test_roudi_portmanager_fixture.hpp"

namespace iox_test_roudi_portmanager
{
using namespace iox::popo;

constexpr uint64_t RESPONSE_QUEUE_CAPACITY{2U};
constexpr uint64_t REQUEST_QUEUE_CAPACITY{2U};

ClientOptions createTestClientOptions()
{
    return ClientOptions{RESPONSE_QUEUE_CAPACITY, iox::NodeName_t("node")};
}

ServerOptions createTestServerOptions()
{
    return ServerOptions{REQUEST_QUEUE_CAPACITY, iox::NodeName_t("node")};
}

// BEGIN aquireClientPortData tests

TEST_F(PortManager_test, AcquireClientPortDataReturnsPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "92225f2c-619a-425b-bba0-6a014822c4c3");
    const ServiceDescription sd{"hyp", "no", "toad"};
    const RuntimeName_t runtimeName{"hypnotoad"};
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = false;
    clientOptions.responseQueueFullPolicy = QueueFullPolicy::BLOCK_PRODUCER;
    clientOptions.serverTooSlowPolicy = ConsumerTooSlowPolicy::WAIT_FOR_CONSUMER;
    m_portManager->acquireClientPortData(sd, clientOptions, runtimeName, m_payloadDataSegmentMemoryManager, {})
        .and_then([&](const auto& clientPortData) {
            EXPECT_THAT(clientPortData->m_serviceDescription, Eq(sd));
            EXPECT_THAT(clientPortData->m_runtimeName, Eq(runtimeName));
            EXPECT_THAT(clientPortData->m_nodeName, Eq(clientOptions.nodeName));
            EXPECT_THAT(clientPortData->m_toBeDestroyed, Eq(false));
            EXPECT_THAT(clientPortData->m_chunkReceiverData.m_queue.capacity(),
                        Eq(clientOptions.responseQueueCapacity));
            EXPECT_THAT(clientPortData->m_connectRequested, Eq(clientOptions.connectOnCreate));
            EXPECT_THAT(clientPortData->m_chunkReceiverData.m_queueFullPolicy,
                        Eq(clientOptions.responseQueueFullPolicy));
            EXPECT_THAT(clientPortData->m_chunkSenderData.m_consumerTooSlowPolicy,
                        Eq(clientOptions.serverTooSlowPolicy));
        })
        .or_else([&](const auto& error) {
            GTEST_FAIL() << "Expected ClientPortData but got PortPoolError: " << static_cast<uint8_t>(error);
        });
}

// END aquireClientPortData tests

// BEGIN aquireServerPortData tests

TEST_F(PortManager_test, AcquireServerPortDataReturnsPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "776c51c4-074a-4404-b6a7-ed08f59f05a0");
    const ServiceDescription sd{"hyp", "no", "toad"};
    const RuntimeName_t runtimeName{"hypnotoad"};
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = false;
    serverOptions.requestQueueFullPolicy = QueueFullPolicy::BLOCK_PRODUCER;
    serverOptions.clientTooSlowPolicy = ConsumerTooSlowPolicy::WAIT_FOR_CONSUMER;
    m_portManager->acquireServerPortData(sd, serverOptions, runtimeName, m_payloadDataSegmentMemoryManager, {})
        .and_then([&](const auto& serverPortData) {
            EXPECT_THAT(serverPortData->m_serviceDescription, Eq(sd));
            EXPECT_THAT(serverPortData->m_runtimeName, Eq(runtimeName));
            EXPECT_THAT(serverPortData->m_nodeName, Eq(serverOptions.nodeName));
            EXPECT_THAT(serverPortData->m_toBeDestroyed, Eq(false));
            EXPECT_THAT(serverPortData->m_chunkReceiverData.m_queue.capacity(), Eq(serverOptions.requestQueueCapacity));
            EXPECT_THAT(serverPortData->m_offeringRequested, Eq(serverOptions.offerOnCreate));
            EXPECT_THAT(serverPortData->m_chunkReceiverData.m_queueFullPolicy,
                        Eq(serverOptions.requestQueueFullPolicy));
            EXPECT_THAT(serverPortData->m_chunkSenderData.m_consumerTooSlowPolicy,
                        Eq(serverOptions.clientTooSlowPolicy));
        })
        .or_else([&](const auto& error) {
            GTEST_FAIL() << "Expected ClientPortData but got PortPoolError: " << static_cast<uint8_t>(error);
        });
}

TEST_F(PortManager_test, AcquireServerPortDataWithSameServiceDescriptionTwiceCallsErrorHandlerAndReturnsError)
{
    ::testing::Test::RecordProperty("TEST_ID", "9f2c24ba-192d-4ce8-a61a-fe40b42c655b");
    const ServiceDescription sd{"hyp", "no", "toad"};
    const RuntimeName_t runtimeName{"hypnotoad"};
    auto serverOptions = createTestServerOptions();

    // first call must be successful
    m_portManager->acquireServerPortData(sd, serverOptions, runtimeName, m_payloadDataSegmentMemoryManager, {})
        .or_else([&](const auto& error) {
            GTEST_FAIL() << "Expected ClientPortData but got PortPoolError: " << static_cast<uint8_t>(error);
        });

    iox::cxx::optional<iox::Error> detectedError;
    auto errorHandlerGuard =
        iox::ErrorHandler::setTemporaryErrorHandler([&](const auto error, const auto, const auto errorLevel) {
            EXPECT_THAT(error, Eq(iox::Error::kPOSH__PORT_MANAGER_SERVERPORT_NOT_UNIQUE));
            EXPECT_THAT(errorLevel, Eq(iox::ErrorLevel::MODERATE));
            detectedError.emplace(error);
        });

    // second call must fail
    m_portManager->acquireServerPortData(sd, serverOptions, runtimeName, m_payloadDataSegmentMemoryManager, {})
        .and_then([&](const auto&) {
            GTEST_FAIL() << "Expected PortPoolError::UNIQUE_SERVER_PORT_ALREADY_EXISTS but got ServerPortData";
        })
        .or_else([&](const auto& error) { EXPECT_THAT(error, Eq(PortPoolError::UNIQUE_SERVER_PORT_ALREADY_EXISTS)); });

    EXPECT_TRUE(detectedError.has_value());
}

TEST_F(PortManager_test, AcquireServerPortDataWithSameServiceDescriptionTwiceAndFirstPortMarkedToBeDestroyedReturnsPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "d7f2815d-f1ea-403d-9355-69470d92a10f");
    const ServiceDescription sd{"hyp", "no", "toad"};
    const RuntimeName_t runtimeName{"hypnotoad"};
    auto serverOptions = createTestServerOptions();

    // first call must be successful
    auto serverPortDataResult =
        m_portManager->acquireServerPortData(sd, serverOptions, runtimeName, m_payloadDataSegmentMemoryManager, {});

    ASSERT_FALSE(serverPortDataResult.has_error());

    serverPortDataResult.value()->m_toBeDestroyed = true;

    iox::cxx::optional<iox::Error> detectedError;
    auto errorHandlerGuard = iox::ErrorHandler::setTemporaryErrorHandler(
        [&](const auto error, const auto, const auto) { detectedError.emplace(error); });

    // second call must now also succeed
    m_portManager->acquireServerPortData(sd, serverOptions, runtimeName, m_payloadDataSegmentMemoryManager, {})
        .or_else([&](const auto& error) {
            GTEST_FAIL() << "Expected ClientPortData but got PortPoolError: " << static_cast<uint8_t>(error);
        });

    detectedError.and_then(
        [&](const auto& error) { GTEST_FAIL() << "Expected error handler to not be called but got: " << error; });
}

// END aquireServerPortData tests

// BEGIN discovery tests

TEST_F(PortManager_test, CreateClientWithConnectOnCreateAndNoServerResultsInWaitForOffer)
{
    ::testing::Test::RecordProperty("TEST_ID", "14070d7b-d8e1-4df5-84fc-119e5e126cde");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;

    auto clientPortUser = createClient(clientOptions);

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::WAIT_FOR_OFFER));
}

TEST_F(PortManager_test, DoDiscoveryWithClientConnectOnCreateAndNoServerResultsInClientNotConnected)
{
    ::testing::Test::RecordProperty("TEST_ID", "6829e506-9f58-4253-bc42-469f2970a2c7");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;

    auto clientPortUser = createClient(clientOptions);
    m_portManager->doDiscovery();

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::WAIT_FOR_OFFER));
}

TEST_F(PortManager_test, CreateClientWithConnectOnCreateAndNotOfferingServerResultsInWaitForOffer)
{
    ::testing::Test::RecordProperty("TEST_ID", "0f7098d0-2646-4c10-b347-9b57b0f593ce");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = false;

    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser = createClient(clientOptions);

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::WAIT_FOR_OFFER));
}

TEST_F(PortManager_test, CreateClientWithConnectOnCreateAndOfferingServerResultsInClientConnected)
{
    ::testing::Test::RecordProperty("TEST_ID", "108170d4-786b-4266-ad2a-ef922188f70b");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;

    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser = createClient(clientOptions);

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::CONNECTED));
}

TEST_F(PortManager_test, CreateServerWithOfferOnCreateAndClientWaitingToConnectResultsInClientConnected)
{
    ::testing::Test::RecordProperty("TEST_ID", "b5bb10b2-bf9b-400e-ab5c-aa3a1e0e826f");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;

    auto clientPortUser = createClient(clientOptions);
    auto serverPortUser = createServer(serverOptions);

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::CONNECTED));
}

TEST_F(PortManager_test, CreateClientWithNotConnectOnCreateAndNoServerResultsInClientNotConnected)
{
    ::testing::Test::RecordProperty("TEST_ID", "fde662f1-f9e1-4302-be41-59a7a0bfa4e7");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = false;

    auto clientPortUser = createClient(clientOptions);

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::NOT_CONNECTED));
}

TEST_F(PortManager_test, DoDiscoveryWithClientNotConnectOnCreateAndNoServerResultsInClientNotConnected)
{
    ::testing::Test::RecordProperty("TEST_ID", "c59b7343-6277-4a4b-8204-506048726be4");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = false;

    auto clientPortUser = createClient(clientOptions);
    m_portManager->doDiscovery();

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::NOT_CONNECTED));
}

TEST_F(PortManager_test, CreateClientWithNotConnectOnCreateAndOfferingServerResultsInClientNotConnected)
{
    ::testing::Test::RecordProperty("TEST_ID", "17cf22ba-066a-418a-8366-1c6b75177b9a");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = false;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;

    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser = createClient(clientOptions);

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::NOT_CONNECTED));
}

TEST_F(PortManager_test, DoDiscoveryWithClientNotConnectOnCreateAndServerResultsInConnectedWhenCallingConnect)
{
    ::testing::Test::RecordProperty("TEST_ID", "87bbb991-4aaf-49c1-b238-d9b0bb18d699");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = false;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;

    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser = createClient(clientOptions);

    clientPortUser.connect();

    m_portManager->doDiscovery();

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::CONNECTED));
}

TEST_F(PortManager_test, DoDiscoveryWithClientConnectResultsInClientNotConnectedWhenCallingDisconnect)
{
    ::testing::Test::RecordProperty("TEST_ID", "b6826f93-096d-473d-b846-ab824efff1ee");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;

    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser = createClient(clientOptions);

    clientPortUser.disconnect();

    m_portManager->doDiscovery();

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::NOT_CONNECTED));
}

TEST_F(PortManager_test, DoDiscoveryWithClientConnectResultsInWaitForOfferWhenCallingStopOffer)
{
    ::testing::Test::RecordProperty("TEST_ID", "45c9cc27-4198-4539-943f-2111ae2d1368");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;

    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser = createClient(clientOptions);

    serverPortUser.stopOffer();

    m_portManager->doDiscovery();

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::WAIT_FOR_OFFER));
}

TEST_F(PortManager_test, DoDiscoveryWithClientConnectResultsInWaitForOfferWhenServerIsDestroyed)
{
    ::testing::Test::RecordProperty("TEST_ID", "585ad47d-1a03-4599-a4dc-57ea1fb6eac7");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;


    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser = createClient(clientOptions);

    serverPortUser.destroy();

    m_portManager->doDiscovery();

    EXPECT_THAT(clientPortUser.getConnectionState(), Eq(ConnectionState::WAIT_FOR_OFFER));
}

TEST_F(PortManager_test, DoDiscoveryWithClientConnectResultsInNoClientsWhenClientIsDestroyed)
{
    ::testing::Test::RecordProperty("TEST_ID", "3be2f7b5-7e22-4676-a25b-c8a93a4aaa7d");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;


    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser = createClient(clientOptions);

    EXPECT_TRUE(serverPortUser.hasClients());

    clientPortUser.destroy();

    m_portManager->doDiscovery();

    EXPECT_FALSE(serverPortUser.hasClients());
}

TEST_F(PortManager_test, CreateMultipleClientsWithConnectOnCreateAndOfferingServerResultsInAllClientsConnected)
{
    ::testing::Test::RecordProperty("TEST_ID", "08f9981f-2585-4574-b0fc-c16cf0eef7d4");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = true;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;

    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser1 = createClient(clientOptions);
    auto clientPortUser2 = createClient(clientOptions);

    EXPECT_THAT(clientPortUser1.getConnectionState(), Eq(ConnectionState::CONNECTED));
    EXPECT_THAT(clientPortUser2.getConnectionState(), Eq(ConnectionState::CONNECTED));
}

TEST_F(PortManager_test,
       DoDiscoveryWithMultipleClientsNotConnectedAndOfferingServerResultsSomeClientsConnectedWhenSomeClientsCallConnect)
{
    ::testing::Test::RecordProperty("TEST_ID", "7d210259-7c50-479e-b108-bf9747ceb0ef");
    auto clientOptions = createTestClientOptions();
    clientOptions.connectOnCreate = false;
    auto serverOptions = createTestServerOptions();
    serverOptions.offerOnCreate = true;

    auto serverPortUser = createServer(serverOptions);
    auto clientPortUser1 = createClient(clientOptions);
    auto clientPortUser2 = createClient(clientOptions);

    clientPortUser2.connect();
    m_portManager->doDiscovery();

    EXPECT_THAT(clientPortUser1.getConnectionState(), Eq(ConnectionState::NOT_CONNECTED));
    EXPECT_THAT(clientPortUser2.getConnectionState(), Eq(ConnectionState::CONNECTED));
}

// END discovery tests

} // namespace iox_test_roudi_portmanager
