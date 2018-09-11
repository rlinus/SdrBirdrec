#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <memory>
#include <atomic>
#include <mutex>

namespace SdrBirdrec
{
	using namespace std;

	class SyncedLogger
	{
	public:
		SyncedLogger(const string &filename)
		{
			//logfile.open(filename, ios::trunc);
			logfile.open(filename, ios::app);
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

	private:
		ofstream logfile;
		ostringstream logbuffer;
		mutex m;
	};
}
