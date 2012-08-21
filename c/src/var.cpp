#include "var.h"

Var::Var()
{
	this->id = max_id;
	std::stringstream ss;
	ss << max_id;
	this->name = ss.str();
	Var::max_id++;
	if(!Var::vars)
		Var::vars = new std::map<std::string, int>();
	Var::vars->insert(Var::vars->begin(), std::pair<std::string,int>(this->name, this->id));
}

// if there is already a variable with this name, return a reference to this variable
Var::Var(std::string name)
{
	if(!Var::vars)
			Var::vars = new std::map<std::string, int>();
	std::map<std::string, int>::const_iterator v = Var::vars->find(name);
	this->name = name;
	if(v != Var::vars->end()) // variable already exists
	{
		this->id = v->second;
	}
	else
	{
		this->id = max_id;
		Var::max_id++;

		Var::vars->insert(Var::vars->begin(), std::pair<std::string,int>(this->name, this->id));
	}
}

bool Var::operator==(const Var& var) const
{
	return this->id == var.id;
}

bool Var::operator!=(const Var& var) const
{
	return this->id != var.id;
}

bool Var::operator<(const Var& var) const
{
	return this->id<var.id;
}

std::string Var::string() const
{
	std::stringstream ss;
	ss << "Var(" << this->name << ")";
	return ss.str();
}

int Var::max_id = 0;
std::map<std::string, int>* Var::vars = 0;

std::ostream& operator<<(std::ostream& os, const Var var)
{
	return os << var.string();
}

std::ostream& operator<<(std::ostream& os, const std::multiset<Var> vars)
{
	std::stringstream ss;
	ss << "{";
	for(std::multiset<Var>::const_iterator var = vars.begin(); var != vars.end(); ++var)
	{
		if(var != vars.begin())
		{
			ss << ",";
		}
		ss << (*var).string();
	}
	ss << "}";
	return os << ss.str();
}

std::ostream& operator<<(std::ostream& os, const std::vector<Var>& vars)
{
	std::stringstream ss;
	ss << "{";
	for(std::vector<Var>::const_iterator var = vars.begin(); var != vars.end(); ++var)
	{
		if(var != vars.begin())
		{
			ss << ",";
		}
		ss << (*var).string();
	}
	ss << "}";
	return os << ss.str();
}
