#pragma once

#include <iosfwd>
#include <memory>

namespace HubPlatform {

namespace Common {
namespace CloudStorageInterface {

enum { PORTION_SIZE_DEFAULT = 4096 };

class UploadContext {
public:
    virtual ~UploadContext() = default;
    virtual std::string readPortion( size_t portionSize ) = 0;
    virtual bool endOfStream() const = 0;
    virtual bool error() const = 0;
};
using UploadContextPtr = std::shared_ptr< UploadContext >;
UploadContextPtr makeFileUploadContext( std::string fileName );

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform