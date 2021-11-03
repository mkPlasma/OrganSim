#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "pipeParameters.h"

using std::string;
using std::vector;
using std::unordered_map;


class PipeManager
{
public:
	PipeManager(const string&);
	int getNoteIndex(const string& noteStr);
	unordered_map<int, PipeParameters>& getPipes();
private:
	unordered_map<int, PipeParameters> pipes;
};
