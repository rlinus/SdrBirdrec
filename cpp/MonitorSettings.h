
namespace SdrBirdrec
{
	using namespace std;
	using namespace tbb::flow;

	/*!
	* This struct bundles properties to control the monitor output of SdrBirdrec.
	*/
	struct MonitorSettings
	{
	public:
		enum class ChannelType { sdr, daqmx };
		const map<string, ChannelType> channelTypeMap = { { "sdr"s, ChannelType::sdr }, { "daqmx"s, ChannelType::daqmx } };
		bool play_audio = false;
		ChannelType channel_type = ChannelType::sdr;
		size_t channel_number = 0;
		double squelch = -2000.0;

		void set(Kwargs options)
		{
			auto search = options.find("play_audio");
			if(search != options.end()) play_audio = bool(stoi(search->second));

			search = options.find("channel_number");
			if(search != options.end()) channel_number = stoull(search->second);

			search = options.find("channel_type");
			if(search != options.end() && channelTypeMap.find(search->second) != channelTypeMap.cend()) channel_type = channelTypeMap.at(search->second);

			search = options.find("squelch");
			if(search != options.end()) squelch = stod(search->second);
		}
	};
}