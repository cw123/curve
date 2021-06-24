/*
 *  Copyright (c) 2021 NetEase Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * @Project: curve
 * @Date: 2021-06-10 10:04:37
 * @Author: chenwei
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <brpc/channel.h>
#include <brpc/server.h>
#include "curvefs/src/mds/fs_manager.h"
#include "curvefs/test/mds/mock_metaserver.h"
#include "curvefs/test/mds/mock_space.h"

using ::testing::AtLeast;
using ::testing::StrEq;
using ::testing::_;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::SaveArg;
using ::testing::Mock;
using ::testing::Invoke;
using ::curvefs::space::MockSpaceService;
using ::curvefs::metaserver::MockMetaserverService;
using curvefs::metaserver::CreateRootInodeRequest;
using curvefs::metaserver::CreateRootInodeResponse;
using curvefs::metaserver::MetaStatusCode;

namespace curvefs {
namespace mds {
class FSManagerTest: public ::testing::Test {
 protected:
    void SetUp() override {
        std::string addr = "127.0.0.1:6700";
        SpaceOptions spaceOptions;
        spaceOptions.spaceAddr = addr;
        spaceOptions.rpcTimeoutMs = 500;
        MetaserverOptions metaserverOptions;
        metaserverOptions.metaserverAddr = addr;
        metaserverOptions.rpcTimeoutMs = 500;
        fsStorage_ = std::make_shared<MemoryFsStorage>();
        spaceClient_ = std::make_shared<SpaceClient>(spaceOptions);
        metaserverClient_ = std::make_shared<MetaserverClient>(
                                        metaserverOptions);
        fsManager_ = std::make_shared<FsManager>(fsStorage_, spaceClient_,
                                                metaserverClient_);
        ASSERT_TRUE(spaceClient_->Init());
        ASSERT_TRUE(metaserverClient_->Init());

        ASSERT_EQ(0, server_.AddService(&mockSpaceService_,
                                        brpc::SERVER_DOESNT_OWN_SERVICE));
        ASSERT_EQ(0, server_.AddService(&mockMetaserverService_,
                                        brpc::SERVER_DOESNT_OWN_SERVICE));
        ASSERT_EQ(0, server_.Start(addr.c_str(), nullptr));

        return;
    }

    void TearDown() override {
        server_.Stop(0);
        server_.Join();
        return;
    }

 protected:
    std::shared_ptr<FsManager> fsManager_;
    std::shared_ptr<FsStorage> fsStorage_;
    std::shared_ptr<SpaceClient> spaceClient_;
    std::shared_ptr<MetaserverClient> metaserverClient_;
    MockSpaceService mockSpaceService_;
    MockMetaserverService mockMetaserverService_;
    brpc::Server server_;
};

template <typename RpcRequestType, typename RpcResponseType,
          bool RpcFailed = false>
void RpcService(google::protobuf::RpcController *cntl_base,
                const RpcRequestType *request, RpcResponseType *response,
                google::protobuf::Closure *done) {
    if (RpcFailed) {
        brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);
        cntl->SetFailed(112, "Not connected to");
    }
    done->Run();
}

TEST_F(FSManagerTest, test1) {
    ASSERT_EQ(1, 1);
    FSStatusCode ret;
    std::string fsName = "fs1";
    uint64_t blockSize = 4096;
    curvefs::common::Volume volume;
    FsInfo fsInfo1;
    CreateRootInodeResponse response;
    response.set_statuscode(MetaStatusCode::OK);
    EXPECT_CALL(mockMetaserverService_, CreateRootInode(_, _, _, _))
        .WillOnce(
            DoAll(SetArgPointee<2>(response),
                  Invoke(RpcService<CreateRootInodeRequest,
                                    CreateRootInodeResponse>)));

    ret = fsManager_->CreateFs(fsName, blockSize, volume, &fsInfo1);
    ASSERT_EQ(ret, FSStatusCode::OK);
}
}  // namespace mds
}  // namespace curvefs
