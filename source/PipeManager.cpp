#include "PipeManager.h"

#include <fstream>
#include <iostream>
#include <json.hpp>
#include <string>


PipeManager::PipeManager(const std::string filePath)
{
	std::ifstream i(filePath);
	nlohmann::json models;
	i >> models;

	// Iterate through json objects
	for(auto &obj : models)
	{
		PipeParameters pipe{};
		pipe.pipeWidth = obj["pipeWidth"];
		pipe.pipeLength = obj["pipeLength"];
		pipe.maxPressure = obj["maxPressure"];
		pipe.flueWidth = obj["flueWidth"];
		pipe.mouthSize = obj["mouthSize"];
		pipe.labiumOffset = obj["labiumOffset"];

		this->pipes.push_back(pipe);
	}
}

std::vector<PipeParameters> PipeManager::getPipes() const
{
	return this->pipes;
}

PipeManager::~PipeManager() = default;