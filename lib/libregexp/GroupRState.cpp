#include "RegexState.hpp"

GroupRState::GroupRState()
{
	conditions = new OrRState(lvl + 1);
	OnValidate = NULL;
}

void GroupRState::AddChar(char c)
{
	conditions->AddChar(c);
}

void GroupRState::AddAction(void (*f)(std::string))
{
	OnValidate = f;
}

void GroupRState::AddQuantity(int min, int max)
{
	conditions->AddQuantity(min, max);
}

void GroupRState::AddState(RegexState* state)
{
	conditions->AddState(state);
}

void GroupRState::PrintChild()
{
	std::cout << "Group: ";
	conditions->PrintChild();
	std::cout << std::endl << "Group End ";
}

bool GroupRState::Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches)
{
	size_t start = pos;
	size_t start2 = pos;
	bool result = conditions->Matches(input, start, matches);
	size_t cant = 0;
	while (result && cant < max)
	{
		start2 = start;
		cant++;
		result = conditions->Matches(input, start, matches);
	}
	if (cant >= min && start != pos)
	{
		std::string matching = input.substr(pos, start2 - pos);
		if (OnValidate)
			OnValidate(matching);
		matches.push_back(matching);
		pos = start;
	}
	return cant >= min;
}

/// <summary>
/// Método para el proyecto Webserv, funciona igual que el match, pero serializa si se ha llegado al final
///		de la cadena y aún sigue siendo válida la expresión regular.
/// </summary>
/// <param name="input"></param>
/// <param name="pos"></param>
/// <param name="record"></param>
/// <returns>Ver RegexState.hpp</returns>
Result GroupRState::StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record)
{	// MEJORA: Se podría realizar con un patrón Visitor para no modificar la estructura de clases del regex.
		size_t start;
	size_t cant;
	size_t none;
	GetProgress(record, pos, start, none, cant);
	size_t start2 = start;

	Result result = conditions->StaticMatch(input, start, record);
	while (result != REQUEST_ERROR && cant < max)
	{
		if (result == REQUEST_CONTINUE)
		{
			SetProgress(record, start, none, cant);
			return REQUEST_CONTINUE;
		}
		start2 = start;
		cant++;
		if (cant < max)
			result = conditions->StaticMatch(input, start, record);
	}

	if (cant >= min)
	{
		if (OnValidate && start != pos)
			OnValidate(input.substr(pos, start2 - pos));
		pos = start;
		return REQUEST_SUCCESS;
	}
	return REQUEST_ERROR;
}

/// <summary>
/// Permite eliminar los nodos innecesarios, actualmente este nodo no es posible simplificarlo.
/// </summary>
/// <param name="toBeFree"></param>
/// <returns></returns>
RegexState* GroupRState::Simplify(std::vector<RegexState*>& toBeFree)
{
	conditions = conditions->Simplify(toBeFree);
	return this;
}