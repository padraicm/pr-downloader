#ifndef HTTP_DOWNLOAD_H
#define HTTP_DOWNLOAD_H

#include <string>
#include <list>
#include <curl/curl.h>
#include "../../FileSystem.h"
#include "../IDownloader.h"


class CHttpDownloader: public IDownloader
{

public:
	CHttpDownloader();
	~CHttpDownloader();

	/**
		downloads a file from Url to filename
	*/
	bool download(const std::string& Url, const std::string& filename, int pos=1);
	void setCount(unsigned int count);
	void setStatsPos(unsigned int pos);
	unsigned int getStatsPos();
	unsigned int getCount();
	const std::string& getCacheFile(const std::string &url);
	void downloadStream(const std::string& url,std::list<CFileSystem::FileData*>& files);
	virtual bool search(std::list<IDownload>& result, const std::string& name, IDownload::category=IDownload::CAT_NONE);
	virtual bool download(IDownload& download);
private:
	CURL* curl;
	bool parallelDownload(IDownload& download);
	unsigned int stats_count;
	unsigned int stats_filepos;
	std::list<IDownload>* realSearch(const std::string& name, IDownload::category cat);
	std::string escapeUrl(const std::string& url);
};

#endif
