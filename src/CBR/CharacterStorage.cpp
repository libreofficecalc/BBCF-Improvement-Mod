#define CPPHTTPLIB_OPENSSL_SUPPORT
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
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <future>
#include <boost/beast/core/detail/base64.hpp>


using namespace std;
using json = nlohmann::json;

bool updateLocalWindowVar = true;
bool updateServerWindowVar = true;

std::string hostAddress = "cbr.rndsh.it";
int hostPort = 8443;
//std::string hostAddress = "localhost";
//int hostPort = 8080;
bool certVer = false;

#define CA_CERT_FILE "./ca-bundle.crt"


std::string encodeB64(const std::string& val) {
    std::string ret;
    ret.resize(boost::beast::detail::base64::encoded_size(val.size()));

    std::size_t read = boost::beast::detail::base64::encode(&ret.front(), val.data(), val.size());

    ret.resize(read);
    return ret;
}

std::string decodeB64(const std::string& val) {
    std::string ret;
    ret.resize(boost::beast::detail::base64::decoded_size(val.size()));

    std::size_t read = boost::beast::detail::base64::decode(&ret.front(), val.c_str(), val.size()).first;

    ret.resize(read);
    return ret;
}


std::string decode64(std::string const& val)
{
    using namespace boost::archive::iterators;
    return {
        transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>{
            std::begin(val)},
        {std::end(val)},
    };
}

std::string encode64(std::string const& val)
{
    using namespace boost::archive::iterators;
    std::string r{
        base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>{
            std::begin(val)},
        {std::end(val)},
    };
    return r.append((3 - val.size() % 3) % 3, '=');
}


static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";


static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';

    }

    return ret;

}

std::string base64_decode(std::string const& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
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
    auto en64 = encodeB64(s);
    //auto dec = decode64(en64);
    //if (s == dec) {
    //    return en64;
    //}
    //if (s == s) {
    //    return en64;
    //}
    return en64;
}

CbrData ConvertCbrDataFromBase64(std::string encodeS) {
    auto decodeS = decodeB64(encodeS);

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

std::string getFullCharName(std::string charName) {
    if (charName == "es") { return "ES(es)"; };//es
    if (charName == "ny") { return "Nu(ny)"; };//nu
    if (charName == "ma") { return "Mai(ma)"; };// mai
    if (charName == "tb") { return "Tsubaki(tb)"; };// tsubaki
    if (charName == "rg") { return "Ragna(rg)"; };//ragna
    if (charName == "hz") { return "Hazama(hz)"; };//hazama
    if (charName == "jn") { return "Jin(jn)"; };//jin
    if (charName == "mu") { return "Mu(mu)"; };//mu
    if (charName == "no") { return "Noel(no)"; };//noel
    if (charName == "mk") { return "Makoto(mk)"; };//makoto
    if (charName == "rc") { return "Rachel(rc)"; };//rachel
    if (charName == "vh") { return "Valkenhain(vh)"; };//valk
    if (charName == "tk") { return "Taokaka(tk)"; };//taokaka
    if (charName == "pt") { return "Platinum(pt)"; };//platinum
    if (charName == "tg") { return "Tager(tg)"; };//tager
    if (charName == "rl") { return "Relius(rl)"; };//relius
    if (charName == "lc") { return "Litchi(lc)"; };//litchi
    if (charName == "iz") { return "Izayoi(iz)"; };//izayoi
    if (charName == "ar") { return "Arakune(ar)"; };//arakune
    if (charName == "am") { return "Amane(am)"; };//amane
    if (charName == "bn") { return "Bang(bn)"; };//bang
    if (charName == "bl") { return "Bullet(bl)"; };//bullet
    if (charName == "ca") { return "Carl(ca)"; };//carl
    if (charName == "az") { return "Azrael(az)"; };//azrael
    if (charName == "ha") { return "Hakumen(ha)"; };//hakumen
    if (charName == "kg") { return "Kagura(kg)"; };//kagura
    if (charName == "kk") { return "Kokonoe(kk)"; };//koko
    if (charName == "rm") { return "Lambda(rm)"; };//lambda
    if (charName == "hb") { return "Hibiki(hb)"; };//hibiki
    if (charName == "tm") { return "Terumi(tm)"; };//terumi
    if (charName == "ph") { return "Nine(ph)"; };//nine
    if (charName == "ce") { return "Celica(ce)"; };//Celica
    if (charName == "nt") { return "Naoto(nt)"; };//naoto
    if (charName == "mi") { return "Izanami(mi)"; };//izanami
    if (charName == "su") { return "Susan(su)"; };//susan
    if (charName == "jb") { return "Jubei(jb)"; };//jubei
    return "Something went wrong";
}

void CbrHTTPPost(json& j, boost::promise<bool>& p) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli(hostAddress);
    // httplib::SSLClient cli("google.com");
    // httplib::SSLClient cli("www.youtube.com");
    cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(certVer);
#else
    httplib::Client cli(hostAddress, hostPort);
#endif
    auto test = j.dump();
    //cli.Post("/replays", "{ \"player_id\": \"KDing\", \"player_name\" : \"KDing\", \"player_character\" : \"player_character\", \"opponent_character\" : \"opponent_character\", \"replay_count\" : 44, \"data\" : \"aGVsbG8gd29ybGQK\" }", "application/json");
                          //{"data":"1","opponent_character":"Makoto","player_character":"Bullet","player_id":"KDing","player_name":"KDing","replay_count":1}
    cli.Post("/replays", test.c_str(), "application/json");

    p.set_value(true);
    return;
}

