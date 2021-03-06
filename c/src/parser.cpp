#include <vector>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/std_pair.hpp>


#include "parser.h"

Parser::Parser() {}

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

struct rexp_var_impl
{
        template <typename T>
        struct result { typedef CommutativeRExp type; }; // this tells Boost the return type

        const CommutativeRExp operator()(std::string& s) const
        {
                // create an element with the given var
                return CommutativeRExp(Var::GetVarId(s));
        }
};
const phx::function<rexp_var_impl> rexp_var;

struct free_var_impl
{
        template <typename T>
        struct result { typedef FreeSemiring type; }; // this tells Boost the return type

        const FreeSemiring operator()(std::string& s) const
        {
                // create an element with the given var
                return FreeSemiring(Var::GetVarId(s));
        }
};
const phx::function<free_var_impl> free_var;

struct prefix_impl
{
	template <typename T, typename U>
	struct result { typedef PrefixSemiring type; }; // this tells Boost the return type

	const PrefixSemiring operator()(std::vector<VarId>& p, unsigned int length) const
	{
		// create a prefix with the given vars
		return PrefixSemiring(p, length);
	}
};
const phx::function<prefix_impl> prefix;

#ifdef USE_GENEPI
struct slsetndd_var_impl
{
	template <typename T, typename U>
	struct result { typedef SemilinSetNdd type; }; // this tells Boost the return type

	const SemilinSetNdd operator()(std::string& s, unsigned int& cnt) const
	{
		// create an element with the given var
		return SemilinSetNdd(Var::GetVarId(s), cnt);
	}
};
const phx::function<slsetndd_var_impl> slsetndd_var;
#endif


struct var_impl
{
	template <typename T>
	struct result { typedef VarId type; }; // this tells Boost the return type

	const VarId operator()(std::string& var) const
	{
		return Var::GetVarId(var);
	}
};
const phx::function<var_impl> variable;


// some boost::qi methods for pushing values through our parser
using qi::_val;
using qi::_1;
using qi::_2;
using qi::_a;
using qi::lit;
using qi::lexeme;
using qi::eps;
using phx::ref; // must be used as phx::ref, otherwise g++ is confused with std::ref...
using phx::insert;
using phx::end;
using phx::begin;
using phx::ref;

typedef std::string::const_iterator iterator_type;

// parser for a commutative regular expression semiring element
struct rexp_elem_parser : qi::grammar<iterator_type, CommutativeRExp()>
{
	rexp_elem_parser() : rexp_elem_parser::base_type(elem)
	{
		elem = '"' >> qi::as_string[lexeme[+(ascii::char_ -'"')]] [_val = rexp_var(_1)] >> '"';
	}
	qi::rule<iterator_type, CommutativeRExp()> elem;
};

#ifdef USE_GENEPI
// parser for a semilinear set expression semiring element
// e.g.: "<a:2, b:5, c:7>" or "<a:1, b:2>" or "<>" (for the one-element)
struct slsetndd_elem_parser : qi::grammar<iterator_type, SemilinSetNdd(), qi::space_type>
{
	slsetndd_elem_parser() : slsetndd_elem_parser::base_type(elem)
	{
		// create new sl-set elements for each entry e.g. "a:2" and aggregate them by using multiplication!
		elem = qi::lit("\"<") >> eps [_val = SemilinSetNdd::one()] >>
				   var_cnt [_val = _val * _1] >>
				   *( (',' >> var_cnt) [_val = _val * _1] ) >> qi::lit(">\"")
		    |  qi::lit("\"<") >> eps [_val = SemilinSetNdd::one()] >> qi::lit(">\"");
		var_cnt = (varidentifier >> ':' >> qi::uint_) [_val = slsetndd_var(_1, _2)];
		varidentifier = qi::as_string[+(qi::alnum)];
	}
	qi::rule<iterator_type, SemilinSetNdd(), qi::space_type> elem;
	qi::rule<iterator_type, SemilinSetNdd(), qi::space_type> var_cnt;
	qi::rule<iterator_type, std::string()> varidentifier;
};
#endif

// parser for a free semiring element
struct free_elem_parser : qi::grammar<iterator_type, FreeSemiring()>
{
        free_elem_parser() : free_elem_parser::base_type(elem)
        {
          //quotes are optional, but if they are present they must surround the constant
          elem = (qi::as_string[lexeme[+(ascii::char_ - '"' - '|' - '<' - ';' - ' ')]] [_val = free_var(_1)])
               | ('"' >> qi::as_string[lexeme[+(ascii::char_ - '"')]] [_val = free_var(_1)] >> '"');

        }
        qi::rule<iterator_type, FreeSemiring()> elem;
};

// parser for a prefix semiring element
struct prefix_elem_parser : qi::grammar<iterator_type, PrefixSemiring()>
{
  unsigned int max_length;
        prefix_elem_parser() : prefix_elem_parser::base_type(elem)
        {
                elem = '"' >> (*var)[_val = prefix(_1,phx::ref(max_length))] >> '"';
                var = qi::as_string[lexeme[+(ascii::char_ - '"' - ',')]] [_val = variable(_1)] >> (',');
        }
        qi::rule<iterator_type, PrefixSemiring()> elem;
        qi::rule<iterator_type, VarId()> var;
};

