# include "RegexParser.hpp"
# include <fstream>

enum s_states
{
	EMPTY,
	ERROR,
	CHAR,
	MCHARS,
	MCHARE,
	ORS,
	STAR,
	GROUPS,
	GROUPE,
	BACKSLASH,
	ESCAPEDCHAR,
	PLUS,
	ACTION,
	ACTINDEX,
	QUESTION,
	CurlyOpen,
	MinLenght,
	LengthSep,
	MaxLenght,
	CurlyClose,
};

void RegexParser::innit()
{
	std::cout << "Automata constructor called" << std::endl;

	const std::string strings[] = { "[", "]", "{", "}", ",", "|", "(", ")", "~", "0123456789", "*", "+", "?", "\\", "^", " \t\n" };
	const size_t size = sizeof(strings) / sizeof(std::string);

	for (int i = 0; i < 20; i++)
		fsa[i] = NULL;
	for (int i = 0; i < 20; i++)
		for (int j = 0; j < 20; j++)
			fta[i][j] = NULL;

	state = 0;
	ostate = 0;

	// Events
	fsa[CHAR] = &RegexParser::character;
	fsa[ERROR] = &RegexParser::errorfound;
	fsa[ORS] = &RegexParser::orfound;
	fsa[MCHARS] = &RegexParser::setstart;
	fsa[MCHARE] = &RegexParser::setend;
	fsa[STAR] = &RegexParser::star;
	fsa[PLUS] = &RegexParser::plus;
	fsa[QUESTION] = &RegexParser::question;
	fsa[GROUPS] = &RegexParser::groupstart;
	fsa[GROUPE] = &RegexParser::groupending;
	fsa[CurlyOpen] = &RegexParser::move;
	fsa[CurlyClose] = &RegexParser::minmaxlength;
	fta[BACKSLASH][ESCAPEDCHAR] = &RegexParser::backslash;
	fta[MCHARS][ESCAPEDCHAR] = &RegexParser::negate;
	fta[GROUPE][ACTION] = &RegexParser::move;

	for (int i = 0; i < 19; i++)
		fta[ACTINDEX][i] = &RegexParser::action;

	set_alphabet(size, strings);
	root = new OrRState(0);
	errors = NULL;

	std::cout << "Regex Automata Inizialized" << std::endl;
}

/// <summary>
/// States transitions
/// </summary>
/// <param name="i"></param>
/// <param name="j"></param>
/// <returns></returns>
int getstateregex(int i, int j)
{
	int state[][17] = {
		//   [   ]   {   }   ,   |   (   )   ~  \d   *   +   ?   \   \^   \s  ^
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // empty			0
			{1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1}, // error			1
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // character		2
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9, 10,  2,  2}, // mCharsStart	3
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // mCharsEnd		4
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // Or				5
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // Star			6
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // GroupS			7
			{3,  4, 15,  2,  2,  5,  7,  8, 12,  2,  6, 11,  14,  9,  2,  2,  2}, // GroupE			8
			{10,10, 15, 10, 10, 10, 10, 10,  2,  2, 10, 10,  10,  9,  2, 10, 10}, // BackSlash		9
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // escapechar		10
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // Plus			11
			{3,  4, 15,  2,  2,  5,  7,  8,  2, 13,  6, 11,  14,  9,  2,  2,  2}, // Action			12
			{3,  4, 15,  2,  2,  5,  7,  8,  2, 13,  6, 11,  14,  9,  2,  2,  2}, // Act Index		13
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // Question		14
			{1,  1,  1,  1,  1,  1,  1,  1,  1, 16,  1,  1,   1,  1,  1,  1,  1}, // CurlyOpen		15
			{1,  1,  1,  1, 17,  1,  1,  1,  1, 16,  1,  1,   1,  1,  1,  1,  1}, // MinLength		16
			{1,  1,  1,  1,  1,  1,  1,  1,  1, 18,  1,  1,   1,  1,  1,  1,  1}, // LengthSep		17
			{1,  1,  1, 19,  1,  1,  1,  1,  1, 18,  1,  1,   1,  1,  1,  1,  1}, // MaxLength		18
			{3,  4, 15,  2,  2,  5,  7,  8,  2,  2,  6, 11,  14,  9,  2,  2,  2}, // CurlyClose		19
	};
	return (state[i][j]);
}

RegexParser::RegexParser(std::string regex, std::vector<void(*)(std::string)> actions) : Automata(getstateregex)
{
	innit();
	Actions = actions;
	str = regex;
	evaluate();
	std::vector<RegexState*> toBeFree;
	root = root->Simplify(toBeFree);
}

RegexParser::RegexParser(std::string regex) : Automata(getstateregex)
{
	innit();
	str = regex;
	evaluate();
	std::vector<RegexState*> toBeFree;
	root = root->Simplify(toBeFree);
}

