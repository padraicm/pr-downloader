#include <zlib.h>
#include <stdio.h>
#include <string.h>
#include <list>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>
#include "bencode/bencode.h"


#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#include <math.h>
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif
#endif

#include "md5.h"
#include "FileSystem.h"
#include "Util.h"
#include "Downloader/IDownloader.h"


CFileSystem* CFileSystem::singleton = NULL;

bool CFileSystem::fileIsValid(const FileData& mod, const std::string& filename) const
{
	MD5_CTX mdContext;
	int bytes;
	unsigned char data[1024];
	struct stat sb;
	if (stat(filename.c_str(),&sb)<0) {
		return false;
	}

	if (!sb.st_mode&S_IFREG) {
		LOG_ERROR("File is no file %s\n", filename.c_str());
		return false;
	}

	gzFile inFile = gzopen (filename.c_str(), "rb");
	if (inFile == NULL) { //file can't be opened
		return false;
	}
	MD5Init (&mdContext);
	unsigned long filesize=0;
	while ((bytes = gzread (inFile, data, 1024)) > 0) {
		MD5Update (&mdContext, data, bytes);
		filesize=filesize+bytes;
	}
	MD5Final (&mdContext);
	gzclose (inFile);
	/*	if (filesize!=mod->size){
			ERROR("File %s invalid, size wrong: %d but should be %d\n", filename.c_str(),filesize, mod->size);
			return false;
		}*/

	int i;
	for (i=0; i<16; i++) {
		if (mdContext.digest[i]!=mod.md5[i]) { //file is invalid
//			ERROR("Damaged file found: %s\n",filename.c_str());
//			unlink(filename.c_str());
			return false;
		}
	}
	return true;
}
bool CFileSystem::validateFile(IDownload& dl)
{
	if (dl.name.empty())
		return false;
	struct stat sb;
	if (stat(dl.name.c_str(),&sb)<0) {
		return false;
	}


	FILE* inFile = fopen(dl.name.c_str(), "rb");
	if (inFile == NULL) { //file can't be opened
		return false;
	}
	MD5_CTX mdContext;
	MD5Init (&mdContext);
	unsigned long filesize=0;
	int bytes;
	unsigned char data[1024];
	while ((bytes = fread (data, sizeof(data), 1,inFile)) > 0) {
		MD5Update (&mdContext, data, bytes);
		filesize=filesize+bytes;
	}
	MD5Final (&mdContext);
	fclose (inFile);
	for (int i=0; i<16; i++) {
		if (mdContext.digest[i]!=dl.md5[i]) { //file is invalid
			LOG_ERROR("md5 invalid!\n");
			return false;
		}
	}
	//TODO check sha1 sums
	return true;
}

std::string CFileSystem::createTempFile(){
	std::string tmp;
#ifndef WIN32
	tmp=tmpnam(NULL);
#else
	char buf[MAX_PATH];
	char tmppath[MAX_PATH];
	GetTempPath(sizeof(tmppath),tmppath);
	GetTempFileName(tmppath,NULL,0,buf);
	tmp->assign(buf);
#endif
	tmpfiles.push_back(tmp);
	return tmp;
}

bool CFileSystem::parseSdp(const std::string& filename, std::list<CFileSystem::FileData>& files)
{
	char c_name[255];
	unsigned char c_md5[16];
	unsigned char c_crc32[4];
	unsigned char c_size[4];

	gzFile in=gzopen(filename.c_str(), "r");
	if (in==Z_NULL) {
		LOG_ERROR("Could not open %s\n",filename.c_str());
		return NULL;
	}
	files.clear();
	FileData tmpfile;
	while (!gzeof(in)) {
		int length = gzgetc(in);
		if (length == -1) break;
		if (!((gzread(in, &c_name, length)) &&
		      (gzread(in, &c_md5, 16)) &&
		      (gzread(in, &c_crc32, 4)) &&
		      (gzread(in, &c_size, 4)))) {
			LOG_ERROR("Error reading %s\n", filename.c_str());
			gzclose(in);
			return false;
		}

		FileData f;
		f.name = std::string(c_name, length);
		std::memcpy(&f.md5, &c_md5, 16);
		f.crc32 = parse_int32(c_crc32);
		f.size = parse_int32(c_size);
		f.compsize = 0;
		files.push_back(f);
	}
	gzclose(in);
	return true;
}

