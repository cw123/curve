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
 * @Date: 2021-06-24 11:21:36
 * @Author: chenwei
 */

#include "curvefs/src/mds/metaserver_client.h"

using curvefs::metaserver::CreateRootInodeRequest;
using curvefs::metaserver::CreateRootInodeResponse;
using curvefs::metaserver::MetaStatusCode;
using curvefs::metaserver::MetaServerService_Stub;

namespace curvefs {
namespace mds {
bool MetaserverClient::Init() {
    if (channel_.Init(options_.metaserverAddr.c_str(), nullptr) != 0) {
        LOG(ERROR) << "Init channel to metaserver: " << options_.metaserverAddr
                  << " failed!";
        return false;
    }
    inited_ = true;
    return true;
}

FSStatusCode MetaserverClient::CreateRootInode(uint32_t fsId,
                             uint32_t uid, uint32_t gid, uint32_t mode) {
    if (!inited_) {
        LOG(ERROR) << "MetaserverClient not Init, init first";
        return FSStatusCode::UNKNOWN_ERROR;
    }

    CreateRootInodeRequest request;
    CreateRootInodeResponse response;

    brpc::Controller cntl;
    cntl.set_timeout_ms(options_.rpcTimeoutMs);

    MetaServerService_Stub stub(&channel_);
    request.set_fsid(fsId);
    request.set_uid(uid);
    request.set_gid(gid);
    request.set_mode(mode);

    stub.CreateRootInode(&cntl, &request, &response, nullptr);

    if (cntl.Failed()) {
        LOG(ERROR) << "CreateInode failed, Rpc error = " << cntl.ErrorText();
        return FSStatusCode::RPC_ERROR;
    }

    if (response.statuscode() != MetaStatusCode::OK) {
        LOG(ERROR) << "CreateInode failed, ret = "
                   << FSStatusCode::INSERT_ROOT_INODE_ERROR;
        return FSStatusCode::INSERT_ROOT_INODE_ERROR;
    }
    return FSStatusCode::OK;
}
}  // namespace mds
}  // namespace curvefs
