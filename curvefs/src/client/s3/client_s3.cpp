/*
 *  Copyright (c) 2020 NetEase Inc.
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
 * Created Date: 21-5-31
 * Author: huyao
 */
#include "client_s3.h"

namespace curvefs {

namespace client { 
void S3ClientImpl::Init(curve::common::S3AdapterOption option) {
    s3Adapter_.Init(option);        
}

int S3ClientImpl::Upload(std::string name, const char* buf, uint64_t length) {
    std::string data;

    const Aws::String aws_key(name.c_str(), name.size());
    data.append(buf, 0, length);
    return s3Adapter_.PutObject(aws_key, data);
   
}
   
int S3ClientImpl::Download(std::string name, char* buf, uint64_t offset, uint64_t length) {
    int ret = 0;
    std::string data;

    const Aws::String aws_key(name.c_str(), name.size());
    ret = s3Adapter_.GetObject(aws_key, &data);
    if (ret < 0) {  
        return ret;
    }

    strncpy(buf, data.c_str(), length);

    return length;
}

int S3ClientImpl::Append(std::string name, const char*buf, uint64_t length) {
    std::string data;
    int ret = 0;

    const Aws::String aws_key(name.c_str(), name.size());
    ret = s3Adapter_.GetObject(aws_key, &data);
    if (ret < 0) {
        return ret;        
    }

    data.append(buf, 0, length);
    ret = s3Adapter_.PutObject(aws_key, data);
    
    return ret;
}

} // namespace client
} // namespace curvefs