// use the given parser and return a list of equations of SR polynomials
// the second parameter is either Polynomial<SR> or NonCommutativePolynomial<SR>
template <typename SR_Parser, typename PSR>
struct equation_parser : qi::grammar<iterator_type, std::vector<std::pair<VarId, PSR>>(), qi::space_type>
{
	equation_parser() : equation_parser::base_type(equations)
	{
		equations %= *(equation);
		equation %= (var >> lexeme["::="] >> polynomial >> ';');
		polynomial = summand [_val = _1] >> *('|' >> summand [_val = _val + _1]); // addition
		summand = eps [_val = PSR::one()] >> // set _val to one-element of the semiring
				*(
					('(' >> polynomial >> ')') [_val = _val * _1] | // group term
					var [_val = _val * _1] | // variable factor
					sr_elem [_val = _val * _1] // sr-elem factor
				 );
		var = varidentifier[_val = variable(_1)];
		varidentifier = qi::as_string[lexeme['<' >> +(ascii::char_ - '>') >> '>']];
	}
	qi::rule<iterator_type, std::vector<std::pair<VarId, PSR>>(), qi::space_type> equations;
	qi::rule<iterator_type, std::pair<VarId, PSR>(), qi::space_type> equation;
	qi::rule<iterator_type, PSR(), qi::space_type> polynomial;
	qi::rule<iterator_type, PSR(), qi::space_type> summand;
	qi::rule<iterator_type, std::string()> varidentifier;
	qi::rule<iterator_type, VarId()> var;
	SR_Parser sr_elem;
};

// non commutative function for using the parser with one int and the prefix semiring
std::vector<std::pair<VarId, NonCommutativePolynomial<PrefixSemiring>>> non_commutative_prefix_parser(std::string input, unsigned int length)
{
        typedef equation_parser<prefix_elem_parser, NonCommutativePolynomial<PrefixSemiring>> equation_parser;
        equation_parser equation;
        equation.sr_elem.max_length = length;

        iterator_type iter = input.begin();
        iterator_type end = input.end();

        std::vector<std::pair<VarId, NonCommutativePolynomial<PrefixSemiring>>> result;
        if(!(qi::phrase_parse(iter, end, equation, qi::space, result) && iter == end))
                std::cout << "bad input, failed at: " << std::string(iter, end) << std::endl;

        return result;
}

// non commutative generic function for using the parser
template <typename SR_Parser, typename SR> 
std::vector<std::pair<VarId, NonCommutativePolynomial<SR>>> non_commutative_parser(std::string input)
{
        typedef equation_parser<SR_Parser, NonCommutativePolynomial<SR>> equation_parser;
        equation_parser equation;

        iterator_type iter = input.begin();
        iterator_type end = input.end();

        std::vector<std::pair<VarId, NonCommutativePolynomial<SR>>> result;
        if(!(qi::phrase_parse(iter, end, equation, qi::space, result) && iter == end))
                std::cout << "bad input, failed at: " << std::string(iter, end) << std::endl;
        return result;
}

// commutative generic function for using the parser
template <typename SR_Parser, typename SR> 
std::vector<std::pair<VarId, CommutativePolynomial<SR>>> commutative_parser(std::string input)
{
	typedef equation_parser<SR_Parser, CommutativePolynomial<SR>> equation_parser;
	equation_parser equation;

	iterator_type iter = input.begin();
	iterator_type end = input.end();

	std::vector<std::pair<VarId, CommutativePolynomial<SR>>> result;
	if(!(qi::phrase_parse(iter, end, equation, qi::space, result) && iter == end))
		std::cout << "bad input, failed at: " << std::string(iter, end) << std::endl;

	return result;
}

// wrapper function for regular expression equations
std::vector<std::pair<VarId, CommutativePolynomial<CommutativeRExp>>> Parser::rexp_parser(std::string input)
{
	return commutative_parser<rexp_elem_parser, CommutativeRExp>(input);
}

#ifdef USE_GENEPI
// wrapper function for ndd semilinear set equations
std::vector<std::pair<VarId, CommutativePolynomial<SemilinSetNdd>>> Parser::slsetndd_parser(std::string input)
{
        return commutative_parser<slsetndd_elem_parser, SemilinSetNdd>(input);
}
#endif

// wrapper function for free semiring equations
std::vector<std::pair<VarId, NonCommutativePolynomial<FreeSemiring>>> Parser::free_parser(std::string input)
{
        return non_commutative_parser<free_elem_parser, FreeSemiring>(input);
}

// wrapper function for prefix semiring equations
std::vector<std::pair<VarId, NonCommutativePolynomial<PrefixSemiring>>> Parser::prefix_parser(std::string input, unsigned int length)
{
	return non_commutative_prefix_parser(input, length);
}
