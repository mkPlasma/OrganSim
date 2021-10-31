#pragma once
#include <string>
#include <vector>

struct PipeParameters {
	float pipeWidth;
	float pipeLength;

	float maxPressure;
	float flueWidth;
	float mouthSize;
	float labiumOffset;
	float widthParam;
};

class PipeManager
{
public:
	PipeManager(const std::string);
	virtual ~PipeManager();
	std::vector<PipeParameters> getPipes() const;
private:
	std::vector<PipeParameters> pipes;
};

