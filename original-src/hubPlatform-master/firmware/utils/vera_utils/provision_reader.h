#pragma once
#ifdef __HUB_TYPE_G150__
#include "vera_utils/g150_reader_impl.h"
#elif __HUB_TYPE_G450__
#include "vera_utils/g450_reader_impl.h"
#elif __HUB_TYPE_G550__
#include "vera_utils/g550_reader_impl.h"
#else
#include "vera_utils/dummy_reader.h"
#endif

namespace HubPlatform {
namespace VeraProvision {

#ifdef __HUB_TYPE_G150__
using ProvisionReader = G150ReaderImpl;
#elif __HUB_TYPE_G450__
using ProvisionReader = G450ReaderImpl;
#elif __HUB_TYPE_G550__
using ProvisionReader = G550ReaderImpl;
#else
using ProvisionReader = DummyReader;
#endif

}  // namespace VeraProvision
}  // namespace HubPlatform
