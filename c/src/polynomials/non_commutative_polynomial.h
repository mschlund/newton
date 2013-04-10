#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <list>
#include <map>
#include <string>

#include "../semirings/semiring.h"
#include "../semirings/free-semiring.h"

#include "../datastructs/matrix.h"
#include "../datastructs/var.h"
#include "../datastructs/var_degree_map.h"

#include "non_commutative_monomial.h"

/* FIXME: NonCommutativePolynomials are no semiring in our definition (not starable). */
template <typename SR>
class NonCommutativePolynomial : public Semiring<NonCommutativePolynomial<SR>,
                                                 Commutativity::NonCommutative,
                                                 SR::GetIdempotence()> {
private:
    std::map<NonCommutativeMonomial<SR>,std::uint_fast16_t> monomials_;

    static void InsertMonomial(
        std::map<NonCommutativeMonomial<SR>, std::uint_fast16_t> &monomials,
        NonCommutativeMonomial<SR> monomial,
        std::uint_fast16_t coeff)
    {
      auto iter = monomials.find(monomial);
      if(iter == monomials.end()) {
        monomials.insert({monomial, 1});
      } else {
        auto tmp = *iter;
        monomials.erase(iter);
        monomials.insert({tmp.first, tmp.second + coeff});
      }
    }

    static void InsertMonomial(
        std::map<NonCommutativeMonomial<SR>, std::uint_fast16_t> &monomials,
        std::vector<std::pair<elemType,int>> &&idx,
        std::vector<VarId> &&variables,
        std::vector<SR> &&srs
        )
    {
      auto tmp_monomial = NonCommutativeMonomial<SR>(std::move(idx), std::move(variables), std::move(srs));
      InsertMonomial(monomials, tmp_monomial, 1);
    };

  public:
    NonCommutativePolynomial() = default;

    NonCommutativePolynomial(const NonCommutativePolynomial &p) = default;
    NonCommutativePolynomial(NonCommutativePolynomial &&p) = default;

    /* Create a 'constant' polynomial. */
    NonCommutativePolynomial(const SR &elem) {
      InsertMonomial(monomials_, {{SemiringType, 0}}, {}, {elem});
    }
    NonCommutativePolynomial(SR &&elem) {
      InsertMonomial(monomials_, {{SemiringType, 0}}, {}, {elem});
    }

    /* Create a polynomial which consists only of one variable. */
    NonCommutativePolynomial(const VarId var) {
      InsertMonomial(monomials_, {{Variable, 0}}, {var}, {});
    }

    NonCommutativePolynomial<SR>& operator=(const NonCommutativePolynomial<SR> &p) = default;
    NonCommutativePolynomial<SR>& operator=(NonCommutativePolynomial<SR> &&p) = default;

    NonCommutativePolynomial<SR>& operator+=(const NonCommutativePolynomial<SR> &polynomial) {
      for (const auto &monomial : polynomial.monomials_) {
        // InsertMonomial handles monomials which are already in 'this' polynomial
        InsertMonomial(monomials_, monomial.first, monomial.second);
      }

      return *this;
    }

    NonCommutativePolynomial<SR>& operator+=(const NonCommutativeMonomial<SR> &monomial) {
      InsertMonomial(monomials_, monomial, 1);
      return *this;
    }

    NonCommutativePolynomial<SR>& operator+(const NonCommutativeMonomial<SR> &monomial) const {
      auto result = *this;
      result += monomial;
      return result;
    }

    NonCommutativePolynomial<SR>& operator*=(const NonCommutativePolynomial<SR> &rhs) {
      if (monomials_.empty()) {
        return *this;
      } else if (rhs.monomials_.empty()) {
        monomials_.clear();
        return *this;
      }

      std::map<NonCommutativeMonomial<SR>, std::uint_fast16_t> tmp_monomials;

      // iterate over both lhs und rhs
      for (const auto &lhs_monomial : monomials_) {
        for (const auto &rhs_monomial : rhs.monomials_) {
          auto tmp_monomial = lhs_monomial.first * rhs_monomial.first;
          auto tmp_coeff = lhs_monomial.second * rhs_monomial.second;

          InsertMonomial(tmp_monomials, tmp_monomial, tmp_coeff);
        }
      }
      monomials_ = std::move(tmp_monomials);
      return *this;
    }

    // multiplying a polynomial with a variable
    NonCommutativePolynomial<SR> operator*(const VarId &var) const {
      NonCommutativePolynomial result_polynomial;
      for (auto &monomial : monomials_) {
        InsertMonomial(result_polynomial.monomials_, monomial * var);
      }
      return result_polynomial;
    }

    friend NonCommutativePolynomial<SR> operator*(const SR &elem,
        const NonCommutativePolynomial<SR> &polynomial) {
      NonCommutativePolynomial result_polynomial;
      for (auto &monomial : polynomial.monomials_) {
        result_polynomial.monomials_ += monomial * elem;
      }
      return result_polynomial;
    }

    bool operator==(const NonCommutativePolynomial<SR> &polynomial) const {
      return monomials_ == polynomial.monomials_;
    }

    /* calculate the delta for this polynomial with the given data at iteration d
     * de2 is data from newton iterand for d-2
     * dl1 is data from newton update for d-1 */
    SR calculate_delta(
        const std::map<VarId, SR> &de2,
        const std::map<VarId, SR> &dl1) const {
      SR result = SR::null();
      for(auto const &monomial : monomials_) {
        result += monomial.first.calculate_delta(de2, dl1) * monomial.second;
      }

      return result;
    }

    /* calculates the derivative of this polynomial
     * return a schema and the used mapping from X to X[d-1]*/
    std::pair<NonCommutativePolynomial<SR>,std::map<VarId,VarId>> derivative() const {
      NonCommutativePolynomial<SR> result;

      /* get a mapping for all variables X s.t. X -> X[d-1]
       * X[d-1] has to be a fresh variable*/
      auto vars = get_variables();
      std::map<VarId, VarId> mapping;
      for(auto const &var : vars) {
        auto tmp = Var::GetVarId();
        mapping.insert({var, tmp});
      }

      /* use the mapping and sum up all derivatives of all monomials */
      for(auto const &monomial : monomials_) {
        result += monomial.first.derivative(mapping) * monomial.second;
      }

      return {result, mapping};
    }

    SR eval(const std::map<VarId, SR> &values) const {
      SR result = SR::null();
      for (const auto &monomial : monomials_) {
        result += monomial.eval(values);
      }
      return result;
    }

    /* Evaluate as much as possible (depending on the provided values) and
     * return the remaining (i.e., unevaluated) monomial. */
    NonCommutativePolynomial<SR> partial_eval(const std::map<VarId, SR> &values) const {
      NonCommutativePolynomial<SR> result = NonCommutativePolynomial<SR>::null();
      for(const auto &monomial : monomials_) {
        result.monomials_.insert(monomial.partial_eval(values));
      }
      return result;
    }

    /* Variable substitution. */
    NonCommutativePolynomial<SR> subst(const std::map<VarId, VarId> &mapping) const {
      NonCommutativePolynomial result_polynomial;

      for (const auto &monomial : monomials_) {
        result_polynomial.monomials_.insert({monomial.first.subst(mapping), monomial.second});
      }
      return result_polynomial;
    }

    static Matrix<SR> eval(const Matrix< NonCommutativePolynomial<SR> > &poly_matrix,
        const std::map<VarId, SR> &values) {
      const std::vector<NonCommutativePolynomial<SR>> &tmp_polynomials = poly_matrix.getElements();
      std::vector<SR> result;
      for (const auto &polynomial : tmp_polynomials) {
        result.emplace_back(polynomial.eval(values));
      }
      return Matrix<SR>{poly_matrix.getRows(), std::move(result)};
    }

    static Matrix<NonCommutativePolynomial<SR> > eval(Matrix<NonCommutativePolynomial<SR> > poly_matrix,
        std::map<VarId,NonCommutativePolynomial<SR> > values) {
      const std::vector<NonCommutativePolynomial<SR>> &tmp_polynomials = poly_matrix.getElements();
      std::vector<NonCommutativePolynomial<SR>> result;
      for (const auto &polynomial : tmp_polynomials) {
        result.emplace_back(polynomial.eval(values));
      }
      return Matrix<NonCommutativePolynomial<SR>>{poly_matrix.getRows(), result};
    }

    /* Convert this polynomial to an element of the free semiring.  Note that
     * the provided valuation might be modified with new elements. */
    FreeSemiring make_free(std::unordered_map<SR, VarId, SR> *valuation) const {
      assert(valuation);

      auto result = FreeSemiring::null();
      // convert this polynomial by adding all converted monomials
      for (const auto &monomial : monomials_) {
        result += monomial.second * monomial.first.make_free(valuation); // TODO: is this correct?
      }
      return result;
    }

    /* Same as make_free but for matrix form. */
    static Matrix<FreeSemiring> make_free(
        const Matrix< NonCommutativePolynomial<SR> > &poly_matrix,
        std::unordered_map<SR, VarId, SR> *valuation) {

        assert(valuation);

        const std::vector< NonCommutativePolynomial<SR> > &tmp_polynomials = poly_matrix.getElements();
        std::vector<FreeSemiring> result;

        for (const auto &polynomial : tmp_polynomials) {
          result.emplace_back(polynomial.make_free(valuation));
        }
        return Matrix<FreeSemiring>{poly_matrix.getRows(), std::move(result)};
    }

    Degree get_degree() {
      Degree degree = 0;
      for (auto &monomial : monomials_) {
        degree = std::max(degree, monomial.first.get_degree());
      }
      return degree;
    }

    // FIXME: get rid of this...
    // if you use this, make sure, you cache it...
    std::set<VarId> get_variables() const {
      std::set<VarId> vars;
      for(auto const &monomial : monomials_)
      {
        auto tmp = monomial.first.get_variables();
        vars.insert(tmp.begin(), tmp.end());
      }
      return vars;
    }

    // TODO: we cannot star polynomials!
    NonCommutativePolynomial<SR> star() const {
      assert(false);
      return (*this);
    }

    static NonCommutativePolynomial<SR> null() {
      return NonCommutativePolynomial<SR>{};
    }

    static NonCommutativePolynomial<SR> one() {
      return NonCommutativePolynomial<SR>{SR::one()};
    }

    static bool is_idempotent;
    static bool is_commutative;

    std::string string() const {
	    // TODO: implement this
      std::stringstream ss;
      for(auto monomial = monomials_.begin(); monomial != monomials_.end(); monomial++) {
        if(monomial != monomials_.begin())
          ss << " + ";
        ss << monomial->second << " * ";
        ss << monomial->first;
      }
      return ss.str();
    }

};

template <typename SR> bool NonCommutativePolynomial<SR>::is_commutative = false;
template <typename SR> bool NonCommutativePolynomial<SR>::is_idempotent = false;

/*template <typename SR>
std::ostream& operator<<(std::ostream& os, const std::map<VarId, SR>& values) {
  for (auto value = values.begin(); value != values.end(); ++value) {
    os << value->first << "→" << value->second << ";";
  }
  return os;
}*/


template <typename SR>
std::ostream& operator<<(std::ostream& os,
    const std::map<VarId, NonCommutativePolynomial<SR> >& values) {
  for (const auto &key_value : values) {
    os << key_value->first << "→" << key_value->second << ";";
  }
  return os;
}
