#include "ssl_utils/x509.h"

#include <openssl/x509.h>
#include <openssl/pem.h>

#include <memory>

namespace HubPlatform {
namespace SslUtils {

using Asn1TimeUPtr = std::unique_ptr< ASN1_TIME, decltype( &ASN1_TIME_free ) >;

namespace {

std::string asn1TimeToString( ASN1_TIME* t ) {
    return std::string( reinterpret_cast< char* >( ASN1_STRING_data( t ) ), ASN1_STRING_length( t ) );
}

}  // namespace

std::string asn1Now() {
    std::unique_ptr< ASN1_TIME, decltype( &ASN1_TIME_free ) > now( ASN1_TIME_new(), ASN1_TIME_free );
    ASN1_TIME_set( now.get(), time( nullptr ) );
    return asn1TimeToString( now.get() );
}

bool asn1TimeDiff( int& days, int& seconds, const std::string& lhs, const std::string& rhs ) {
    Asn1TimeUPtr asn1lhs( ASN1_TIME_new(), ASN1_TIME_free );
    Asn1TimeUPtr asn1rhs( ASN1_TIME_new(), ASN1_TIME_free );

    if( !lhs.empty() ) {
        if( !ASN1_TIME_set_string( asn1lhs.get(), lhs.c_str() ) ) {
            return false;
        }
    }
    else {
        asn1lhs.reset();
    }

    if( !rhs.empty() ) {
        if( !ASN1_TIME_set_string( asn1rhs.get(), rhs.c_str() ) ) {
            return false;
        }
    }
    else {
        asn1rhs.reset();
    }

    return ASN1_TIME_diff( &days, &seconds, asn1lhs.get(), asn1rhs.get() );
}

bool asn1TimeLess( const std::string& lhs, const std::string& rhs ) {
    int day, sec;
    if( !asn1TimeDiff( day, sec, lhs, rhs ) ) {
        return true;
    }

    return ( day > 0 || sec > 0 );
}

std::pair< std::string, std::string > getValidityPeriod( const std::string& certificate ) {
    std::pair< std::string, std::string > result;

    std::unique_ptr< BIO, decltype( &BIO_free ) > certBio( BIO_new( BIO_s_mem() ), BIO_free );
    BIO_write( certBio.get(), certificate.c_str(), certificate.length() );

    std::unique_ptr< X509, decltype( &X509_free ) > certX509(
        PEM_read_bio_X509( certBio.get(), nullptr, nullptr, nullptr ), X509_free );

    if( !certX509 ) {
        return result;
    }

    ASN1_TIME* not_before = X509_get_notBefore( certX509 );
    ASN1_TIME* not_after = X509_get_notAfter( certX509 );

    result.first = asn1TimeToString( not_before );
    result.second = asn1TimeToString( not_after );

    return result;
}

int chooseValidPeriod( const ValidityPeriods& periods, const std::string& reference ) {
    std::string now = reference.empty() ? asn1Now() : reference;
    int result = -1;
    for( int idx = 0; idx < periods.size(); idx++ ) {
        auto& currentPeriod = periods[ idx ];
        // Check if current period is valid
        if( !asn1TimeLess( now, currentPeriod.first ) && !asn1TimeLess( currentPeriod.second, now ) ) {
            // Check if current period ends after previously chosen one
            if( result == -1 || ( asn1TimeLess( periods[ result ].second, currentPeriod.second ) ) ) {
                result = idx;
            }
        }
    }
    return result;
}

}  // namespace SslUtils
}  // namespace HubPlatform
