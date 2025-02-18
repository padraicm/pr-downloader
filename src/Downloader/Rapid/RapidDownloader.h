#ifndef RAPID_DOWNLOADER_H
#define RAPID_DOWNLOADER_H


#include <string>
#include <list>
#include <stdio.h>
//FIXME: read from config / use this as default
#define REPO_MASTER "http://repos.caspring.org/repos.gz"
#define REPO_MASTER_RECHECK_TIME 3600 //how long to cache the repo-master file in secs without rechecking
#define REPO_RECHECK_TIME 600

#include "../IDownloader.h"
#include "RepoMaster.h"

class CSdp;
class CHttpDownload;
class CFileSystem;

class CRapidDownloader: public IDownloader
{
public:
	CRapidDownloader(const std::string& masterurl=REPO_MASTER);
	~CRapidDownloader();

	/**
		download a tag, for exampe ba:stable
	*/
	bool download_tag(const std::string& modname);
	/**
		lists all tags on all servers
	*/
	void list_tag();
	/**
		remove a dsp from the list of remote dsps
	*/
	void addRemoteDsp(CSdp& dsp);
	/**
		search for a mod, searches for the short + long name
	*/
	virtual bool search(std::list<IDownload>& result, const std::string& name, IDownload::category=IDownload::CAT_NONE);
	/**
		start a download
	*/
	virtual bool download(IDownload& download);

private:
	/**
		download by name, for example "Complete Annihilation revision 1234"
	*/
	bool download_name(const std::string& longname, int reccounter=0);
	/**
		update all repos from the web
	*/
	bool reloadRepos();
	bool reposLoaded;
	/**
		helper function for sort
	*/
	static bool list_compare(CSdp& first ,CSdp& second);
	CRepoMaster* repoMaster;
	std::list<CSdp> sdps;
};

#endif
