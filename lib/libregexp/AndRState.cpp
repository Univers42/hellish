#include "RegexState.hpp"

void AndRState::AddState(RegexState* state)
{
	states.push_back(state);
}

void AndRState::AddAction(void(*f)(std::string))
{
	states.back()->AddAction(f);
}

void AndRState::AddQuantity(int min, int max)
{
	states.back()->SetQuantity(min, max);
}

void AndRState::SetQuantity(int min, int max)
{
	states.back()->SetQuantity(min, max);
}

AndRState::AndRState(int lvl)
{
	this->lvl = lvl;
	this->print = true;
}

AndRState::AndRState(AndRState& other)
{
	this->print = other.print;
	this->states = std::vector<RegexState*>(other.states);
}

AndRState::~AndRState()
{
    for (std::vector<RegexState*>::iterator it = states.begin(); it != states.end(); ++it)
        delete *it;
    states.clear();
}

void AndRState::AddChar(char c)
{
	if (!states.empty()
		&& dynamic_cast<OrRState*>(states.back()))
		states.back()->AddChar(c);
	else
		states.push_back(new CharRState(c, lvl + 1));
}

void AndRState::PrintChild()
{
	std::cout << std::endl << "AndRState: ";
	for (std::vector<RegexState*>::const_iterator state = states.begin(); state != states.end(); ++state)
	{
		(*state)->PrintChild();
	}
}

bool AndRState::Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches)
{
	size_t start = pos;
	for (std::vector<RegexState*>::const_iterator state = states.begin(); state != states.end(); ++state)
	{
		bool result = (*state)->Matches(input, start, matches);

		// Sale si todo está correcto pero queda por leer.
		if (result && state + 1 != states.end() && input[start] == '\0')
			throw std::exception();

		if (!result)
			return false;
	}
	pos = start;
	return true;
}

/// <summary>
/// Método para el proyecto Webserv, funciona igual que el match, pero serializa si se ha llegado al final
///		de la cadena y aún sigue siendo válida la expresión regular.
/// </summary>
/// <param name="input"></param>
/// <param name="pos"></param>
/// <param name="record"></param>
/// <returns>Ver RegexState.hpp</returns>
Result AndRState::StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record)
{	// MEJORA: Se podría realizar con un patrón Visitor para no modificar la estructura de clases del regex.
	size_t start;
	size_t none;
	GetProgress(record, pos, start, cstate, none);

	while (cstate < states.size())
	{
		Result result = states[cstate]->StaticMatch(input, start, record);
		switch (result)
		{
			case REQUEST_CONTINUE:
			{
				SetProgress(record, start, cstate, none);
				return REQUEST_CONTINUE;
			}
			case REQUEST_SUCCESS: break;
			case REQUEST_ERROR: return REQUEST_ERROR;
		}

		cstate++;
		if (result == REQUEST_SUCCESS && cstate != states.size() && input[start] == '\0')
		{
			SetProgress(record, start, cstate, none);
			return REQUEST_CONTINUE;
		}
	}
	pos = start;
	return REQUEST_SUCCESS;
}

/// <summary>
/// Permite eliminar los nodos innecesarios (si este nodo contiene un hijo, este nodo se vuelve innecesario)
/// </summary>
/// <param name="toBeFree"></param>
/// <returns></returns>
RegexState* AndRState::Simplify(std::vector<RegexState*>& toBeFree)
{
	if (states.size() == 1)
	{
		toBeFree.push_back(this);
		return states[0]->Simplify(toBeFree);
	}
	else
	{
		for (size_t i = 0; i < states.size(); i++)
			states[i] = states[i]->Simplify(toBeFree);
	}
	return this;
}