# include "Automata.hpp"

void Automata::innit()
{
	this->alphabet = NULL;
	this->errors = NULL;
	this->get_state = NULL;
	this->data = NULL;
}

Automata::Automata(int (*getstate)(int, int))
{
	innit();
	this->get_state = getstate;
}

Automata::Automata(void *data)
{
	innit();
	this->data = data;
}

Automata::Automata()
{
	innit();
}

Automata::~Automata()
{
	std::cout << "Automata destructor start" << std::endl;
}

int Automata::alphabet_idx(std::string *alphabet, char c)
{
	int i = -1;

	while (++i < this->alphabet_length)
	{
		if (alphabet[i].find(c) != std::string::npos)
			return (i);
	}
	return (i);
}

int Automata::evaluate()
{
	this->o_idx = 0;
	this->c_idx = -1;

	while (++c_idx < (int)str.length())
	{
		this->state = this->get_state(this->state, alphabet_idx(this->alphabet, this->str[this->c_idx]));
		this->ostate = this->state;
	}
	return (this->state);
}

void Automata::set_string(const std::string& literal)
{
	this->str = literal;
}

void Automata::set_alphabet(size_t size, const std::string* strings)
{
	this->alphabet = new std::string[size];
	this->alphabet_length = size;
	for (size_t i = 0; i < size; i++)
		this->alphabet[i] = strings[i];
}

void Automata::set_errors(size_t size, const std::string* strings)
{
	this->errors = new std::string[size];
	this->errors_length = size;
	for (size_t i = 0; i < size; i++)
		this->errors[i] = strings[i];
}

void Automata::print_state()
{
	std::cout << "State: " << this->state << std::endl;
}