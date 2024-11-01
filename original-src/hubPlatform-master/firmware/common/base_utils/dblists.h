#ifndef DBLISTS_H
#define DBLISTS_H

#include <string>

#include "base_utils/ezlodb.h"
#include "device_model/h/permission.h"

JsonArray getLimitedResult( const JsonObject& params,
                            const Resource::Resources resource,
                            const std::string& dbName,
                            const Db::Query query = JSON_EMPTY_OBJ,
                            const Json* fieldsToReturn = nullptr,
                            int nToReturn = 0,
                            int nToSkip = 0 );
void getList( const JsonObject& params,
              JsonObject& answer,
              const Resource::Resources resource,
              const std::string& dbName,
              const Db::Query query = JSON_EMPTY_OBJ );
bool getListById( const JsonObject& params,
                  JsonObject& answer,
                  const Resource::Resources resource,
                  const std::string& dbName );

#endif  // DBLISTS_H