void CbrHTTPPostNoThreat(json& j) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli(hostAddress);
    cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(certVer);
#else
    httplib::Client cli(hostAddress, hostPort);
#endif
    auto test = j.dump();
    cli.Post("/replays", test.c_str(), "application/json");
    return;
}



json cbrHTTP_GetTable(boost::promise<json>& j) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli(hostAddress);
    // httplib::SSLClient cli("google.com");
    // httplib::SSLClient cli("www.youtube.com");
    cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(certVer);
#else
    httplib::Client cli(hostAddress, hostPort);
#endif

    if (auto res = cli.Get("/replays")) {
        if (res->status < 200 || res->status >= 300) {
            j.set_value(NULL);
            return NULL;
        }
        cout << res->status << endl;
        cout << res->get_header_value("Content-Type") << endl;
        cout << res->body << endl;
        // Using (raw) string literals and json::parse
        // parse explicitly
        auto j3 = json::parse(res->body);
        for (int it = 0; it != j3.size(); ++it) {
            std::string s2 = j3[it].at("player_character");
            j3[it]["player_character"] = getFullCharName(s2);
        }
        // explicit conversion to string
        j.set_value(j3);
        return j3;

    }

    else {

        cout << "error code: " << res.error() << std::endl;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        auto result = cli.get_openssl_verify_result();
        if (result) {
            cout << "verify error: " << X509_verify_cert_error_string(result) << endl;
        }
#endif
    }
    j.set_value(NULL);
    return NULL;
}

json cbrHTTP_GetCharData(std::string public_id, boost::promise<json>& j) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli(hostAddress);
    // httplib::SSLClient cli("google.com");
    // httplib::SSLClient cli("www.youtube.com");
    cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(certVer);
#else
    httplib::Client cli(hostAddress, hostPort);
#endif

    if (auto res = cli.Get(std::string("/replays/") + public_id)) {
        if (res->status < 200 || res->status >= 300) {
            j.set_value(NULL);
            return NULL;
        }
        cout << res->status << endl;
        cout << res->get_header_value("Content-Type") << endl;
        cout << res->body << endl;
        // Using (raw) string literals and json::parse
        // parse explicitly
        auto j3 = json::parse(res->body);
        // explicit conversion to string
        j.set_value(j3);
        return j3;
    }

    else {
        j.set_value(NULL);
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

json cbrHTTP_GetFilteredTable(std::string PlayerID, std::string PlayerName, std::string PlayerCharacter, std::string OpponentCharacter, std::string ReplayCount, boost::promise<json>& j) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli(hostAddress);
    // httplib::SSLClient cli("google.com");
    // httplib::SSLClient cli("www.youtube.com");
    cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(certVer);
#else
    httplib::Client cli(hostAddress, hostPort);
#endif
    auto playerIDStr = "?PlayerID=" + PlayerID;
    auto playerNameStr = "&PlayerName=" + PlayerName;
    auto playerCharacterStr = "&PlayerCharacter=" + PlayerCharacter;
    auto opponentCharacterStr = "&OpponentCharacter=" + OpponentCharacter;
    auto replayCountStr = "&ReplayCount=" + ReplayCount;
    std::string request = std::string("/replays/filter") + playerIDStr + playerNameStr + playerCharacterStr + opponentCharacterStr + replayCountStr;
    if (auto res = cli.Get(request)) {
        if (res->status < 200 || res->status >= 300) {
            j.set_value(NULL);
            return NULL;
        }
        cout << res->status << endl;
        cout << res->get_header_value("Content-Type") << endl;
        cout << res->body << endl;
        // Using (raw) string literals and json::parse
        // parse explicitly
        auto j3 = json::parse(res->body);
        for (int it = 0; it != j3.size(); ++it) {
            std::string s2 = j3[it].at("player_character");
            j3[it]["player_character"] = getFullCharName(s2);
        }
        // explicit conversion to string
        j.set_value(j3);
        return j3;
    }

    else {

        cout << "error code: " << res.error() << std::endl;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        auto result = cli.get_openssl_verify_result();
        if (result) {
            cout << "verify error: " << X509_verify_cert_error_string(result) << endl;
        }
#endif
    }

    j.set_value(NULL);
    return NULL;
}


