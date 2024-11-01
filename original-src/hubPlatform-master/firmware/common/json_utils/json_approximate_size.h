#pragma once
#include "json_fwd.h"

size_t jsonApproximateSize( const Json& json );

size_t jsonObjectSize( const JsonObject& json );
size_t jsonArraySize( const JsonObject& json );