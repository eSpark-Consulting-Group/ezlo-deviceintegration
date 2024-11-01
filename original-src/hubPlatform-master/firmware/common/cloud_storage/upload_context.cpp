#include "upload_context.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include <fstream>
#include <utility>

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

struct Iterator {
    size_t totalData = 0;
    size_t currentPos = 0;

    bool endOfStream() const {
        return currentPos >= totalData;
    }
};

class UploadFromFileContext : public UploadContext {
public:
    explicit UploadFromFileContext( std::string fileName );
    ~UploadFromFileContext() override;
    std::string readPortion( size_t portionSize ) override;
    bool endOfStream() const override;
    bool error() const override;

    const std::string& fileName() const;
    size_t streamSize() const;
    void close();

private:
    std::string m_fileName;
    std::ifstream m_stream;
    Iterator m_iterator;
};

UploadFromFileContext::UploadFromFileContext( std::string fileName )
: m_fileName( std::move( fileName ) ) {
    if( m_fileName.empty() )
        return;

    m_stream.open( m_fileName, std::ios::binary | std::ios::ate );
    if( m_stream ) {
        m_iterator.totalData = m_stream.tellg();
        m_stream.seekg( 0, std::ios::beg );
        m_iterator.currentPos = m_stream.tellg();

        LOGGER_DEBUG( "File stream opened: " + m_fileName + ", stream size: " + std::to_string( streamSize() ) );
    }
    else {
        LOGGER_ERROR( "Stream open failed: " + m_fileName );
    }
}

UploadFromFileContext::~UploadFromFileContext() {
    close();
}

std::string UploadFromFileContext::readPortion( size_t portionSize ) {
    if( portionSize == 0 )
        portionSize = PORTION_SIZE_DEFAULT;

    if( m_stream && !endOfStream() ) {
        std::string buffer( portionSize, '\0' );
        auto readSize = m_stream.readsome( const_cast< char* >( buffer.data() ), portionSize );
        m_iterator.currentPos += readSize;
        if( readSize < buffer.size() ) {
            // truncate:
            buffer.resize( readSize );
        }

        return buffer;
    }
    return {};
}

bool UploadFromFileContext::endOfStream() const {
    return m_stream.eof() || m_iterator.endOfStream();
}

bool UploadFromFileContext::error() const {
    return m_stream.fail();
}

const std::string& UploadFromFileContext::fileName() const {
    return m_fileName;
}

size_t UploadFromFileContext::streamSize() const {
    return m_iterator.totalData;
}

void UploadFromFileContext::close() {
    if( m_stream.is_open() ) {
        m_stream.close();
        m_fileName.clear();
        m_iterator = Iterator();
    }
}

UploadContextPtr makeFileUploadContext( std::string fileName ) {
    return std::make_shared< UploadFromFileContext >( std::move( fileName ) );
}

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform