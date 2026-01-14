#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <thread>
#include <filesystem>
#include <syncstream>
#include <barrier>
#include <semaphore>
#include <cpr/cpr.h>

#define sout std::osyncstream(std::cout)
#define serr std::osyncstream(std::cerr)

using namespace std;
namespace fs = filesystem;

constexpr auto MAIN_JS_PATTERN =  R"(\/(browser-public|play)\/static\/js\/main\.[\w\d]{8}\.js)";
constexpr int MAX_PARALLEL_DOWNLOADS = 2;


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

// currently unused (TODO: implement downloadStatusJs)
optional<string> fetchBalancer(int server, const string& baseUrl) {
    string url = baseUrl + (server == 0 ? "/s/status.js" : "/balancer");

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

counting_semaphore<MAX_PARALLEL_DOWNLOADS> semaphore1{ MAX_PARALLEL_DOWNLOADS };

void downloadMap(const string& url, const string& path, const string& fileName) {
    semaphore1.acquire();
    cpr::Response resp = cpr::Get(cpr::Url{ url });
    semaphore1.release();

    if (resp.status_code != 403 && resp.status_code != 404) {
        ofstream file(path, ios::binary);
        file.write(resp.text.data(), resp.text.size());
        sout << "MAP FOUND!!! (" << fileName << "): " << resp.status_code << "\n";
    }
    else {
        sout << "No Map (" << fileName << "): " << resp.status_code << "\n";
    }
}

void downloadFile(const string& url, const string& path) {
    semaphore1.acquire();
    cpr::Response resp = cpr::Get(cpr::Url{ url });
    semaphore1.release();

    if (resp.status_code == 200) {
        ofstream file(path, ios::binary);
        file.write(resp.text.data(), resp.text.size());
        sout << "Finished " << path << "\n";
    }
    else {
        serr << "Download failed: " << url << " (" << resp.status_code << ")\n";
    }
}


int phase = 1;
barrier barrier1(11, []() noexcept {
    switch (phase) {
    case 1: cout << "\nDownloading missing js files...\n"; break;
    case 2: cout << "\nTrying to download maps...\n"; break;
    }
    phase++;
});


void downloadServerMainJS(int server) {
    string baseUrl = baseUrlForServer(server);

    auto indexHtmlOpt = fetchIndexHtml(server, baseUrl);
    if (!indexHtmlOpt) return barrier1.arrive_and_drop();;

    auto mainJsUrlOpt = extractMainJSUrl(*indexHtmlOpt);
    if (!mainJsUrlOpt) {
        serr << "JS not found in response from " << baseUrl << "\n";
        return barrier1.arrive_and_drop();;
    }

    auto [localPath, fileName] = buildLocalPath(server, *mainJsUrlOpt);
    string mapUrl = *mainJsUrlOpt + ".map";
    string mapPath = localPath + ".map";
    string mapFilename = fileName + ".map";

    if (fs::exists(localPath)) {
        sout << baseUrl << ": " << fileName << "\nNo Changes\n";
        barrier1.arrive_and_wait();
    }
    else {
        sout << baseUrl << ": " << fileName << "\nUpdate!\n";
        barrier1.arrive_and_wait();
        downloadFile(baseUrl + *mainJsUrlOpt, localPath);
    }

    barrier1.arrive_and_wait();

    if (!fs::exists(mapPath)) {
        downloadMap(baseUrl + mapUrl, mapPath, mapFilename);
    }
    else {
        sout << "Map already exists locally.\n";
    }
}

// TODO: implement downloadStatusJs
void downloadStatusJs() {
    int server = 0;
    string baseUrl = baseUrlForServer(server);

    auto balancerOpt = fetchBalancer(server, baseUrl);
    if (!balancerOpt) {
        serr << "Balancer not found from " << baseUrl << "\n";
        return;
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

    sout << "\nDone.\n";

    // then wait 5 seconds
    this_thread::sleep_for(chrono::seconds(10));
    return 0;
}