CFileSystem::~CFileSystem()
{
	std::list<std::string>::iterator it;
	for (it = tmpfiles.begin(); it != tmpfiles.end(); ++it) {
		remove(it->c_str());
	}
	tmpfiles.clear();
}


CFileSystem::CFileSystem()
{
	tmpfiles.clear();
#ifndef WIN32
	char* buf;
	buf=getenv("HOME");
	springdir=buf;
	springdir.append("/.spring");
#else
	TCHAR pathMyDocs[MAX_PATH];
	SHGetFolderPath( NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, pathMyDocs);
	springdir=pathMyDocs;
	springdir.append("\\My Games\\Spring");
#endif
}

void CFileSystem::Initialize()
{
}

CFileSystem* CFileSystem::GetInstance()
{
	if (singleton==NULL)
		singleton=new CFileSystem();
	return singleton;
}

void CFileSystem::Shutdown()
{
	CFileSystem* tmpFileSystem=singleton;
	singleton=NULL;
	delete tmpFileSystem;
	tmpFileSystem=NULL;
}

const std::string& CFileSystem::getSpringDir() const
{
	return springdir;
}

bool CFileSystem::directoryExists(const std::string& path) const
{
	struct stat fileinfo;
	int res=stat(path.c_str(),&fileinfo);
	return (res==0);
}

void CFileSystem::createSubdirs (const std::string& path) const
{
	std::string tmp;
	tmp=path;
	if (path[path.length()]!=PATH_DELIMITER) {
		tmp=tmp.substr(0,tmp.rfind(PATH_DELIMITER));
	}
	for (unsigned int i=0; i<tmp.size(); i++) {
		char c=tmp.at(i);
		if (c==PATH_DELIMITER) {
#ifdef WIN32
			mkdir(tmp.substr(0,i).c_str());
#else
			mkdir(tmp.substr(0,i).c_str(),0777);
#endif
		}
	}
#ifdef WIN32
	mkdir(tmp.c_str());
#else
	mkdir(tmp.c_str(),0777);
#endif
}


const std::string CFileSystem::getPoolFileName(const CFileSystem::FileData& fdata) const
{
	std::string name;
	std::string md5;

	md5ItoA(fdata.md5, md5);
	name=getSpringDir();
	name += PATH_DELIMITER;
	name += "pool";
	name += PATH_DELIMITER;
	name += md5.at(0);
	name += md5.at(1);
	name += PATH_DELIMITER;
	createSubdirs(name);
	name += md5.substr(2);
	name += ".gz";
	return name;
}

int CFileSystem::validatePool(const std::string& path)
{
	DIR* d;
	d=opendir(path.c_str());
	unsigned long time=getTime();
	int res=0;
	if (d!=NULL) {
		struct dirent* dentry;
		while ( (dentry=readdir(d))!=NULL) {
			struct stat sb;
			std::string tmp;
			if (dentry->d_name[0]!='.') {
				tmp=path+PATH_DELIMITER+dentry->d_name;
				stat(tmp.c_str(),&sb);
				if ((sb.st_mode&S_IFDIR)!=0) {
					res=res+validatePool(tmp);
					unsigned long now=getTime();
					if (time<now) {
						LOG_INFO("Valid files: %d\r",res);
						fflush(stdout);
						time=now;
					}
				} else {
					FileData filedata;
					std::string md5;
					int len=tmp.length();
					if (len<36) { //file length has at least to be <md5[0]><md5[1]>/<md5[2-30]>.gz
						LOG_ERROR("Invalid file: %s\n", tmp.c_str());
					} else {
						md5="";
						md5.push_back(tmp.at(len-36));
						md5.push_back(tmp.at(len-35));
						md5.append(tmp.substr(len-33, 30));
						md5AtoI(md5,filedata.md5);
						if (!fileIsValid(filedata,tmp)) {
							LOG_ERROR("Invalid File in pool: %s\n",tmp.c_str());
						} else {
							res++;
						}
					}
				}
			}

		}
	}
	closedir(d);
	return res;
}

