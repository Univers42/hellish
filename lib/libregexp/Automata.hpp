#pragma once

#include <iostream>

class Automata
{
	protected:
	  int c_idx;				// Current index
	  int o_idx;				// Old index
	  void *data;				// Not used in this proyect
	  std::string *alphabet;	// Valid characters
	  int alphabet_length;		// Not used in this proyect
	  std::string *errors;		// Not used in this proyect
	  int errors_length;		// Not used in this proyect
	  int (Automata::*state_function) (int, int);
	  int alphabet_idx (std::string *alphabet, char c);	// Method to get state matrix index
	  void innit ();

	public:
	  Automata ();
	  Automata (void *data);
	  Automata (int (*get_state) (int, int));
	  ~Automata ();
	  std::string str;			// String to be analized
	  int state;				// Current state
	  int ostate;				// Old state
	  virtual int evaluate ();	// Start the state machine
	  int (*get_state) (int, int);
	  void set_string (const std::string &str);
	  void set_alphabet (size_t size, const std::string *strings);
	  void set_errors (size_t size, const std::string *strings);
	  void print_state ();
};