bool cbrHTTP_DeleteCharData(std::string public_id, boost::promise<bool>& j) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli(hostAddress);
    // httplib::SSLClient cli("google.com");
    // httplib::SSLClient cli("www.youtube.com");
    cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(certVer);
#else
    httplib::Client cli(hostAddress, hostPort);
#endif

    if (auto res = cli.Delete(std::string("/replays/") + public_id)) {
        if (res->status < 200 || res->status >= 300) {
            j.set_value(false);
            return false;
        }
        cout << res->status << endl;
        cout << res->get_header_value("Content-Type") << endl;
        cout << res->body << endl;
        // Using (raw) string literals and json::parse
        // parse explicitly
        // explicit conversion to string

        j.set_value(true);
        return true;
    }

    else {
        j.set_value(false);
        return false;
        cout << "error code: " << res.error() << std::endl;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        auto result = cli.get_openssl_verify_result();
        if (result) {
            cout << "verify error: " << X509_verify_cert_error_string(result) << endl;
        }
#endif
    }
}

json LoadCbrMetadata(std::string PlayerName, std::string PlayerCharacter, std::string OpponentCharacter, std::string ReplayCount,boost::promise<json>& j) {
    boost::filesystem::path dir("CBRsave");
    if (!(boost::filesystem::exists(dir))) {
        boost::filesystem::create_directory(dir);
    }
    using namespace boost::filesystem;
    struct recursive_directory_range
    {
        typedef recursive_directory_iterator iterator;
        recursive_directory_range(path p) : p_(p) {}

        iterator begin() { return recursive_directory_iterator(p_); }
        iterator end() { return recursive_directory_iterator(); }

        path p_;
    };

    //boost::filesystem::path dir("CBRsave");
    json container;
    for (const auto& dirEntry : recursive_directory_iterator("CBRsave")) {
        auto& t = dirEntry;
        std::string s = dirEntry.path().string();
        auto found = std::string::npos != s.find(".cbr");
        if (found) {
            auto j = readCbrMetadata(s);
            if (j != NULL) {
                if (PlayerName != "" && PlayerName != j.at("player_name")) { continue; }
                if (PlayerCharacter != "" && PlayerCharacter != j.at("player_character")) { continue; }
                //if (OpponentCharacter != "" && OpponentCharacter != j.at("opponent_character")) { continue; }
                if (OpponentCharacter != "") { 
                    std::string opp_str = j["opponent_character"];
                    if (std::string::npos == opp_str.find(OpponentCharacter)) {
                        continue;
                    } 
                }
                int replayCountFound = j.at("replay_count");
                if (ReplayCount != "" && atoi(ReplayCount.c_str()) > replayCountFound) { continue; }
                int replayCount = j.at("replay_count");
                container.push_back(j);
            }
        }
    }
    for (int it = 0; it != container.size(); ++it) {
        std::string s2 = container[it].at("player_character");
        container[it]["player_character"] = getFullCharName(s2);
    }
    j.set_value(container);
    return container;
}




