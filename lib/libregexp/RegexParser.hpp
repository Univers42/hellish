#ifndef REGEXPARSER_HPP
# define REGEXPARSER_HPP

# include "RegexState.hpp"
# include "Automata.hpp"
# include <vector>
# include <stack>

/// <summary>
/// Automata to analize a regular expression
/// </summary>
class RegexParser : public Automata
{
	private:
		GroupRState* LastGroup;
		RegexState* root;
		std::vector<RegexState*> groups;
		std::map<int, std::map<RegexState*, CurrentState>> records;
		void innit();
		void character();
		void errorfound();
		void orfound();
		void setstart();
		void setend();
		void star();
		void plus();
		void question();
		void groupstart();
		void groupending();
		void minmaxlength();
		void backslash();
		void negate();
		void move();
		void action();
		void (RegexParser::* fsa[20])(void);		// Function state action
		void (RegexParser::* fta[20][20])(void);	// Function state transition

	public:
		std::vector<void (*)(std::string)> Actions;
		bool Match(std::string str);
		Result StaticMatch(std::string str, int fd);
		std::vector<std::string> Search(std::string str);
		RegexParser(std::string regex, std::vector<void(*)(std::string)> actions);
		RegexParser(std::string regex);
		virtual ~RegexParser() {}
		int evaluate();
};
#endif