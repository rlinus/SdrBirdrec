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

		void write_line(const string& msg, bool to_cout = true)
		{
			lock_guard<mutex> lock(m);
			logfile << msg << endl;
			if(to_cout) cout << msg << endl;
		}

		void write(const string& msg, bool to_cout = true)
		{
			lock_guard<mutex> lock(m);
			logfile << msg;
			logfile.flush();
			if(to_cout)
			{
				cout << msg;
				cout.flush();
			}
		}

	private:
		ofstream logfile;
		mutex m;
	};
}