json readCbrMetadata(std::string filename) {
    std::ifstream infile(filename, std::ios_base::binary);
    CbrData insert;
    insert.setPlayerName("-1");
    if (infile.fail()) {
        //File does not exist code here
    }
    else {
        //std::string b = "";
        //infile >> b;
        std::string charName = "";
        std::string playerName = "";
        std::string opponentChar = "";
        int rCount = 0;
        try
        {
            boost::iostreams::filtering_stream<boost::iostreams::input> f;
            f.push(boost::iostreams::gzip_decompressor());
            f.push(infile);
            boost::archive::binary_iarchive archive(f);
            CbrData insert;
            
            try
            {
                archive >> charName;
            }
            catch (std::exception& ex) {

            }
            catch (boost::archive::archive_exception& ex) {

            }
            try
            {
                archive >> playerName;
            }
            catch (std::exception& ex) {

            }
            catch (boost::archive::archive_exception& ex) {

            }
            try
            {
                archive >> opponentChar;
            }
            catch (std::exception& ex) {

            }
            catch (boost::archive::archive_exception& ex) {

            }
            try
            {
                archive >> rCount;
            }
            catch (std::exception& ex) {

            }
            catch (boost::archive::archive_exception& ex) {

            }
            try
            {
                archive >> insert;
            }
            catch (std::exception& ex) {

            }
            catch (boost::archive::archive_exception& ex) {

            }
        }
        catch (std::exception& ex) {

        }
        catch (boost::archive::archive_exception& ex) {

        }
        
        if (charName == "") {
            return NULL;
        }
        

        json j2 = {
          {"player_id", "local"},
          {"player_name", playerName},
          {"player_character", charName},
          {"opponent_character", opponentChar},
          {"replay_count", rCount},
          {"path", filename},
        };

        return j2;
    }
    return NULL;
}

//---- the thread code is an abomination cause i never worked with threads in c++ before, should probably clean it up, which definetly will happen because i love cleaning my code up x.x
bool serverThreadActive = false;
boost::thread characterStorageServerThread;
boost::promise<json> p;
boost::unique_future<json> f = p.get_future();
json threadDownloadData(bool run) {

    //saveReplayDataInMenu();

    if (run && serverThreadActive == false) {
        serverThreadActive = true;
        characterStorageServerThread = boost::thread(&cbrHTTP_GetTable, std::ref(p));
    }
    auto test = f.is_ready();
    if (f.is_ready() && serverThreadActive == true && characterStorageServerThread.joinable()) {
        
        characterStorageServerThread.join();
        serverThreadActive = false;
        auto retVal = f.get();
        p = boost::promise<json>();
        f = p.get_future();
        return retVal;
    }
    return NULL;
}


json threadDownloadFilteredData(bool run, std::string PlayerID, std::string PlayerName, std::string PlayerCharacter, std::string OpponentCharacter, std::string ReplayCount) {

    //saveReplayDataInMenu();

    if (run && serverThreadActive == false) {
        serverThreadActive = true;
        characterStorageServerThread = boost::thread(&cbrHTTP_GetFilteredTable, PlayerID, PlayerName, PlayerCharacter, OpponentCharacter, ReplayCount, std::ref(p));
    }
    auto test = f.is_ready();
    if (f.is_ready() && serverThreadActive == true && characterStorageServerThread.joinable()) {

        characterStorageServerThread.join();
        serverThreadActive = false;
        auto retVal = f.get();
        p = boost::promise<json>();
        f = p.get_future();
        return retVal;
    }
    return NULL;
}

bool localDataUpdating = false;
bool serverThreadActivityActive = false;
boost::thread characterStorageDownloadThread;
boost::promise<json> pDownload;
boost::unique_future<json> fDownload = pDownload.get_future();
json threadDownloadCharData(bool run, std::string public_id) {

    if (run && serverThreadActivityActive == false) {
        serverThreadActivityActive = true;
        characterStorageDownloadThread = boost::thread(&cbrHTTP_GetCharData, public_id, std::ref(pDownload));
        localDataUpdating = true;
    }
    if (fDownload.is_ready() && serverThreadActivityActive == true && characterStorageDownloadThread.joinable()) {

        characterStorageDownloadThread.join();
        serverThreadActivityActive = false;
        auto retVal = fDownload.get();
        pDownload = boost::promise<json>();
        fDownload = pDownload.get_future();
        localDataUpdating = false;
        updateLocalWindowVar = true;

        return retVal;
    }
    return NULL;
}


