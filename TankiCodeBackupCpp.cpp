#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <thread>
#include <filesystem>
#include <syncstream>
#include <cpr/cpr.h>


using namespace std;
namespace fs = filesystem;


constexpr auto MAIN_JS_PATTERN =  R"(\/(browser-public|play)\/static\/js\/main\.[\w\d]{8}\.js)";


string baseUrlForServer(int s) {
    return s == 0 ? "https://tankionline.com"
                  : "https://public-deploy" + to_string(s) + ".test-eu.tankionline.com";
}

optional<string> fetchIndexHtml(int server, const string& baseUrl) {
    string url = baseUrl + (server == 0 ? "/play/index.html" : "/browser-public/index.html");

    auto resp = cpr::Get(cpr::Url{ url });

    if (resp.status_code == 200) {
        return resp.text;
    }
    return nullopt;
}

optional<string> extractMainJSUrl(const string& html) {
    static const regex pattern(MAIN_JS_PATTERN);
    smatch match;

    if (regex_search(html, match, pattern)) {
        return match[0].str();
    }
    return nullopt;
}

pair<string, string> buildLocalPath(int server, const string& mainJS) {
    string serverDir = to_string(server);

    fs::create_directories(serverDir);

    string fileName = fs::path(mainJS).filename().string();
    string localPath = "./" + serverDir + "/" + fileName;

    return { localPath, fileName };
}

void downloadMap(const string& url, const string& path, const string& fileName) {
    cpr::Response resp = cpr::Get(cpr::Url{ url });

    if (resp.status_code != 403 && resp.status_code != 404) {
        ofstream file(path, ios::binary);
        file.write(resp.text.data(), resp.text.size());
        osyncstream(cout) << "Has Map! (" << fileName << "): " << resp.status_code << "\n";
    }
    else {
        osyncstream(cout) << "No Map (" << fileName << "): " << resp.status_code << "\n";
    }
}

void downloadFile(const string& url, const string& path) {
    cpr::Response resp = cpr::Get(cpr::Url{ url });

    if (resp.status_code == 200) {
        ofstream file(path, ios::binary);
        file.write(resp.text.data(), resp.text.size());
        osyncstream(cout) << "Finished " << path << "\n";
    }
    else {
        osyncstream(cerr) << "Download failed: " << url << " (" << resp.status_code << ")\n";
    }
}

void downloadServerMainJS(int server) {
    string baseUrl = baseUrlForServer(server);

    auto indexHtmlOpt = fetchIndexHtml(server, baseUrl);
    if (!indexHtmlOpt) return;

    auto mainJsUrlOpt = extractMainJSUrl(*indexHtmlOpt);
    if (!mainJsUrlOpt) {
        osyncstream(cerr) << "JS not found in response from " << baseUrl << "\n";
        return;
    }

    auto [localPath, fileName] = buildLocalPath(server, *mainJsUrlOpt);
    string mapUrl = *mainJsUrlOpt + ".map";
    string mapPath = localPath + ".map";
    string mapFilename = fileName + ".map";

    if (fs::exists(localPath)) {
        osyncstream(cout) << baseUrl << ": " << fileName << "\nNo Changes\n";
    }
    else {
        osyncstream(cout) << baseUrl << ": " << fileName << "\nUpdate!\n";
        downloadFile(baseUrl + *mainJsUrlOpt, localPath);
    }

    if (!fs::exists(mapPath)) {
        downloadMap(baseUrl + mapUrl, mapPath, mapFilename);
    }
    else {
        osyncstream(cout) << "Map already exists locally.\n";
    }
}

int main() {
    {
        vector<jthread> threads;
        for (int i = 0; i <= 10; ++i) {
            // server 0 is alias for /play server
            threads.emplace_back(downloadServerMainJS, i);
        }
        // wait for threads to join (jthread joins automatically when going out of scope)
    }

    // then wait 5 seconds
    this_thread::sleep_for(chrono::seconds(5));
    return 0;
}
