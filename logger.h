/**
 * @file
 *  This file is part of UTILS
 *
 *  For conditions of distribution and use, please see the copyright
 *  notice in the file 'COPYING' at the root directory of this package
 *  and the copyright notice at https://github.com/TUM-I5/utils
 *
 * @copyright 2013 Technische Universitaet Muenchen
 * @author Sebastian Rettenberger <rettenbs@in.tum.de>
 */

#ifndef UTILS_LOGGER_H
#define UTILS_LOGGER_H

#include "utils/timeutils.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <vector>

#ifndef LOG_LEVEL
#ifdef NDEBUG
#define LOG_LEVEL 2
#else // NDEBUG
#define LOG_LEVEL 3
#endif // NDEBUG
#endif // LOG_LEVEL

#ifndef LOG_PREFIX
#define LOG_PREFIX "%a %b %d %X"
#endif // DEBUG_PRFIX

#ifndef LOG_ABORT
#ifdef MPI_VERSION
#define LOG_ABORT MPI_Abort(MPI_COMM_WORLD, -1)
#else // MPI_VERSION
#define LOG_ABORT abort()
#endif // MPI_VERSION
#endif // LOG_ABORT

/**
 * A collection of useful utility functions
 */
namespace utils
{

/**
 * Handles debugging/logging output
 * 
 * Most of the code is taken form QDebug form the Qt Framework
 */
class Logger
{
public:
	/** Message type */
	enum DebugType {
		/** A debug messages */
		DEBUG,
		/** A info message (printed to stdout) */
		INFO,
		/** A warning message */
		WARNING,
		/** A fatal error */
		ERROR
	};
private:
	/** Contains all information for a debug message */
	struct Stream {
		/** The debug type */
		DebugType type;
		/** MPI Rank, set to 0 to print message */
		int rank;
		/** References */
		int ref;
		/** Buffer for the output */
		std::stringstream buffer;
		/** Print additional space */
		bool space;
		
		/**
		 * Set defaults for a debug message
		 */
		Stream(DebugType t, int r)
			: type(t), rank(r), ref(1),
			buffer(std::stringstream::out),
			space(true) { }
	} *stream;
	/**
	 * Pointer to all information about the message
	 */
public:
	/**
	 * Start a new Debug message
	 * 
	 * @param t Type of the message
	 * @param rank Rank of the current process, only messages form rank
	 *  0 will be printed
	 */
	Logger(DebugType t, int rank)
		: stream(new Stream(t, rank))
	{
		stream->buffer << utils::TimeUtils::timeAsString(LOG_PREFIX, time(0L));

		switch (t) {
		case DEBUG:
			stream->buffer << ", Debug: ";
			break;
		case INFO:
			stream->buffer << ", Info:  ";
			break;
		case WARNING:
			stream->buffer << ", Warn:  ";
			break;
		case ERROR:
			stream->buffer << ", Error: ";
			break;
		}
	}
	/**
	 * Copy constructor
	 */
	Logger(const Logger& o) : stream(o.stream) { stream->ref++; };
	~Logger()
	{
		if (!--stream->ref) {
			if (stream->rank == 0) {
				if (stream->type == INFO)
					std::cout << stream->buffer.str() << std::endl;
				else
					std::cerr << stream->buffer.str() << std::endl;
			}

			if (stream->type == ERROR) {
				delete stream;
				LOG_ABORT;
			}

			delete stream;
		}
	}
	
	/**
	 * Copy operator
	 */
	Logger &operator=(const Logger& other)
	{
		if (this != &other) {
			Logger copy(other);
			std::swap(stream, copy.stream);
		}
		return *this;
	}
	
	
	/********* Space handling *********/
	
