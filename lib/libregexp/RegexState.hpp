	#ifndef REGEXSTATE_HPP
	#define REGEXSTATE_HPP

	# include <stdexcept>
	# include <iostream>
	# include <iomanip>
	# include <utility>
	# include <vector>
	# include <string>
	# include <map>
	# include <stack>
	# include <memory>
	# include <stdint.h>
	# include <cstdlib>

	// Añadido para el proyecto WebServ 42, permite serializar la posición de validación.
	class CurrentState
	{
		public:
			int fd;
			size_t cstate;
			size_t start;
			size_t cant;
	};

	// Añadido para el proyecto WebServ 42
	enum Result {
		REQUEST_ERROR,		// La cadena no cumple con la expresión regular.
		REQUEST_SUCCESS,	// La cadena se ha finalizado correctamente.
		REQUEST_CONTINUE,	// La cadena ha llegado a su fin, pero hasta este punto se cumple la expresión regular.
	};

	class RegexState
	{
		public:
			int lvl;		// Uso para debug.
			size_t min;		// Cantidad mínima necesaria para validar este nodo.
			size_t max;		// Cantidad máxima de validaciones a este nodo.
			virtual ~RegexState();
			virtual RegexState* Simplify(std::vector<RegexState*> &) { return this; };
			virtual void PrintChild() { std::cout << "Nada" << std::endl; };
			virtual void AddChar(char c) { (void)c; };
			virtual void AddAction(void (*f)(std::string)) { (void)f; };
			virtual void AddState(RegexState *state) { (void)state; };
			virtual void AddQuantity(int min, int max);
			virtual void SetQuantity(int min, int max);
			virtual bool Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches);
			virtual Result StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record) { (void)input; (void)pos;  (void)record; return REQUEST_ERROR; };
			virtual void GetProgress(std::map<RegexState*, CurrentState>& record, int pos, size_t& start, size_t& cstate, size_t& cant);
			virtual void SetProgress(std::map<RegexState*, CurrentState>& record, size_t& start, size_t& cstate, size_t& cant);

		protected:
			RegexState();
	};

	/// <summary>
	///  Rama del árbol, verifica que todos los nodos hijos sean validos.
	/// </summary>
	class AndRState : public RegexState
	{
		public:
			bool print;
			size_t cstate;
			std::vector<RegexState*> states;
			AndRState(int lvl);
			AndRState(AndRState& other);
			~AndRState();
			RegexState *Simplify(std::vector<RegexState*>& toBeFree);
			void AddChar(char c);
			void AddState(RegexState* state);
			void AddAction(void (*f)(std::string));
			void AddQuantity(int min, int max);
			void SetQuantity(int min, int max);
			bool Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches);
			Result StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record);
			void PrintChild();
	};

	/// <summary>
	///  Hoja del árbol, valida un caracter.
	/// </summary>
	class CharRState : public RegexState
	{
		public:
			CharRState(char c, int lvl);
			char character;
			bool Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches);
			Result StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record);
			void PrintChild();
	};

	/// <summary>
	///  Rama del árbol y nodo inicial del Regex, por simplicidad añade un AndRState al iniciarse. 
	/// </summary>
	class OrRState : public RegexState
	{
		public:
			OrRState(int lvl);
			bool print;
			size_t cstate;
			std::vector<RegexState*> states;
			void AddChar(char c);
			void AddState(RegexState* state);
			void AddAction(void (*f)(std::string));
			void AddQuantity(int min, int max);
			void SetQuantity(int min, int max);
			bool Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches);
			Result StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record);
			RegexState* Simplify(std::vector<RegexState*>& toBeFree);
			void PrintChild();
	};

	/// <summary>
	///  Hoja del árbol, valida utilizando un set de caracteres.
	/// </summary>
	class MultiCharState : public RegexState
	{
		public:
			bool		negate;
			std::string chars;
			MultiCharState(int lvl);
			void AddChar(char c);
			void AddState(RegexState* state);
			bool Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches);
			Result StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record);
			void PrintChild();
	};

	/// <summary>
	///  Funciona como una raíz del árbol, permite añadir cuantificadores a un grupo de nodos.
	/// </summary>
	class GroupRState : public RegexState
	{
		private:
			void (*OnValidate)(std::string);
		public:
			RegexState *conditions;
			GroupRState();
			void AddChar(char c);
			void AddAction(void (*f)(std::string));
			void AddQuantity(int min, int max);
			void AddState(RegexState* state);
			bool Matches(const std::string& input, size_t& pos, std::vector<std::string>& matches);
			Result StaticMatch(const std::string& input, size_t& pos, std::map<RegexState*, CurrentState>& record);
			RegexState* Simplify(std::vector<RegexState*>& toBeFree);
			void PrintChild();
	};
	#endif