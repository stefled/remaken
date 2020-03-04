#include "Cache.h"

#include <fstream>
using namespace std;
Cache::Cache(const CmdOptions & options)
{
    //fs::path rootPath = getenv(Constants::REMAKENPKGROOT); //?nullptr !!
    m_cacheFile = options.getRemakenRoot() / Constants::REMAKEN_CACHE_FILE;
}

Cache::~Cache()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    write(m_cachedUrls);
}

bool Cache::contains(string url)
{
    load();
    std::lock_guard<std::mutex> lock(m_mutex);
    if (find(m_cachedUrls.begin(),m_cachedUrls.end(),url) != m_cachedUrls.end()){
        return true;
    }
    return false;
}

void Cache::add(string url)
{
    load();
    std::lock_guard<std::mutex> lock(m_mutex);
    if (find(m_cachedUrls.begin(),m_cachedUrls.end(),url) == m_cachedUrls.end()){
        m_cachedUrls.push_back(url);
    }
}


void Cache::remove(string url){
    load();
    std::lock_guard<std::mutex> lock(m_mutex);
    if (find(m_cachedUrls.begin(),m_cachedUrls.end(),url) != m_cachedUrls.end()) {
        m_cachedUrls.remove(url);
    }
}

void Cache::write(list<string> data) {
    ofstream fos(m_cacheFile.c_str(),ios::out);
    for (auto && str : data) {
        fos<<str<< '\n';
    }
    fos.close();
}

void Cache::load() {
    if (!m_loaded) {
        if (fs::exists(m_cacheFile)) {
            ifstream fis(m_cacheFile.generic_string(),ios::in);
            while (!fis.eof()) {
                string curStr;
                getline(fis,curStr);
                m_cachedUrls.push_back(curStr);
            }
            fis.close();
            m_loaded = true;
        }
    }
}