bool CFileSystem::isOlder(const std::string& filename, int secs)
{
	struct stat sb;
	if (stat(filename.c_str(),&sb)<0) {
		return true;
	}
	time_t t;
#ifdef WIN32
	LARGE_INTEGER date;

	SYSTEMTIME pTime;
	FILETIME pFTime;
	GetSystemTime(&pTime);
	SystemTimeToFileTime(&pTime, &pFTime);

	date.HighPart = pFTime.dwHighDateTime;
	date.LowPart = pFTime.dwLowDateTime;

	date.QuadPart -= 11644473600000 * 10000;

	t = date.QuadPart / 10000000;
#else
	time(&t);
#endif
	return (t<sb.st_ctime+secs);
}

bool CFileSystem::fileExists(const std::string& filename)
{
	FILE* fp = NULL;
	fp = fopen(filename.c_str(), "r");
	if (fp == NULL) {
		return false;
	}
	fclose(fp);
	return true;
}

bool CFileSystem::parseTorrent(const char* data, int size, IDownload& dl)
{
	struct be_node* node=be_decoden(data, size);
#ifdef DEBUG
	be_dump(node);
#endif
	if (node->type!=BE_DICT) {
		LOG_ERROR("Error in torrent data\n");
		be_free(node);
		return false;
	}
	int i;
	struct be_node* infonode=NULL;
	for (i = 0; node->val.d[i].val; ++i) { //search for a dict with name info
		if ((node->type==BE_DICT) && (strcmp(node->val.d[i].key,"info")==0)) {
			infonode=node->val.d[i].val;
			break;
		}
	}
	if (infonode==NULL) {
		LOG_ERROR("couldn't find info node in be dict\n");
		be_free(node);
		return false;
	}
	for (i = 0; infonode->val.d[i].val; ++i) { //fetch needed data from dict and fill into dl
		struct be_node*datanode;
		datanode=infonode->val.d[i].val;
		switch(datanode->type) {
		case BE_STR: //current value is a string
//			if ((strcmp("name",infonode->val.d[i].key)==0) && (dl.name.empty())) { //set filename if not already set
//					dl.name=datanode->val.s;
//			} else
			if (!strcmp("pieces", infonode->val.d[i].key)) { //hash sum of a piece
				const int count = strlen(datanode->val.s)/6;
				for (int i=0; i<count; i++) {
					struct IDownload::piece piece;
					for(int j=0; j<5; j++) {
						piece.sha[j]=datanode->val.s[i*5+j];
						piece.state=IDownload::STATE_NONE;
					}
					dl.pieces.push_back(piece);
				}
			}
			break;
		case BE_INT: //current value is a int
			if (strcmp("length",infonode->val.d[i].key)==0) { //filesize
				dl.size=datanode->val.i;
			} else if (!strcmp("piece length",infonode->val.d[i].key)) { //length of a piece
				dl.piecesize=datanode->val.i;
			}
			break;
		default:
			break;
		}
	}
	DEBUG_LINE("Parsed torrent data: %s %d\n", dl.name.c_str(), dl.piecesize);
	be_free(node);
	return true;
}

bool CFileSystem::dumpSDP(const std::string& filename)
{
	std::list<CFileSystem::FileData> files;
	files.clear();
	if (!parseSdp(filename, files))
		return false;
	std::list<CFileSystem::FileData>::iterator it;
	LOG_INFO("filename	size	virtualname	crc32\n");
	for(it=files.begin(); it!=files.end(); ++it) {
		LOG_INFO("%s %8d %s %X\n",getPoolFileName(*it).c_str(), (*it).size, (*it).name.c_str(), (*it).crc32);
	}
	return true;
}

