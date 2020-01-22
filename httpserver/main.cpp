#include "channel_encryption.hpp"
#include "command_line.h"
#include "http_connection.h"
#include "worktips_logger.h"
#include "worktipsd_key.h"
#include "rate_limiter.h"
#include "security.h"
#include "service_node.h"
#include "swarm.h"
#include "version.h"
#include "utils.hpp"

#include <boost/filesystem.hpp>
#include <sodium.h>

#include <cstdlib>
#include <iostream>
#include <vector>

namespace fs = boost::filesystem;

static boost::optional<fs::path> get_home_dir() {

    /// TODO: support default dir for Windows
#ifdef WIN32
    return boost::none;
#endif

    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        return boost::none;

    return fs::path(pszHome);
}

constexpr int EXIT_INVALID_PORT = 2;

int main(int argc, char* argv[]) {

    worktips::command_line_parser parser;

    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        parser.print_usage();
        return EXIT_FAILURE;
    }

    auto options = parser.get_options();

    if (options.print_help) {
        parser.print_usage();
        return EXIT_SUCCESS;
    }

    if (options.data_dir.empty()) {
        if (auto home_dir = get_home_dir()) {
            if (options.testnet) {
                options.data_dir = (home_dir.get() / ".worktips" / "testnet" / "storage").string();
            } else {
                options.data_dir = (home_dir.get() / ".worktips" / "storage").string();
            }
        }
    }

    if (!fs::exists(options.data_dir)) {
        fs::create_directories(options.data_dir);
    }

    worktips::LogLevel log_level;
    if (!worktips::parse_log_level(options.log_level, log_level)) {
        std::cerr << "Incorrect log level: " << options.log_level << std::endl;
        worktips::print_log_levels();
        return EXIT_FAILURE;
    }

    worktips::init_logging(options.data_dir, log_level);

    if (options.testnet) {
        worktips::set_testnet();
        WORKTIPS_LOG(warn, "Starting in testnet mode, make sure this is intentional!");
    }

    // Always print version for the logs
    print_version();
    if (options.print_version) {
        return EXIT_SUCCESS;
    }

    if (options.ip == "127.0.0.1") {
        WORKTIPS_LOG(critical,
                 "Tried to bind worktips-storage to localhost, please bind "
                 "to outward facing address");
        return EXIT_FAILURE;
    }

    if (options.port == options.worktipsd_rpc_port) {
        WORKTIPS_LOG(error, "Storage server port must be different from that of "
                        "Worktipsd! Terminating.");
        exit(EXIT_INVALID_PORT);
    }

    WORKTIPS_LOG(info, "Setting log level to {}", options.log_level);
    WORKTIPS_LOG(info, "Setting database location to {}", options.data_dir);
    WORKTIPS_LOG(info, "Setting Worktipsd RPC to {}:{}", options.worktipsd_rpc_ip, options.worktipsd_rpc_port);
    WORKTIPS_LOG(info, "Listening at address {} port {}", options.ip, options.port);

    boost::asio::io_context ioc{1};
    boost::asio::io_context worker_ioc{1};

    if (sodium_init() != 0) {
        WORKTIPS_LOG(error, "Could not initialize libsodium");
        return EXIT_FAILURE;
    }

    {
        const auto fd_limit = util::get_fd_limit();
        if (fd_limit != -1) {
            WORKTIPS_LOG(debug, "Open file descriptor limit: {}", fd_limit);
        } else {
            WORKTIPS_LOG(debug, "Open descriptor limit: N/A");
        }
    }

    try {

        auto worktipsd_client = worktips::WorktipsdClient(ioc, options.worktipsd_rpc_ip, options.worktipsd_rpc_port);

        // Normally we request the key from daemon, but in integrations/swarm
        // testing we are not able to do that, so we extract the key as a
        // command line option:
        worktips::private_key_t private_key;
        worktips::private_key_ed25519_t private_key_ed25519; // Unused at the moment
        worktips::private_key_t private_key_x25519;
#ifndef INTEGRATION_TEST
        std::tie(private_key, private_key_ed25519, private_key_x25519) =
            worktipsd_client.wait_for_privkey();
#else
        private_key = worktips::worktipsdKeyFromHex(options.worktipsd_key);
        WORKTIPS_LOG(info, "WORKTIPSD LEGACY KEY: {}", options.worktipsd_key);

        private_key_x25519 = worktips::worktipsdKeyFromHex(options.worktipsd_x25519_key);
        WORKTIPS_LOG(info, "x25519 SECRET KEY: {}", options.worktipsd_x25519_key);

        private_key_ed25519 = worktips::private_key_ed25519_t::from_hex(options.worktipsd_ed25519_key);

        WORKTIPS_LOG(info, "ed25519 SECRET KEY: {}", options.worktipsd_ed25519_key);
#endif

        const auto public_key = worktips::derive_pubkey_legacy(private_key);
        WORKTIPS_LOG(info, "Retrieved keys from Worktipsd; our SN pubkey is: {}",
                 util::as_hex(public_key));

        // TODO: avoid conversion to vector
        const std::vector<uint8_t> priv(private_key_x25519.begin(),
                                        private_key_x25519.end());
        ChannelEncryption<std::string> channel_encryption(priv);

        worktips::worktipsd_key_pair_t worktipsd_key_pair{private_key, public_key};

        const auto public_key_x25519 =
            worktips::derive_pubkey_x25519(private_key_x25519);

        WORKTIPS_LOG(info, "SN x25519 pubkey is: {}",
                 util::as_hex(public_key_x25519));

        const auto public_key_ed25519 =
            worktips::derive_pubkey_ed25519(private_key_ed25519);

        WORKTIPS_LOG(info, "SN ed25519 pubkey is: {}",
                 util::as_hex(public_key_ed25519));

        worktips::worktipsd_key_pair_t worktipsd_key_pair_x25519{private_key_x25519,
                                                     public_key_x25519};

        worktips::ServiceNode service_node(ioc, worker_ioc, options.port,
                                       worktipsd_key_pair, worktipsd_key_pair_x25519,
                                       options.data_dir, worktipsd_client,
                                       options.force_start);
        RateLimiter rate_limiter;

        worktips::Security security(worktipsd_key_pair, options.data_dir);

        /// Should run http server
        worktips::http_server::run(ioc, options.ip, options.port, options.data_dir,
                               service_node, channel_encryption, rate_limiter,
                               security);
    } catch (const std::exception& e) {
        // It seems possible for logging to throw its own exception,
        // in which case it will be propagated to libc...
        std::cerr << "Exception caught in main: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown exception caught in main." << std::endl;
        return EXIT_FAILURE;
    }
}
