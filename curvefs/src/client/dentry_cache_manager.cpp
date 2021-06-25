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
 * Created Date: Thur May 27 2021
 * Author: xuchaojie
 */
#include "curvefs/src/client/dentry_cache_manager.h"

#include <string>
#include <list>
#include <unordered_map>

namespace curvefs {
namespace client {

CURVEFS_ERROR DentryCacheManager::GetDentry(
    uint64_t parent, const std::string &name, Dentry *out) {
    curve::common::LockGuard lg(mtx_);
    CURVEFS_ERROR ret = CURVEFS_ERROR::OK;
    auto it = dCache_.find(parent);
    if (it != dCache_.end()) {
        auto ix = it->second.find(name);
        if (ix != it->second.end()) {
            *out = ix->second;
            return CURVEFS_ERROR::OK;
        }
    }
    ret = metaClient_->GetDentry(fsId_, parent, name, out);
    if (ret != CURVEFS_ERROR::OK) {
        LOG(ERROR) << "metaClient_ GetDentry failed, ret = " << ret
                   << ", parent = " << parent
                   << ", name = " << name;
        return ret;
    }
    if (it == dCache_.end()) {
        it = dCache_.emplace(parent,
            std::unordered_map<std::string, Dentry>()).first;
    }
    it->second.emplace(name, *out);
    return CURVEFS_ERROR::OK;
}

CURVEFS_ERROR DentryCacheManager::CreateDentry(const Dentry &dentry) {
    curve::common::LockGuard lg(mtx_);
    uint64_t parent = dentry.parentinodeid();
    std::string name = dentry.name();
    CURVEFS_ERROR ret = metaClient_->CreateDentry(dentry);
    if (ret != CURVEFS_ERROR::OK) {
        LOG(ERROR) << "metaClient_ CreateDentry failed, ret = " << ret
                   << ", parent = " << parent
                   << ", name = " << name;
        return ret;
    }
    auto it = dCache_.emplace(parent,
            std::unordered_map<std::string, Dentry>()).first;
    it->second.emplace(name, dentry);
    return CURVEFS_ERROR::OK;
}

CURVEFS_ERROR DentryCacheManager::DeleteDentry(
    uint64_t parent, const std::string &name) {
    curve::common::LockGuard lg(mtx_);
    CURVEFS_ERROR ret = metaClient_->DeleteDentry(fsId_, parent, name);
    if (ret != CURVEFS_ERROR::OK) {
        LOG(ERROR) << "metaClient_ DeleteInode failed, ret = " << ret
                   << ", parent = " << parent
                   << ", name = " << name;
        return ret;
    }
    auto it = dCache_.find(parent);
    if (it != dCache_.end()) {
        it->second.erase(name);
        if (it->second.empty()) {
            dCache_.erase(it);
        }
    }
    return CURVEFS_ERROR::OK;
}

CURVEFS_ERROR DentryCacheManager::ListDentry(
    uint64_t parent, std::list<Dentry> *dentryList) {
    bool perceed = true;
    CURVEFS_ERROR ret = CURVEFS_ERROR::OK;
    dentryList->clear();
    std::string last = "";
    do {
        std::list<Dentry> part;
        ret = metaClient_->ListDentry(fsId_, parent, 
            last, maxListCount_, &part);
        LOG(INFO) << "ListDentry fsId = " << fsId_
                  << ", parent = " << parent
                  << ", last = " << last
                  << ", count = " << maxListCount_
                  << ", ret = " << ret
                  << ", part.size() = " << part.size();
        if (ret != CURVEFS_ERROR::OK) {
            if (CURVEFS_ERROR::NOTEXIST == ret) {
                return CURVEFS_ERROR::OK;
            }
            LOG(ERROR) << "metaClient_ ListDentry failed, ret = " << ret
                       << ", parent = " << parent
                       << ", last = " << last
                       << ", count = " << maxListCount_;
            return ret;
        }
        if (!part.empty()) {
            last = part.back().name();
            dentryList->splice(dentryList->end(), part);
        }
        if (part.size() < maxListCount_) {
            perceed = false;
            break;
        }
    } while (perceed);
    return CURVEFS_ERROR::OK;
}

}  // namespace client
}  // namespace curvefs
