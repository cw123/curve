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
 * Project: curve
 * Created Date: 2021-05-19
 * Author: chenwei
 */
#include <glog/logging.h>
#include "curvefs/src/metaserver/inode_manager.h"
#include "curvefs/src/common/define.h"
#include "src/common/timeutility.h"

namespace curvefs {
namespace metaserver {
MetaStatusCode InodeManager::CreateInode(uint32_t fsId, uint64_t length,
                                uint32_t uid, uint32_t gid, uint32_t mode,
                                FsFileType type, std::string symlink,
                                Inode *newInode) {
    LOG(INFO) << "CreateInode, fsId = " << fsId
              << ", length = " << length
              << ", uid = " << uid
              << ", gid = " << gid
              << ", mode = " << mode
              << ", type =" << FsFileType_Name(type)
              << ", symlink = " << symlink;
    if (type == FsFileType::TYPE_SYM_LINK
        && symlink.empty()) {
        return MetaStatusCode::SYM_LINK_EMPTY;
    }

    // 1. generate inode
    Inode inode;
    inode.set_inodeid(GetNextId());
    inode.set_fsid(fsId);
    inode.set_length(length);
    inode.set_uid(uid);
    inode.set_gid(gid);
    inode.set_mode(mode);
    inode.set_type(type);
    inode.set_mtime(curve::common::TimeUtility::GetTimeofDayMs());
    inode.set_atime(curve::common::TimeUtility::GetTimeofDayMs());
    inode.set_ctime(curve::common::TimeUtility::GetTimeofDayMs());
    inode.set_nlink(0);  // TODO(cw123): nlink now is all 0
    if (type == FsFileType::TYPE_SYM_LINK) {
        inode.set_symlink(symlink);
    }
    // 2. insert inode
    MetaStatusCode status = inodeStorage_->Insert(inode);
    if (status != MetaStatusCode::OK) {
        LOG(ERROR) << "CreateInode fail, fsId = " << fsId
              << ", length = " << length
              << ", uid = " << uid
              << ", gid = " << gid
              << ", mode = " << mode
              << ", type =" << FsFileType_Name(type)
              << ", symlink = " << symlink
              << ", ret = " << MetaStatusCode_Name(status);
        return status;
    }

    newInode->CopyFrom(inode);
    LOG(INFO) << "CreateInode success, fsId = " << fsId
              << ", length = " << length
              << ", uid = " << uid
              << ", gid = " << gid
              << ", mode = " << mode
              << ", type =" << FsFileType_Name(type)
              << ", symlink = " << symlink << " ," << inode.DebugString();

    return MetaStatusCode::OK;
}
MetaStatusCode InodeManager::CreateRootInode(uint32_t fsId, uint32_t uid,
                                uint32_t gid, uint32_t mode) {
    LOG(INFO) << "CreateRootInode, fsId = " << fsId
              << ", uid = " << uid
              << ", gid = " << gid
              << ", mode = " << mode;

    // 1. generate inode
    Inode inode;
    inode.set_inodeid(ROOTINODEID);
    inode.set_fsid(fsId);
    inode.set_length(0);
    inode.set_uid(uid);
    inode.set_gid(gid);
    inode.set_mode(mode);
    inode.set_type(FsFileType::TYPE_DIRECTORY);
    inode.set_mtime(curve::common::TimeUtility::GetTimeofDayMs());
    inode.set_atime(curve::common::TimeUtility::GetTimeofDayMs());
    inode.set_ctime(curve::common::TimeUtility::GetTimeofDayMs());
    inode.set_nlink(0);  // TODO(cw123): nlink now is all 0

    // 2. insert inode
    MetaStatusCode status = inodeStorage_->Insert(inode);
    if (status != MetaStatusCode::OK) {
        LOG(ERROR) << "CreateRootInode fail, fsId = " << fsId
              << ", uid = " << uid
              << ", gid = " << gid
              << ", mode = " << mode
              << ", ret = " << MetaStatusCode_Name(status);
        return status;
    }

    LOG(INFO) << "CreateRootInode success, inode: " << inode.DebugString();
    return MetaStatusCode::OK;
}

uint64_t InodeManager::GetNextId() {
    return nextInodeId_.fetch_add(1, std::memory_order_relaxed);
}

MetaStatusCode InodeManager::GetInode(uint32_t fsId, uint64_t inodeId,
                                        Inode *inode) {
    LOG(INFO) << "GetInode, fsId = " << fsId
              << ", inodeId = " << inodeId;

    // if (inodeId == 1) {
    //     inode->set_inodeid(1);
    //     inode->set_fsid(fsId);
    //     inode->set_length(0);
    //     inode->set_uid(0);
    //     inode->set_gid(0);
    //     inode->set_mode(S_IFDIR | 0777);
    //     inode->set_type(FsFileType::TYPE_DIRECTORY);
    //     inode->set_mtime(curve::common::TimeUtility::GetTimeofDayMs());
    //     inode->set_atime(curve::common::TimeUtility::GetTimeofDayMs());
    //     inode->set_ctime(curve::common::TimeUtility::GetTimeofDayMs());
    //     inode->set_nlink(0);
    //     LOG(INFO) << "GetInode rootinode success, fsId = " << fsId
    //               << ", inodeId = " << inodeId
    //               << ", " << inode->DebugString();
    //     return MetaStatusCode::OK;
    // }

    MetaStatusCode status = inodeStorage_->Get(InodeKey(fsId, inodeId),
                                    inode);
    if (status != MetaStatusCode::OK) {
        LOG(ERROR) << "GetInode fail, fsId = " << fsId
              << ", inodeId = " << inodeId
              << ", ret = " << MetaStatusCode_Name(status);
        return status;
    }

    LOG(INFO) << "GetInode success, fsId = " << fsId
              << ", inodeId = " << inodeId
              << ", " << inode->DebugString();

    return MetaStatusCode::OK;
}

MetaStatusCode InodeManager::DeleteInode(uint32_t fsId, uint64_t inodeId) {
    LOG(INFO) << "DeleteInode, fsId = " << fsId
              << ", inodeId = " << inodeId;
    MetaStatusCode status = inodeStorage_->Delete(InodeKey(fsId, inodeId));
    if (status != MetaStatusCode::OK) {
        LOG(ERROR) << "DeleteInode fail, fsId = " << fsId
              << ", inodeId = " << inodeId
              << ", ret = " << MetaStatusCode_Name(status);
        return status;
    }

    LOG(INFO) << "DeleteInode success, fsId = " << fsId
              << ", inodeId = " << inodeId;
    return MetaStatusCode::OK;
}

MetaStatusCode InodeManager::UpdateInode(const Inode &inode) {
    LOG(INFO) << "UpdateInode, " << inode.DebugString();
    MetaStatusCode status = inodeStorage_->Update(inode);
    if (status != MetaStatusCode::OK) {
        LOG(ERROR) << "UpdateInode fail, " << inode.DebugString()
                  << ", ret = " << MetaStatusCode_Name(status);
        return status;
    }

    LOG(INFO) << "UpdateInode success, " << inode.DebugString();
    return MetaStatusCode::OK;
}
}  // namespace metaserver
}  // namespace curvefs
