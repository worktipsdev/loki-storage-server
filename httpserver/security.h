#pragma once

#include <string>

#include <boost/filesystem/path.hpp>

namespace worktips {

struct worktipsd_key_pair_t;

class Security {
  public:
    Security(const worktipsd_key_pair_t& key_pair,
             const boost::filesystem::path& base_path);

    std::string base64_sign(const std::string& body);
    void generate_cert_signature();
    std::string get_cert_signature() const;

  private:
    const worktipsd_key_pair_t& key_pair_;
    std::string cert_signature_;
    boost::filesystem::path base_path_;
};
} // namespace worktips