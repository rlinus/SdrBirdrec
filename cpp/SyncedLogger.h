#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <memory>
#include <atomic>
#include <mutex>

#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace SdrBirdrec
{
	using namespace std;


	/*!
	* Thread save logger. Logs to textfile and buffer, that can be output to stdout with call to logbuffer2cout() method.
	*/
	class SyncedLogger
	{
	public:
		SyncedLogger(const string &filename)
		{
			logfile.open(filename, ios::trunc);
			//logfile.open(filename, ios::app);
		}

		~SyncedLogger()
		{
			logfile.close();
		}

		void write_line(const string& msg)
		{
			lock_guard<mutex> lock(m);
			logfile << msg << endl;
			logbuffer << msg << endl;
		}

		void write(const string& msg)
		{
			lock_guard<mutex> lock(m);
			logfile << msg;
			logfile.flush();
			logbuffer << msg;
			logbuffer.flush();
		}

		void logbuffer2cout()
		{
			lock_guard<mutex> lock(m);
			cout << logbuffer.str();
			cout.flush();
			logbuffer.str("");
			logbuffer.clear();
		}

		std::string time2string(std::chrono::system_clock::time_point& t)
		{
			using namespace std::chrono;

			// get number of milliseconds for the current second
			// (remainder after division into seconds)
			auto ms = duration_cast<milliseconds>(t.time_since_epoch()) % 1000;

			// convert to std::time_t in order to convert to std::tm (broken time)
			auto timer = system_clock::to_time_t(t);

			// convert to broken time
			std::tm bt = *std::localtime(&timer);

			std::ostringstream oss;
			oss << std::put_time(&bt, "%d-%m-%Y %H:%M:%S"); // HH:MM:SS
			oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

			return oss.str();
		}

	private:
		ofstream logfile;
		ostringstream logbuffer;
		mutex m;
	};
}
