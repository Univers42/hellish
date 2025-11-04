#include "RegexState.hpp"

CharRState::CharRState(char c, int lvl)
{
	this->lvl = lvl;
	character = c;
}

void CharRState::PrintChild()
{
	std::cout << character;
}

bool CharRState::Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches)
{
	(void)matches;
	size_t cant = 0;
	while (pos < input.size() && cant < max && input[pos] == character)
	{
		pos++;
		cant++;
	}
	return cant >= min;
}

Result CharRState::StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record)
{
	size_t cant = 0;
	size_t none = 0;
	GetProgress(record, 0, none, none, cant);
	while (pos < input.size() && cant < max && input[pos] == character)
	{
		pos++;
		cant++;
	}

	if (cant < max && !input[pos])
	{
		SetProgress(record, none, none, cant);
		return REQUEST_CONTINUE;
	}
	return cant >= min ? REQUEST_SUCCESS : REQUEST_ERROR;
}