void RegexParser::character()
{
	if (groups.size() > 0)
		groups.back()->AddChar(str[c_idx]);
	else
		root->AddChar(str[c_idx]);
}

void RegexParser::errorfound()
{
	throw std::exception();
}

void RegexParser::orfound()
{
	if (groups.size() > 0)
		groups.back()->AddState(new AndRState(groups.back()->lvl + 1));
	else
		root->AddState(new AndRState(root->lvl + 1));
}

void RegexParser::setstart()
{
	groups.push_back(new MultiCharState(1));
}

void RegexParser::setend()
{
	RegexState* group = groups.back();
	groups.pop_back();
	if (groups.size() > 0)
		groups.back()->AddState(group);
	else
		root->AddState(group);
}

void RegexParser::star()
{
	if (groups.size() > 0)
		groups.back()->AddQuantity(0, INT32_MAX);
	else
		root->SetQuantity(0, INT32_MAX);
}

void RegexParser::plus()
{
	if (groups.size() > 0)
		groups.back()->AddQuantity(1, INT32_MAX);
	else
		root->SetQuantity(1, INT32_MAX);
}

void RegexParser::question()
{
	if (groups.size() > 0)
		groups.back()->AddQuantity(0, 1);
	else
		root->SetQuantity(0, 1);
}

void RegexParser::groupstart()
{
	groups.push_back(new GroupRState());
}

void RegexParser::groupending()
{
	LastGroup = static_cast<GroupRState*>(groups.back());
	groups.pop_back();
	if (groups.size() > 0)
		groups.back()->AddState(LastGroup);
	else
		root->AddState(LastGroup);
}

void RegexParser::minmaxlength()
{
	std::string strn = str.substr(o_idx + 1, c_idx - o_idx - 1);
	int minlength = std::atoi(strn.c_str());
	int maxlength = std::atoi(strn.c_str() + strn.find(',') + 1);
	std::cout << "Se ha encontrado una longitud de validación: " << strn << " min: " << minlength << " max: " << maxlength << std::endl;
	if (groups.size() > 0)
		groups.back()->AddQuantity(minlength, maxlength);
	else
		root->SetQuantity(minlength, maxlength);
}

MultiCharState* PreSets(std::string chars, bool negate = false)
{
	MultiCharState* mchar = new MultiCharState(1);
	mchar->chars = chars;
	mchar->negate = negate;
	return mchar;
}

/// <summary>
/// Se utiliza para indicar el comportamiento luego de un '\'.
/// </summary>
void RegexParser::backslash()
{
	RegexState* target = groups.size() > 0 ? groups.back() : root;
	if (str[c_idx] == 's')
		target->AddState(PreSets("\n \r\t"));
	else if (str[c_idx] == 'w')
		target->AddState(PreSets("aeiouAEIOU_bcdfghjklmnpqrstvxwyzBCDFGHJKLMNPQRSTVXWYZ"));
	else if (str[c_idx] == 'd')
		target->AddState(PreSets("0123456789"));
	else
		target->AddChar(str[c_idx]);
}

void RegexParser::negate()
{
	RegexState *target = groups.size() > 0 ? groups.back() : root;
	MultiCharState *mchars = dynamic_cast<MultiCharState*>(target);
	if (mchars)
		mchars->negate = true;
}

void RegexParser::move()
{
	o_idx = c_idx;
}

void RegexParser::action()
{
	std::string strn = str.substr(o_idx + 1, c_idx - o_idx);
	int index = std::atoi(strn.c_str());
	if (index >= 0 && index < static_cast<int>(Actions.size()))
	{
		std::cout << "Añadiendo acción idx: " << index << std::endl;
		LastGroup->AddAction(Actions[index]);
	}
}

bool RegexParser::Match(std::string str)
{
	size_t pos = 0;
	std::vector<std::string> matches;
	root->Matches(str, pos, matches);
	return pos == str.size();
}

Result RegexParser::StaticMatch(std::string str, int fd)
{
	size_t pos = 0;
	return root->StaticMatch(str, pos, records[fd]);
}

std::vector<std::string> RegexParser::Search(std::string str)
{
	size_t pos = 0;
	size_t tpos = 0;
	std::vector<std::string> matches;
	while (pos < str.length())
	{
		tpos = pos;
		bool found = root->Matches(str, tpos, matches);
		if (found)
			pos = pos == tpos ? pos + 1 : tpos;
		else
			pos++;
	}
	return matches;
}

int RegexParser::evaluate()
{
	this->o_idx = 0;
	this->c_idx = -1;

	while (++c_idx < (int)str.length())
	{
		this->state = this->get_state(this->state, alphabet_idx(this->alphabet, this->str[this->c_idx]));
		if (fta[ostate][state] != NULL)
			(*this.*fta[ostate][state])();
		if (fsa[state] != NULL)
			(*this.*fsa[state])();
		this->ostate = this->state;
	}
	return (this->state);
}