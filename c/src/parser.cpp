#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/phoenix_function.hpp>

#include "parser.h"

Parser::Parser()
{
}

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

// this function object is used for star'ing elements from within the parser
// the templates achieve, that this can be used for every type (hopefully a semiring)
// TODO: enforce that T is subclass of Semiring!
struct star_impl
{
	template <typename T> struct result { typedef T type; }; // needed because this is how boost knows of what type the result will be
	
	template <typename T>
	T operator()(T& s) const // this function does the actual work
	{
		return s.star();
	}
};
const phx::function<star_impl> star;

struct free_var_impl
{
	template <typename T> 
	struct result { typedef FreeSemiring type; }; // needed because this is how boost knows of what type the result will be

	//template <typename T>
	const FreeSemiring operator()(std::string& s) const // this function does the actual work
	{
		// create an element with the given var
		return FreeSemiring(Var::getVar(s));
	}
};
const phx::function<free_var_impl> free_var;

struct rexp_var_impl
{
	template <typename T> 
	struct result { typedef CommutativeRExp type; }; // needed because this is how boost knows of what type the result will be

	//template <typename T>
	const CommutativeRExp operator()(std::string& s) const // this function does the actual work
	{
		// create an element with the given var
		return CommutativeRExp(Var::getVar(s));
	}
};
const phx::function<rexp_var_impl> rexp_var;

struct polyrexp_impl
{
	template <typename T,typename U> 
	struct result { typedef Polynomial<CommutativeRExp> type; }; // needed because this is how boost knows of what type the result will be

	//template <typename T>
	const Polynomial<CommutativeRExp> operator()(CommutativeRExp& s, std::vector<std::string>& v) const // this function does the actual work
	{
		// create an element with the given var
		std::vector<VarPtr> vars;
		for(auto it = v.begin(); it != v.end(); ++it)
			vars.push_back(Var::getVar(*it));
		return Polynomial<CommutativeRExp>({Monomial<CommutativeRExp>(s,vars)});
	}
};
const phx::function<polyrexp_impl> polyrexp;

// some boost::qi methods for pushing values through our parser 
using qi::_val;
using qi::_1;
using qi::_2;
using qi::lit;
using qi::lexeme;

template <typename Iterator>
struct float_parser : qi::grammar<Iterator, FloatSemiring()>
{
	// specify the parser rules, term is the start rule (and in this case the only rule)
	float_parser() : float_parser::base_type(expression)
	{
		// what does a float expression look like?
		expression = term [_val  = _1] >> *( '+' >> term [_val = _val + _1] );
		term = starfactor [_val = _1] >> *( 'x' >> starfactor [_val = _val * _1] );
		starfactor = factor [_val = _1] >> -(lit('*'))[_val = star(_val)];
		factor =
			qi::float_ [_val = _1] |
			'(' >> expression [_val = _1] >> ')';
	}

	// this declare the available rules and the respecting return types of our parser
	qi::rule<Iterator, FloatSemiring()> expression;
	qi::rule<Iterator, FloatSemiring()> term;
	qi::rule<Iterator, FloatSemiring()> starfactor;
	qi::rule<Iterator, FloatSemiring()> factor;

};

template <typename Iterator>
struct free_parser : qi::grammar<Iterator, FreeSemiring()>
{
	// specify the parser rules, term is the start rule (and in this case the only rule)
	free_parser() : free_parser::base_type(expression)
	{
		// what does a free semiring term look like?
		expression = term [_val  = _1] >> *( '+' >> term [_val = _val + _1] );
		term = starfactor [_val = _1] >> *( '.' >> starfactor [_val = _val * _1] );
		starfactor = factor [_val = _1] >> -(lit('*'))[_val = star(_val)];
		factor =
			qi::as_string[lexeme[+qi::alpha]] [_val = free_var(_1)] |
			'(' >> expression [_val = _1] >> ')';
	}

