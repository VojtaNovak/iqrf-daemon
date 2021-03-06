/**
 * Copyright 2016-2017 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "JsonUtils.h"
#include <string>
#include <functional>

class IDaemon;
class ISerializer;
class IMessaging;

class IService
{
public:
  // component
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void update(const rapidjson::Value& cfg) =0;
  virtual const std::string& getClientName() const = 0;

  // references
  virtual void setDaemon(IDaemon* daemon) = 0;
  virtual void setSerializer(ISerializer* serializer) = 0;
  virtual void setMessaging(IMessaging* messaging) = 0;

  // interface
  inline virtual ~IService() {};
};
