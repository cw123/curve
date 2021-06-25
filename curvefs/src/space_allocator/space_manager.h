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

#ifndef CURVEFS_SRC_SPACE_ALLOCATOR_SPACE_MANAGER_H_
#define CURVEFS_SRC_SPACE_ALLOCATOR_SPACE_MANAGER_H_

#include <bthread/mutex.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "curvefs/src/space_allocator/bitmap_allocator.h"
#include "curvefs/src/space_allocator/rw_lock.h"
#include "curvefs/src/space_allocator/s3_allocator.h"

namespace curvefs {

namespace mds {
class FsInfo;
}

namespace space {

struct SpaceManagerOption {};

class SpaceManager {
 public:
    SpaceManager() = default;
    virtual ~SpaceManager() = default;

    virtual SpaceStatusCode InitSpace(const mds::FsInfo& fsInfo) = 0;

    virtual SpaceStatusCode UnInitSpace(uint32_t fsId) = 0;

    virtual SpaceStatusCode StatSpace(uint32_t fsId, uint64_t* total,
                                      uint64_t* available,
                                      uint64_t* blkSize) = 0;

    virtual SpaceStatusCode AllocateSpace(uint32_t fsId, uint32_t size,
                                          const SpaceAllocateHint& hint,
                                          std::vector<PExtent>* exts) = 0;

    virtual SpaceStatusCode DeallocateSpace(
        uint32_t fsId,
        const ::google::protobuf::RepeatedPtrField<::curvefs::space::Extent>&
            extents) = 0;

    virtual SpaceStatusCode AllocateS3Chunk(uint32_t fsId, uint64_t* id) = 0;

 private:
    SpaceManager(const SpaceManager&);
    SpaceManager& operator=(const SpaceManager&);
};

class DefaultSpaceManager : public SpaceManager {
 public:
    explicit DefaultSpaceManager(const SpaceManagerOption& opt);

    SpaceStatusCode InitSpace(const mds::FsInfo& fsInfo);

    SpaceStatusCode UnInitSpace(uint32_t fsId);

    SpaceStatusCode StatSpace(uint32_t fsId, uint64_t* total,
                              uint64_t* available, uint64_t* blkSize);

    SpaceStatusCode AllocateSpace(uint32_t fsId, uint32_t size,
                                  const SpaceAllocateHint& hint,
                                  std::vector<PExtent>* exts);

    SpaceStatusCode DeallocateSpace(
        uint32_t fsId,
        const ::google::protobuf::RepeatedPtrField<::curvefs::space::Extent>&
            extents);

    SpaceStatusCode AllocateS3Chunk(uint32_t fsId, uint64_t* id) override;

 private:
    SpaceStatusCode InitS3Space(const mds::FsInfo& fsInfo);

    SpaceStatusCode InitBlockSpace(const mds::FsInfo& fsInfo);

 private:
    SpaceManagerOption opt_;
    BthreadRWLock blkRwlock_;
    std::unordered_map<int, std::unique_ptr<BitmapAllocator>> blkAllocators_;

    BthreadRWLock s3Rwlock_;
    std::unordered_map<int, std::unique_ptr<S3Allocator>> s3Allocator_;
};

}  // namespace space
}  // namespace curvefs

#endif  // CURVEFS_SRC_SPACE_ALLOCATOR_SPACE_MANAGER_H_
