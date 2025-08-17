#ifndef _EAGE_UTILITY_LOGGER_H
#define _EAGE_UTILITY_LOGGER_H

#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <ctime>
#include <memory>
#include <thread>

#ifndef DEBUG
#   define EAGE_ASSERT(Expr, Msg) \
    EAGE_ASSERT_IMPL(#Expr, Expr, __FILE__, __LINE__, Msg)
#else
#   define EAGE_ASSERT(Expr, Msg) ;
#endif

inline void EAGE_ASSERT_IMPL( const char* expr_str, bool expr, const char* file, int line, const char* msg )
{
    if( !expr )
    {
        std::cerr << "Assert failed:\t" << msg << "\n"
            << "Expected:\t" << expr_str << "\n"
            << "Source:\t\t" << file << ", line " << line << "\n";
        abort();
    }
}

namespace utility
{
	// Severity levels
	enum class LOG_LEVEL : uint8_t
	{
		TRACE	= 0,
		DEBUG	= 1,
		INFO	= 2,
		WARNING = 3,
		EAGE_ERROR	= 4
	};
	
	const size_t LOG_PREFIX_LENGTH = 128;
	
	// Uncolored text prefix for severity levels
	const std::unordered_map<LOG_LEVEL, std::string> uncolored
	{
		{ LOG_LEVEL::EAGE_ERROR, " [ERROR] " },
		{ LOG_LEVEL::WARNING,    " [WARNING] " },
		{ LOG_LEVEL::INFO,       " [INFO] " },
		{ LOG_LEVEL::DEBUG,      " [DEBUG] " },
		{ LOG_LEVEL::TRACE,      " [TRACE] " },
	};
	
	// Colored text prefix for severity levels
	const std::unordered_map<LOG_LEVEL, std::string> colored
	{
		{ LOG_LEVEL::EAGE_ERROR, " \x1b[31;1m[ERROR]\x1b[0m " },
		{ LOG_LEVEL::WARNING, " \x1b[33;1m[WARN]\x1b[0m " },
		{ LOG_LEVEL::INFO, " \x1b[32;1m[INFO]\x1b[0m " },
		{ LOG_LEVEL::DEBUG, " \x1b[34;1m[DEBUG]\x1b[0m " },
		{ LOG_LEVEL::TRACE, " \x1b[37;1m[TRACE]\x1b[0m " },
	};
	
	// Cutoff logging message base on the severity level from preprocessor
	#if defined( LOGGING_LEVEL_ALL ) || defined( LOGGING_LEVEL_TRACE )
		constexpr LOG_LEVEL LOG_LEVEL_CUTOFF = LOG_LEVEL::TRACE;
	#elif defined( LOGGING_LEVEL_DEBUG )
	    constexpr LOG_LEVEL LOG_LEVEL_CUTOFF = LOG_LEVEL::DEBUG;
	#elif defined( LOGGING_LEVEL_WARN )
	    constexpr LOG_LEVEL LOG_LEVEL_CUTOFF = LOG_LEVEL::WARNING;
	#elif defined( LOGGING_LEVEL_ERROR )
	    constexpr LOG_LEVEL LOG_LEVEL_CUTOFF = LOG_LEVEL::ERROR;
	#elif defined( LOGGING_LEVEL_NONE )
	    constexpr LOG_LEVEL LOG_LEVEL_CUTOFF = LOG_LEVEL::ERROR + 1;
	#else
	    constexpr LOG_LEVEL LOG_LEVEL_CUTOFF = LOG_LEVEL::INFO;
	#endif
	
	// helper function to get formated log time: 'yyyy/mm/dd hh:mm:ss.xxxxxx'
	inline std::string timestamp()
	{
		// get the current time
		std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
		std::time_t tt = std::chrono::system_clock::to_time_t( tp );
		std::tm lct{};
		localtime_s( &lct, &tt );
		std::chrono::duration<double> fractional_seconds = ( tp - std::chrono::system_clock::from_time_t( tt ) ) + std::chrono::seconds( lct.tm_sec );
	
		// format the time string
		std::string buffer( "yyyy/mm/dd hh:mm:ss.xxxxxx" );
		sprintf_s( &buffer.front(), 27u, "%04d/%02d/%02d %02d:%02d:%09.6f", lct.tm_year + 1900, lct.tm_mon + 1, lct.tm_mday, lct.tm_hour, lct.tm_min, fractional_seconds.count() );
		return buffer;
	}
	
	// Logger base class
	using logging_config_t = std::unordered_map<std::string, std::string>;
	class Logger
	{
	public:
		Logger() = delete;
		Logger( const logging_config_t& config ) {};
	
		virtual ~Logger() {};
		virtual void Log( const std::string&, const LOG_LEVEL ) {};
		virtual void Log( const std::string& ) {};
	
		// Register the thread so logging will contain the thread's given name
		virtual void RegisterThread( const std::thread::id id, const std::string thread_name )
		{
			if( id != std::thread::id{} )
			{
				mThreadIds[ id ] = thread_name;
			}
		}
	
		// Deregister the thread which is no long used
		virtual void DeregisterThread( const std::thread::id id )
		{
			if( id != std::thread::id{} )
			{
				mThreadIds.erase( id );
			}
		}
	
	protected:
		std::mutex mLocks;
		std::unordered_map<std::thread::id, std::string> mThreadIds;
	};
	
	// Standard out logger
	class StdOutLogger : public Logger
	{
	public:
		StdOutLogger() = delete;
		StdOutLogger( const logging_config_t& config )
		: Logger( config )
		, mLevels( config.find( "color" ) != config.end() ? colored : uncolored )
		{}
	
		virtual void 
		Log( const std::string& message, const LOG_LEVEL level )
		{
			if( level < LOG_LEVEL_CUTOFF )
				return;
	
			std::string thread_name = " [ UNKNOW_THREAD ]";
			auto result = mThreadIds.find( std::this_thread::get_id() );
			if( result != mThreadIds.end() )
			{
				thread_name = " [ " + result->second + " ]	";
			}
	
			std::string output;
			output.reserve( message.length() + LOG_PREFIX_LENGTH );
			output.append( timestamp() );
			output.append( thread_name );
			output.append( mLevels.find( level )->second );
			output.append( message );
			output.push_back( '\n' );
			Log( output );
		}
	
		virtual void 
		Log( const std::string& message )
		{
			std::cout << message;
			std::cout.flush();
		}
	
	protected:
		const std::unordered_map<LOG_LEVEL, std::string> mLevels;
	};
	
	// Logger factory
	// Create loggers via function pointers
	using logger_creator = std::unique_ptr<Logger> (*)( const logging_config_t& );
	class LoggerFactory
	{
	public:
		LoggerFactory()
		{
			mCreators.emplace( "", []( const logging_config_t& config )->std::unique_ptr<Logger>{ return std::make_unique<Logger>( config ); } );
			mCreators.emplace( "std_out", []( const logging_config_t& config )->std::unique_ptr<Logger>{ return std::make_unique<StdOutLogger>( config ); } );
		}
	
		std::unique_ptr<Logger>
		Produce( const logging_config_t& config ) const
		{
			// get the logger type
			auto type = config.find( "type" );
			if( type == config.end() )
			{
				throw std::runtime_error( "Logger factory requires a configuration contains a tpye of logger" );
			}
	
			// get the logger
			auto found = mCreators.find( type->second );
			if( found == mCreators.end() )
			{
				throw std::runtime_error( "Failed to produce the logger for type: " + type->second );
			}
	
			return found->second( config );
		}
	
	
	private:
		std::unordered_map<std::string, logger_creator> mCreators;
	};
	
	// statically get a factory
	inline LoggerFactory& GetFactory()
	{
		static LoggerFactory factory_singleton{};
		return factory_singleton;
	}
	
	// get logger from the factory
	inline Logger& GetLogger( const logging_config_t& config = { { "type", "std_out" }, { "color", "" } } )
	{
		static std::unique_ptr<Logger> singleton( GetFactory().Produce( config ) );
		return *singleton;
	}

	// Stream-based logging helper class
	class LogStream
	{
	public:
		LogStream(LOG_LEVEL level, bool shouldAssert = false) : mLevel(level), mShouldAssert(shouldAssert) {}
		
		~LogStream()
		{
			std::string message = mStream.str();
			GetLogger().Log(message, mLevel);
			
			if (mShouldAssert)
			{
				EAGE_ASSERT(false, message.c_str());
			}
		}
		
		template<typename T>
		LogStream& operator<<(const T& value)
		{
			mStream << value;
			return *this;
		}
		
	private:
		LOG_LEVEL mLevel;
		bool mShouldAssert;
		std::ostringstream mStream;
	};
	
	// Stream-based LOG functions
	inline LogStream LOG(LOG_LEVEL level)
	{
		return LogStream(level);
	}
	
	inline LogStream LOG()
	{
		return LogStream(LOG_LEVEL::INFO);
	}
	
	inline LogStream LOG_ERROR()
	{
		return LogStream(LOG_LEVEL::EAGE_ERROR, true);
	}
	
	// String-based LOG functions (existing)
	inline void LOG( const LOG_LEVEL level, const std::string& message )
	{
		GetLogger().Log( message, level );
	}
	
	inline void LOG( const std::string& message )
	{
		GetLogger().Log( message, LOG_LEVEL::INFO );
	}
	
	inline void LOG_ERROR( const std::string& message )
	{
		GetLogger().Log( message, LOG_LEVEL::EAGE_ERROR );
		EAGE_ASSERT(false, message.c_str());
	}
}


#endif // _EAGE_UTILITY_LOGGER_H