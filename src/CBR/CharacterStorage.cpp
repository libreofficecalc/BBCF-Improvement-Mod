#include <CBR/CharacterStorage.h>
#include <CBR/httplib.h>
#include <iostream>
#include "json.hpp"
#include "CBR/CbrData.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/filesystem.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>


using namespace std;
using json = nlohmann::json;

#define CA_CERT_FILE "./ca-bundle.crt"

std::string decode64(const std::string& val) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
        return c == '\0';
        });
}

std::string encode64(const std::string& val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

std::string ConvertCbrDataToBase64(CbrData& cbr) {
    std::string s = "";
    std::stringstream outfile;
    {
        
        boost::iostreams::filtering_stream<boost::iostreams::output> f;
        f.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip_params(boost::iostreams::gzip::best_compression)));
        f.push(outfile);
        boost::archive::binary_oarchive archive(f);
        archive << cbr;
    }

    s = outfile.str();
    return encode64(s);
}

CbrData ConvertCbrDataFromBase64(std::string encodeS) {
    auto decodeS = decode64(encodeS);

    std:stringstream infile;
    infile << decodeS;

    CbrData insert;
    insert.setPlayerName("-1");
    if (infile.fail()) {
        //File does not exist code here
    }
    else {
        boost::iostreams::filtering_stream<boost::iostreams::input> f;
        f.push(boost::iostreams::gzip_decompressor());
        f.push(infile);
        boost::archive::binary_iarchive archive(f);
        CbrData insert;
        archive >> insert;
        return insert;
        
    }
    return insert;
}


void testHTTPPost(json& j) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli("localhost", 8080);
    // httplib::SSLClient cli("google.com");
    // httplib::SSLClient cli("www.youtube.com");
    cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(true);
#else
    httplib::Client cli("localhost", 8080);
#endif
    auto test = j.dump();
    //cli.Post("/replays", "{ \"player_id\": \"KDing\", \"player_name\" : \"KDing\", \"player_character\" : \"player_character\", \"opponent_character\" : \"opponent_character\", \"replay_count\" : 44, \"data\" : \"aGVsbG8gd29ybGQK\" }", "application/json");
                          //{"data":"1","opponent_character":"Makoto","player_character":"Bullet","player_id":"KDing","player_name":"KDing","replay_count":1}
    cli.Post("/replays", test.c_str(), "application/json");
}


json testHTTPReq() {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli("localhost", 8080);
    // httplib::SSLClient cli("google.com");
    // httplib::SSLClient cli("www.youtube.com");
    cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(true);
#else
    httplib::Client cli("localhost", 8080);
#endif

    if (auto res = cli.Get("/replays")) {
        cout << res->status << endl;
        cout << res->get_header_value("Content-Type") << endl;
        cout << res->body << endl;
        // Using (raw) string literals and json::parse
        // parse explicitly
        auto j3 = json::parse(res->body);
        // explicit conversion to string

        return j3;
        for (int it = 0; it != j3.size(); ++it) {
            std::string s2 = j3[it].at("player_name");
        }
    }

    else {
        return NULL;
        cout << "error code: " << res.error() << std::endl;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        auto result = cli.get_openssl_verify_result();
        if (result) {
            cout << "verify error: " << X509_verify_cert_error_string(result) << endl;
        }
#endif
    }
}