	// this declare the available rules and the respecting return types of our parser
	qi::rule<Iterator, FreeSemiring()> expression;
	qi::rule<Iterator, FreeSemiring()> term;
	qi::rule<Iterator, FreeSemiring()> starfactor;
	qi::rule<Iterator, FreeSemiring()> factor;
};

template <typename Iterator>
struct rexp_parser : qi::grammar<Iterator, CommutativeRExp()>
{
	// specify the parser rules, term is the start rule (and in this case the only rule)
	rexp_parser() : rexp_parser::base_type(expression)
	{
		// what does a commutative regular expression term look like?
		expression = term [_val  = _1] >> *( '+' >> term [_val = _val + _1] );
		term = starfactor [_val = _1] >> *( '.' >> starfactor [_val = _val * _1] );
		starfactor = factor [_val = _1] >> -(lit('*'))[_val = star(_val)];
		factor =
			qi::as_string[lexeme[+qi::lower]] [_val = rexp_var(_1)] |
			'(' >> expression [_val = _1] >> ')';
	}

	// this declare the available rules and the respecting return types of our parser
	qi::rule<Iterator, CommutativeRExp()> expression;
	qi::rule<Iterator, CommutativeRExp()> term;
	qi::rule<Iterator, CommutativeRExp()> starfactor;
	qi::rule<Iterator, CommutativeRExp()> factor;
};

template <typename Iterator>
struct polyrexp_parser : qi::grammar<Iterator, Polynomial<CommutativeRExp>()>
{
	// specify the parser rules, term is the start rule (and in this case the only rule)
	polyrexp_parser() : polyrexp_parser::base_type(expression)
	{
		// what does a polynomial in the commutative regular expression semiring look like?
		expression = term [_val  = _1] >> *( '+' >> term [_val = _val + _1] );
		term = ( rexp >> *( '.' >> var) )[_val = polyrexp(_1, _2)];
		var = +qi::as_string[lexeme[+qi::upper]] [_val = _1];
	}

	// this declare the available rules and the respecting return types of our parser
	qi::rule<Iterator, Polynomial<CommutativeRExp>()> expression;
	qi::rule<Iterator, Polynomial<CommutativeRExp>()> term;
	qi::rule<Iterator, std::string()> var;
	rexp_parser<Iterator> rexp;
};

typedef std::string::const_iterator iterator_type;

FloatSemiring Parser::parse_float(std::string input)
{
	typedef float_parser<iterator_type> float_parser;
	float_parser floater;
	
	iterator_type iter = input.begin();
	iterator_type end = input.end();

	FloatSemiring result;
	if(!(qi::parse(iter, end, floater, result) && iter == end))
		std::cout << "bad input, failed at: " << std::string(iter, end) << std::endl;

	return result;
}


FreeSemiring Parser::parse_free(std::string input)
{
	typedef free_parser<iterator_type> free_parser;
	free_parser freeer;
	
	iterator_type iter = input.begin();
	iterator_type end = input.end();

	FreeSemiring result;
	if(!(qi::parse(iter, end, freeer, result) && iter == end))
		std::cout << "bad input, failed at: " << std::string(iter, end) << std::endl;

	return result;
}

CommutativeRExp Parser::parse_rexp(std::string input)
{
	typedef rexp_parser<iterator_type> rexp_parser;
	rexp_parser rexper;
	
	iterator_type iter = input.begin();
	iterator_type end = input.end();

	CommutativeRExp result;
	if(!(qi::parse(iter, end, rexper, result) && iter == end))
		std::cout << "bad input, failed at: " << std::string(iter, end) << std::endl;

	return result;
}

Polynomial<CommutativeRExp> Parser::parse_polyrexp(std::string input)
{
	typedef polyrexp_parser<iterator_type> polyrexp_parser;
	polyrexp_parser polyrexper;
	
	iterator_type iter = input.begin();
	iterator_type end = input.end();

	Polynomial<CommutativeRExp> result;
	if(!(qi::parse(iter, end, polyrexper, result) && iter == end))
		std::cout << "bad input, failed at: " << std::string(iter, end) << std::endl;


	return result;
}
