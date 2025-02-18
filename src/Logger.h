#ifndef LOGGER_H
#define LOGGER_H

#include "FileSystem.h"

/**
*	plain log output
*/
void LOG(const std::string& format, ...);
/**
*	output log level info
*/
void LOG_INFO(const std::string& format, ...);
/**
*	output log level error
*/
void LOG_ERROR(const std::string& format, ...);

/**
*	output progress bar
*/
void LOG_DOWNLOAD(const std::string& filename);

void LOG_PROGRESS(float done, float total);

#ifdef DEBUG
	#define DEBUG_LINE(fmt, ...) \
		printf( "%s:%d:%s(): " fmt "\n", __FILE__, \
									__LINE__, __FUNCTION__, __VA_ARGS__);
#else
	#define	DEBUG_LINE(fmt, ...)
#endif

#endif
