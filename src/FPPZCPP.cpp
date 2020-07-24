
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstring>

#include <istream>
#include <ostream>

#include <filesystem>

#include <unistd.h>
#include <termios.h>
#include <chrono>
#include <thread>
#include <cmath>

#include <httpserver.hpp>
#include "common.h"
#include "settings.h"
#include "Plugin.h"
#include "Plugins.h"
#include "log.h"
#include "channeloutput/channeloutput.h"
#include "mqtt.h"
#include "MultiSync.h"

#include "fppversion_defines.h"

#include "commands/Commands.h"

#include "ZCPPOutput.h"

class FPPZcppPlugin : public FPPPlugin, public httpserver::http_resource {
private:
    std::vector<std::unique_ptr <ZCPPOutput>> _zcppOutputs;

public:

    FPPZcppPlugin() : FPPPlugin("fpp-zcpp") {
        readFiles();
        sendConfigFileNow();
        registerCommand();
    }
    virtual ~FPPZcppPlugin() 
    {
         _zcppOutputs.clear();
    }
    
    class SendConfigCommand : public Command {
    public:
        SendConfigCommand(FPPZcppPlugin *p) : Command("SendConfig"), plugin(p) {
             args.push_back(CommandArg("SendConfig", "bool", "Send Configs to Controllers").setDefaultValue("true"));
        }
        
        virtual std::unique_ptr<Command::Result> run(const std::vector<std::string> &args) override {
            bool sendconfigs = true;
            if (args.size() >= 1) {
                sendconfigs= args[1] == "true" || args[1] == "1";
            }
            plugin->sendConfigFileNow(sendconfigs);
            return std::make_unique<Command::Result>("Sending Configs");
        }
        FPPZcppPlugin *plugin;
    };
    void registerCommand() {
        CommandManager::INSTANCE.addCommand(new SendConfigCommand(this));
    }
    
    virtual const std::shared_ptr<httpserver::http_response> render_GET(const httpserver::http_request &req) override {
        std::string p0 = req.get_path_pieces()[0];
        int plen = req.get_path_pieces().size();
        if (plen > 1) {
            std::vector<std::string> vals;
            for (int x = 1; x < req.get_path_pieces().size(); x++) {
                std::string p1 = req.get_path_pieces()[x];
                vals.push_back(p1);
            }
            sendConfigFileNow();
        }
        
        std::string v = getIPs();
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(v, 200));
    }
    
#if FPP_MAJOR_VERSION < 4 || FPP_MINOR_VERSION < 1
    virtual void modifyChannelData(int ms, uint8_t *seqData) override {
#else
    virtual void modifySequenceData(int ms, uint8_t *seqData) override {
#endif

    }
    
    void sendConfigFileNow(bool sendConfig = true) {
        for(auto & output: _zcppOutputs)
		{
			output->SendConfig();
		}
    }

	std::vector<std::string> getZCPPFile(std::string const& folder)
	{
		std::vector<std::string> files;
		std::string path(folder);
		std::string ext(".zcpp");
		for (auto& p : std::filesystem::recursive_directory_iterator(path))
		{
			if (p.path().extension() == ext)
			{
				files.push_back(p.path().string());
			}
		}
		return files;
	}
	
	void readFiles()
	{
		std::vector<std::string> files = getZCPPFile("/home/fpp/media/config/");
		if (files.size() > 0)
		{
			for(auto const& file: files)
			{
				std::unique_ptr<ZCPPOutput> output = std::make_unique<ZCPPOutput>();
				bool worked = output->ReadConfig(file);
				if(worked)
				{
					_zcppOutputs.push_back(std::move(output));
				}
			}
		}
		std::vector<std::string> files2 = getZCPPFile("/home/fpp/media/upload/");
		if (files2.size() > 0)
		{
			for(auto const& file: files2)
			{
				std::unique_ptr<ZCPPOutput> output = std::make_unique<ZCPPOutput>();
				bool worked = output->ReadConfig(file);
				if(worked)
				{
					_zcppOutputs.push_back(std::move(output));
				}
			}
		}
	}
	
	std::string getIPs()
	{
		std::string ips;
		for(auto & out: _zcppOutputs)
		{
			ips += out->GetIPAddress();
			ips += ",";
		}
		return ips;
	} 
};


extern "C" {
    FPPPlugin *createPlugin() {
        return new FPPZcppPlugin();
    }
}