boost::thread characterStorageDeleteThread;
boost::promise<bool> pDelete;
boost::unique_future<bool> fDelete = pDelete.get_future();
bool threadDeleteCharData(bool run, std::string public_id) {

    if (run && serverThreadActivityActive == false) {
        serverThreadActivityActive = true;
        characterStorageDownloadThread = boost::thread(&cbrHTTP_DeleteCharData, public_id, std::ref(pDelete));
    }
    if (fDelete.is_ready() && serverThreadActivityActive == true && characterStorageDownloadThread.joinable()) {

        characterStorageDownloadThread.join();
        serverThreadActivityActive = false;
        auto retVal = fDelete.get();
        pDelete = boost::promise<bool>();
        fDelete = pDelete.get_future();
        return retVal;
    }
    return false;
}

boost::thread localMetadataThread;
boost::promise<json> pLocal;
boost::unique_future<json> fLocal = pLocal.get_future();
json threadGetLocalMetadata(bool run, std::string PlayerName, std::string PlayerCharacter, std::string OpponentCharacter, std::string ReplayCount) {
    if (run && serverThreadActivityActive == false) {
        localDataUpdating = true;
        serverThreadActivityActive = true;
        localMetadataThread = boost::thread(&LoadCbrMetadata, PlayerName, PlayerCharacter, OpponentCharacter, ReplayCount, std::ref(pLocal));
    }
    if (fLocal.is_ready() && serverThreadActivityActive == true && localMetadataThread.joinable()) {
        localDataUpdating = false;
        localMetadataThread.join();
        serverThreadActivityActive = false;
        auto retVal = fLocal.get();
        pLocal = boost::promise<json>();
        fLocal = pLocal.get_future();
        return retVal;
    }
    return NULL;
}

boost::thread localUploadThread;
boost::promise<bool> pLocalUpload;
boost::unique_future<bool> fLocalUpload = pLocalUpload.get_future();
void threadUploadCharData(json& j) {
    if (serverThreadActive == false) {
        serverThreadActive = true;
        updateServerWindowVar = true;
        localUploadThread = boost::thread(&CbrHTTPPost, j, std::ref(pLocalUpload));
    }
    if (fLocalUpload.is_ready() && serverThreadActive == true && localUploadThread.joinable()) {

        localUploadThread.join();
        serverThreadActive = false;
        auto retVal = fLocalUpload.get();
        pLocalUpload = boost::promise<bool>();
        fLocalUpload = pLocalUpload.get_future();

    }
}


bool isCbrServerBusy() {
    return serverThreadActive;
}

bool isCbrServerActivityHappening() {
    return serverThreadActivityActive;
}

bool isCbrLocalDataUpdating() {
    return localDataUpdating;
}

bool updateServerWindow(bool reset) {
    
    if (serverThreadActive == false) {
        auto ret = updateServerWindowVar;
        if (reset) {
            updateServerWindowVar = false;
        }
        return ret;
    }
    return false;

}

bool updateLocalWindow(bool reset) {

    auto ret = updateLocalWindowVar;
    if (reset) {
        updateLocalWindowVar = false;
    }
    return ret;

}
bool setUpdateServerWindow(bool reset) {

    return updateServerWindowVar = reset;

}
bool setUpdateLocalWindow(bool reset) {

    return updateLocalWindowVar = reset;

}


void threadReturnCheck() {
    if (fLocalUpload.is_ready() && serverThreadActive == true && localUploadThread.joinable()) {

        localUploadThread.join();
        serverThreadActive = false;
        auto retVal = fLocalUpload.get();
        pLocalUpload = boost::promise<bool>();
        fLocalUpload = pLocalUpload.get_future();
    }

    if (fDelete.is_ready() && serverThreadActivityActive == true && characterStorageDownloadThread.joinable()) {

        characterStorageDownloadThread.join();
        serverThreadActivityActive = false;
        auto retVal = fDelete.get();
        pDelete = boost::promise<bool>();
        fDelete = pDelete.get_future();
    }

}

json convertCBRtoJson(CbrData& cbr, std::string uploaderID) {

    auto base64 = ConvertCbrDataToBase64(cbr);

    auto opponentChar = getOpponentCharNameString(cbr);

    //auto cbrData = ConvertCbrDataFromBase64(base64);
    json j2 = {
      {"player_id", uploaderID},
      {"player_name", cbr.getPlayerName()},
      {"player_character", cbr.getCharName()},
      {"opponent_character", opponentChar},
      {"replay_count", cbr.getReplayCount()},
      {"data", base64},
    };
    return j2;
}


