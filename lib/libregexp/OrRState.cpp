#include "RegexState.hpp"

OrRState::OrRState(int lvl)
{
	this->lvl = lvl;
	states.push_back(new AndRState(lvl + 1));
}

void OrRState::AddChar(char c)
{
	states.back()->AddChar(c);
}

void OrRState::AddState(RegexState* state)
{
	if (dynamic_cast<AndRState*>(state))
		states.push_back(state);
	else
		states.back()->AddState(state);
}

void OrRState::AddAction(void(*f)(std::string))
{
	states.back()->AddAction(f);
}

void OrRState::AddQuantity(int min, int max)
{
	states.back()->AddQuantity(min, max);
}

void OrRState::SetQuantity(int min, int max)
{
	states.back()->AddQuantity(min, max);
}

void OrRState::PrintChild()
{
	std::cout << "OrRState: ";
	for (std::vector<RegexState*>::const_iterator state = states.begin(); state != states.end(); ++state)
	{
		(*state)->PrintChild();
	}
	std::cout << std::endl << "Or End";
}

bool OrRState::Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches)
{
	size_t start = pos;
	for (std::vector<RegexState*>::const_iterator state = states.begin(); state != states.end(); ++state)
	{
		bool result = (*state)->Matches(input, start, matches);
		if (result)
		{
			pos = start;
			return true;
		}
	}
	return false;
}

/// <summary>
/// Método para el proyecto Webserv, funciona igual que el match, pero serializa si se ha llegado al final
///		de la cadena y aún sigue siendo válida la expresión regular.
/// </summary>
/// <param name="input"></param>
/// <param name="pos"></param>
/// <param name="record"></param>
/// <returns>Ver RegexState.hpp</returns>
Result OrRState::StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record)
{
	size_t start;
	size_t none;
	GetProgress(record, pos, start, cstate, none);

	while (cstate < states.size())
	{
		switch (states[cstate]->StaticMatch(input, start, record))
		{
			case REQUEST_CONTINUE:
			{
				SetProgress(record, start, cstate, none);
				return REQUEST_CONTINUE;
			}
			case REQUEST_SUCCESS:
			{
				pos = start;
				return REQUEST_SUCCESS;
			}
			case REQUEST_ERROR: break;
		}
		cstate++;
	}
	return REQUEST_ERROR;
}

/// <summary>
/// Permite eliminar los nodos innecesarios (si este nodo contiene un hijo, este nodo se vuelve innecesario)
/// </summary>
/// <param name="toBeFree"></param>
/// <returns></returns>
RegexState* OrRState::Simplify(std::vector<RegexState*>& toBeFree)
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
