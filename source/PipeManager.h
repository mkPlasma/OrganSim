#pragma once
#include <string>
#include <vector>
#include "pipeParameters.h"


class PipeManager
{
public:
	PipeManager(const std::string);
	virtual ~PipeManager();
	std::vector<PipeParameters> getPipes() const;
private:
	std::vector<PipeParameters> pipes;
};

