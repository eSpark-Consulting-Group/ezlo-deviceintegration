#include "smtp_client/smtp_client.h"

#include "cli11/cli11.h"

#include <string>
#include <iostream>

struct EmailMessage {
    std::string from;
    std::string to;
    std::string topic;
    std::string body;
};

void parseArgs( int argc, char* argv[], HubPlatform::SMTPClient::Settings& settings, EmailMessage& email ) {
    CLI::App app{ "Email sender test program" };

    app.add_option( "--server", settings.smtpServerUrl, "SMTP server URL: smtp://host:port or smtps://host:port" )
        ->required();
    app.add_option( "--from", email.from, "Sender email address" )->required();
    app.add_option( "--to", email.to, "Recipient email address" )->required();
    app.add_option( "--topic", email.topic, "Email topic" )->required();
    app.add_option( "--body", email.body, "Email body" )->required();

    app.add_option( "--encryption", settings.encryption, "Use TLS encryption" );
    app.add_option( "--verify-server-origin",
                    settings.verifyServerCertificateOrigin,
                    "Vefiry server authenticity with known ca-certificate" );
    app.add_option( "--server-ca-cert",
                    settings.serverCertificateAuthorityFile,
                    "Use ca-certificate to vefiry server authenticity" );
    app.add_option( "--verify-server-hostname",
                    settings.verifyServerHostname,
                    "Server hostname must be mentioned in certificate CN or SAN fields" );

    app.add_option(
        "--client-cert", settings.clientCertificateFile, "Add client certificate for server side validation" );
    app.add_option( "--client-cert-key",
                    settings.clientCertificateKeyFile,
                    "Add client certificate key for server side validation" );

    app.add_option( "--username", settings.clientUsername, "Add client username" );
    app.add_option( "--password", settings.clientPassword, "Add client password" );

    app.add_flag( "-v,--verbose", settings.verbose, "Verbose output" );

    try {
        app.parse( argc, argv );
    }
    catch( const CLI::ParseError& e ) {
        std::exit( app.exit( e ) );
    }
}

int main( int argc, char* argv[] ) {
    HubPlatform::SMTPClient::Settings settings;
    EmailMessage email;

    parseArgs( argc, argv, settings, email );

    HubPlatform::SMTPClient sender( std::move( settings ) );

    if( sender.send( email.from, email.to, email.topic, email.body, std::time_t( nullptr ) ) )
        std::cout << "Email send!" << std::endl;
    else
        std::cerr << "Email send FAILED" << std::endl;

    return 0;
}