	/**
	 * Add a space to output message and activate spaces
	 */
	Logger &space()
	{
		stream->space = true;
		stream->buffer << ' ';
		return *this;
	}
	/**
	 * Deactivate spaces
	 */
	Logger &nospace()
	{
		stream->space = false;
		return *this;
	}
	/**
	 * Add space of activated
	 */
	Logger &maybeSpace()
	{
		if (stream->space)
			stream->buffer << ' ';
		return *this;
	}
	
	/**
	 * Default function to add messages
	 */
	template<typename T>
	Logger &operator<<(T t)
	{
		stream->buffer << t;
		return maybeSpace();
	}

	/**
	 * Add a string variable to the message
	 */
	Logger &operator<<(const std::string& t)
	{
		stream->buffer << '"' << t << '"';
		return maybeSpace();
	}

	/**
	 * Add a string variable to the message
	 */
	Logger &operator<<(std::string& t)
	{
		stream->buffer << '"' << t << '"';
		return maybeSpace();
	}

	/**
	 * Operator to add functions like std::endl
	 */
	Logger &operator<<(std::ostream& (*func)(std::ostream&))
	{
		stream->buffer << func;
		return *this; // No space in this case
	}
};

/**
 * Dummy Logger class, does nothing
 */
class NoLogger
{
public:
	NoLogger() {};
	~NoLogger() {};
	
	/**
	 * Do nothing with the message
	 */
	template<typename T>
	NoLogger &operator<<(const T&)
	{
		return *this;
	}

	/**
	 * Operator to add functions like std::endl
	 */
	NoLogger &operator<<(std::ostream& (*func)(std::ostream&))
	{
		return *this;
	}
};

/**
 * Add a std::vector<T> to the message
 */
template <typename T>
inline Logger &operator<<(Logger debug, const std::vector<T> &list)
{
	debug.nospace() << '(';
	for (size_t i = 0; i < list.size(); i++) {
		if (i)
			debug << ", ";
		debug << list[i];
	}
	debug << ')';
	
	return debug.space();
}

}

// Define global functions

/**
 * Create error message and exit
 */
inline
utils::Logger logError()
{
	return utils::Logger(utils::Logger::ERROR, 0);
}

#if LOG_LEVEL >= 1
/**
 * Create a warning message if enabled
 *
 * @relates utils::Logger
 */
inline
utils::Logger logWarning( int rank = 0 )
{
	return utils::Logger(utils::Logger::WARNING, rank);
}
#else // LOG_LEVEL >= 1
/**
 * Create a dummy warning message if disabled
 *
 * @relates utils::NoLogger
 */
inline
utils::NoLogger logWarning( int = 0 ) { return utils::NoLogger(); }
#endif // LOG_LEVEL >= 1

#if LOG_LEVEL >= 2
/**
 * Create a info message if enabled
 *
 * @relates utils::Logger
 */
inline
utils::Logger logInfo( int rank = 0 )
{
	return utils::Logger(utils::Logger::INFO, rank);
}
#else // LOG_LEVEL >= 2
/**
 * Create a dummy info message if disabled
 *
 * @relates utils::NoLogger
 */
inline
utils::NoLogger logInfo( int = 0 ) { return utils::NoLogger(); }
#endif // LOG_LEVEL >= 2

#if LOG_LEVEL >= 3
/**
 * Create a debug message if enabled
 *
 * @relates utils::Logger
 */
inline
utils::Logger logDebug( int rank = 0 )
{
	return utils::Logger(utils::Logger::DEBUG, rank);
}
#else // LOG_LEVEL >= 3
/**
 * Create a dummy debug message if disabled
 *
 * @relates utils::NoLogger
 */
inline
utils::NoLogger logDebug( int = 0 ) { return utils::NoLogger(); }
#endif // LOG_LEVEL >= 3


// Use for variables unused when compiling with NDEBUG
#ifdef NDEBUG
#define NDBG_UNUSED(x) ((void) x)
#else // NDEBUG
#define NDBG_UNUSED(x)
#endif // NDEBUG

#endif // UTILS_LOGGER_H
