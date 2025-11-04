#include "RegexState.hpp"

RegexState::RegexState()
{
	this->min = 1;
	this->max = 1;
}

RegexState::~RegexState() { }

void RegexState::AddQuantity(int min, int max)
{
	this->min = min;
	this->max = max;
}

void RegexState::SetQuantity(int min, int max)
{
	this->min = min;
	this->max = max;
}

bool RegexState::Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches)
{
	(void)input;
	(void)pos;
	(void)matches;
	return false;
}

/// <summary>
/// Para WebServ42. Permite deserializar la posición en la cadena a ser validada y el estado actual de los nodos.
/// </summary>
/// <param name="record"></param>
/// <param name="pos"></param>
/// <param name="start"></param>
/// <param name="cstate"></param>
/// <param name="cant"></param>
void RegexState::GetProgress(std::map<RegexState*, CurrentState>& record, int pos, size_t& start, size_t& cstate, size_t& cant)
{
	std::map<RegexState*, CurrentState>::iterator it = record.find(this);
	if (it != record.end())
	{
		start = it->second.start;
		cstate = it->second.cstate;
		cant = it->second.cant;

		record.erase(it);
	}
	else
	{
		start = pos;
		cstate = 0;
		cant = 0;
	}
}

/// <summary>
/// Para WebServ42. Permite serializar la posición en la cadena a ser validada y el estado actual de los nodos.
/// </summary>
/// <param name="record"></param>
/// <param name="start"></param>
/// <param name="cstate"></param>
/// <param name="cant"></param>
void RegexState::SetProgress(std::map<RegexState*, CurrentState>& record, size_t& start, size_t& cstate, size_t& cant)
{
	record[this].start = start;
	record[this].cstate = cstate;
	record[this].cant = cant;
}
