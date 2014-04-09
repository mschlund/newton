#include <iostream>
#include "semilinSetNdd.h"

int static_i = 0;

// insert an offset if it is unique in offsets
std::vector<std::vector<int>>& insert_offset(std::vector<std::vector<int>>& offsets, const std::vector<int>& offset) {
  if(std::find(offsets.begin(), offsets.end(), offset) == offsets.end())
  {
    offsets.push_back(offset);
  }
  return offsets;
}

std::vector<std::vector<int>> multiply_offsets(const std::vector<std::vector<int>>& offsets1, const std::vector<std::vector<int>>& offsets2) {
  std::vector<std::vector<int>> offsets;
  for(auto o1 : offsets1) {
    for(auto o2 : offsets2) {
      assert(o1.size() == o2.size());
      std::vector<int> o(o1.size());
      for(int i=0; i<o1.size(); ++i) {
        o.at(i) = o1.at(i) + o2.at(i);
      }
      offsets = insert_offset(offsets, o);
    }
  }
  return offsets;
}

std::string serialize_offsets(const std::vector<std::vector<int>>& offsets) {
  std::stringstream result;
  result << "[";
  for(const auto& offset : offsets) {
    if(&offset != &offsets.at(0))
      result << ",";
    result << "(";
    for(const auto& o : offset) {
      if(&o != &offset.at(0))
        result << ",";
      result << o;
    }
    result << ")";
  }
  result << "]";
  return result.str();
}

std::vector<int> add_vectors(const std::vector<int>& vec1, const std::vector<int>& vec2) {
  assert(vec1.size() == vec2.size());
  std::vector<int> result(vec1.size());
  for(int i = 0; i < vec1.size(); ++i)
    result.at(i) = vec1.at(i) + vec2.at(i);
  return result;
}
std::vector<int> subtract_vectors(const std::vector<int>& vec1, const std::vector<int>& vec2) {
  assert(vec1.size() == vec2.size());
  std::vector<int> result(vec1.size());
  for(int i = 0; i < vec1.size(); ++i)
    result.at(i) = vec1.at(i) - vec2.at(i);
  return result;
}

// check if the given candidate is truly a generator starting at the given offset
// n is number of offsets
bool SemilinSetNdd::isGenerator(const std::vector<int>& offset, const std::vector<int>& candidate, int n) const {
  // try to sample enough points in the solution space
  // from the given offset to be sure this is a generator.
  // First guess is, n is enough for n > offsets.size()
  auto solution = offset; // start at offset
  for(int i = 0; i<=n; ++i) {
    // solution += candidate;
    for(int j = 0; j < candidate.size(); ++j)
      solution.at(j)+=candidate.at(j);

    if(!this->set.isSolution(solution))
      return false; // this is not a generator for this offset
  }
  return true; // we tried n+1 sample points, this should be a generator, TODO: prove this!
}

// test if offset 2 = offset1 + N×candidate
bool SemilinSetNdd::isGeneratorFor(const std::vector<int>& offset1, const std::vector<int>& candidate, const std::vector<int>& offset2) const {
  auto new_candidate = candidate;
  std::vector<int> test(candidate.size());
  int N = -1; // N should not be negative
  for(int i = 0; i < candidate.size(); ++i) {
    int tmp = offset2.at(i)-offset1.at(i);
    if(tmp == 0 && tmp == candidate.at(i))
      continue;
    int tmp2;
    if(candidate.at(i) != 0)
      tmp2 = tmp / candidate.at(i);
    else {
      std::cout << " not a valid generator for this offset at " << i << " c:" << candidate.at(i) << " tmp: " << tmp << " ";
      return false; // if candidate != tmp then this is not a valid generator for this offset
    }

    if(N == -1)
      N = tmp2;
    if(N != tmp2) {
      std::cout << " not linear dependend ";
      return false; // vectors are not linear dependent
    }
  }
  // we managed to get here without returning false,
  // there is an N for offset 2 = offset1 + N×candidate
  return true;
}

// PSEUDOCODE
//  for all offset1 in offsets:
//    for all offset2 in offsets:
//      let candidate_generator = offset2-offset1
//      if candidate_generator is positive:
//        if candidate_generator is generator at offset1:
//          if offset1 is already generated by original_offset:
//            let new_candidate_generator = offset2-original_offset
//            if new_candidate_generator is positive:
//              if new_candidate_generator is generator at original_offset:
//                → offset2 is created by original_offset
//              else:
//                → offset1 seems to be necessary offset (TODO)
//            else:
//              → no useful generator
//              → 
//          else:
//            → offset2 is created by offset1
//        else:
//          → offset2 is not created by candidate_generator at offset1
//          → continue
//      else:
//        → no useful generator
//        → continue
//    end for
//  end for
//
//  accumulate all offsets which can be used to create other offsets
//  accumulate all offsets which cannot be created by other offsets+generators

bool isPositiveNotNull(const std::vector<int>& offset) {
  int sum = 0;
  for(auto o : offset) {
    if(o < 0)
      return false;
    sum += o;
  }
  return sum > 0;
}

// return all unique offsets which are necessary
std::vector<std::vector<int>> SemilinSetNdd::getUniqueOffsets(const std::vector<std::vector<int>>& offsets) const {
  // we set the respective element to true if we can generate this offset with another one.
  std::vector<bool> generated(offsets.size(), false);
  std::map<int,int> generated_by;

  // TODO: optimize the number of candidates
  for(unsigned int i = 0; i < offsets.size(); ++i) {
    const auto& offset1 = offsets.at(i);
    for(unsigned int j = 0; j < offsets.size(); ++j) {
      const auto& offset2 = offsets.at(j);

      auto candidate_generator = subtract_vectors(offset2,offset1);
      if(isPositiveNotNull(candidate_generator)) {
        if(isGenerator(offset1, candidate_generator, offsets.size())) {
          // offset2 was indeed created by offset1+candidate
          // TODO: be careful! there might be another offset which created offset1...
          // but maybe this may not be the case because of the order in which we
          // traverse the offsets (sorted lexicographically)
          // INFO: we know the is a generator, which can create offset2 = offset1+candidate
          // std::cout << "offset: ";
          // for(const auto& o : offset1)
          //   std::cout << o;
          // std::cout << " + N×";
          // for(const auto& c : candidate_generator)
          //   std::cout << c;
          // std::cout << " = ";
          // for(const auto& o : offset2)
          //   std::cout << o;

          if(generated.at(i)) { // offset1 was already generated by some other offset+generator
            // std::cout << " offset 1 already generated. check if neccesary: → ";
            const auto& original_offset = offsets.at(generated_by.at(i));
            auto new_candidate_generator = subtract_vectors(offset2,original_offset);

            if(isPositiveNotNull(new_candidate_generator)) {
              if(isGenerator(original_offset, new_candidate_generator, offsets.size())) {
                // std::cout << "offset2 is created by original_offset: → ";
                generated_by[j] = generated_by.at(i);
                generated.at(j) = true;
                // TODO: still something left todo?
                // for(auto o : original_offset)
                //   std::cout << o;
              } else {
                // std::cout << "offset1 seems to be necessary offset → ";
                // TODO: what to do? → nothing as we do not know more than before - do not set generated_by
                generated_by[j] = i;
                generated.at(j) = true;
              }
            } else {
              // std::cout << "new generator candidate is no generator at original_offset → ";
              // offset2 is created by generator_candidate from offset1 but not from original_offset
              generated_by[j] = i;
              generated.at(j) = true;
            }

          } else {
            // std::cout << "offset2 is generated by candidate generator at offset1 → ";
            // TODO: we might need it 
            generated_by[j] = i;
            generated.at(j) = true;
          }
        } else {
          // std::cout << "nope candidate generator is not a generator for offset2 at offset1 → ";
          // TODO: what to do?
        }
      } else {
        // std::cout << "generator candidate is no generator at offset1";
      }
      // std::cout << std::endl << "--------------------------------" << std::endl;
    }
  }

  std::set<std::vector<int>> result;

  // accumulate all offsets which can be used to create other offsets
  for(const auto& i : generated_by) {
    const auto& offset = offsets.at(i.second);
    result.insert(offset);
  }

  // accumulate all offsets which cannot be created by other offsets+generators
  for(int i = 0; i < generated.size(); ++i) {
    if(!generated.at(i)) // this offset is not to be found in other linear sets
      result.insert(offsets.at(i));
  }

  std::vector<std::vector<int>> returned_result;
  returned_result.insert(returned_result.end(), result.begin(), result.end());
  return returned_result;
}

bool SemilinSetNdd::genepi_init(std::string pluginname, int number_of_variables)
{
  genepi_loader_init();
  genepi_loader_load_default_plugins();
  plugin = genepi_loader_get_plugin(pluginname.c_str());
  std::cout << "Plugin: " << genepi_plugin_get_name(plugin) << std::endl;
  // TODO: use of cache???
  solver = genepi_solver_create(plugin, 0, 0, 0);
  k = number_of_variables;
  return true; // TODO: return correct value!
}

bool SemilinSetNdd::genepi_dealloc() {
  genepi_solver_delete(solver);
  genepi_plugin_del_reference(plugin);
  genepi_loader_terminate();
  return true;
}

SemilinSetNdd::SemilinSetNdd() : set(Genepi(solver, k, false)) {
}

SemilinSetNdd::SemilinSetNdd(int zero) : set(Genepi(solver, k, false)) {
  assert(zero == 0);
}

SemilinSetNdd::SemilinSetNdd(VarId var) : SemilinSetNdd(var, 1) {
}

SemilinSetNdd::SemilinSetNdd(VarId var, int cnt) {
  auto v = var_map.find(var);
  if(v == var_map.end())
  {
    var_map.insert(std::make_pair(var, var_map.size()));
    v = var_map.find(var);
  }

  int position = v->second; // which position is this variable on?
  std::vector<int> alpha(k);
  alpha.at(position) = cnt;

  this->set = Genepi(solver, alpha, false);
  this->offsets.push_back(alpha);
}
SemilinSetNdd::SemilinSetNdd(Genepi set, std::vector<std::vector<int>> offsets) : set(set), offsets(offsets) {
}

SemilinSetNdd::SemilinSetNdd(const SemilinSetNdd& expr) : set(expr.set), offsets(expr.offsets) {
}

SemilinSetNdd::~SemilinSetNdd() {
}

SemilinSetNdd SemilinSetNdd::operator=(const SemilinSetNdd& term) {
  this->set = term.set;
  this->offsets = term.offsets;
  return *this;
}

SemilinSetNdd SemilinSetNdd::operator+=(const SemilinSetNdd& term) {
  this->set = this->set.union_op(term.set);
  for(auto offset : term.offsets)
    insert_offset(this->offsets, offset);
  return *this;
}

SemilinSetNdd SemilinSetNdd::operator*=(const SemilinSetNdd& term) {
  std::vector<int> sel_a(3*k, 1);
  std::vector<int> sel_b(3*k, 1);
  for(int i = 0; i < k; i++)
  {
    sel_a[3*i]     = 0;
    sel_b[3*i+1]   = 0;
  }

  // inverse projection from original dimensions to an extended version, which is intersected with the natural numbers
  Genepi a_ext(this->set.invproject(sel_a).intersect(Genepi(solver, 3*k, true)));
  Genepi b_ext(term.set.invproject(sel_b).intersect(Genepi(solver, 3*k, true)));

  std::vector<int> generic_alpha = {1, 1, -1}; // 1*a[i]+1*b[i]-1*c[i]=0
  Genepi generic_sum(solver, generic_alpha, 0);

  std::vector<int> component_selection(3*k, 1); // TODO: maybe we only use k+2 or so elements
  std::vector<int> component_projection(3*k, 0); // we want to project away component 3i and 3i+1

  // initialize the result automaton with (a_i,b_i,N) for i in 0..k-1
  Genepi result(solver, 3*k, true); // natural numbers
  result = result.intersect(a_ext).intersect(b_ext);

  // one run for each variable
  // at the end of each loop run we delete the used a and b component
  // the new sum component will be at position i at the end of the loop run
  // (a,b,a+b) component relevant for current run is at position (i,i+1,i+2) at the begin of a run
  for(int i = 0; i < k; i++)
  {
    component_selection.resize(3*k-2*i);
    component_projection.resize(3*k-2*i);
    // TODO: figure out, if precalculating this and apply inv_project has better complexity
    // than always create a new sum automaton for different components
    component_selection[i]   = 0;
    component_selection[i+1] = 0;
    component_selection[i+2] = 0;
    // inverse projection on generic sum automaton to create automaton for component i
    Genepi component_sum(generic_sum.invproject(component_selection));
    // use this sum automaton on the intermediate result
    result = result.intersect(component_sum);

    component_selection[i]   = 1; // reset component_selection
    component_selection[i+1] = 1;
    component_selection[i+2] = 1;

    component_projection[i]   = 1; // project away the already used a and b component at position (i,i+1)
    component_projection[i+1] = 1;
    result = result.project(component_projection);
    component_projection[i]   = 0;
    component_projection[i+1] = 0;
  }
  this->set = result;
  this->offsets = this->getUniqueOffsets(multiply_offsets(this->offsets, term.offsets));
  return *this;

}

bool SemilinSetNdd::operator < (const SemilinSetNdd& term) const {
  return this->set < term.set;
}
bool SemilinSetNdd::operator == (const SemilinSetNdd& term) const {
  return this->set == term.set;
}
SemilinSetNdd SemilinSetNdd::star () const {
  SemilinSetNdd offset_star = one();
  for(auto offset : this->offsets) {
    offset_star *= SemilinSetNdd(Genepi(this->solver, offset, true),{std::vector<int>(k,0)});
  }

  SemilinSetNdd result = one(); // result = 1

  SemilinSetNdd temp = one();
  for(int i = 1; i <= k; i++) { // 1..k
    temp *= *this; // temp = S^i
    result += temp;
  }
  result = one() + (result * offset_star); // S^i * offset_star
  return result;
}

SemilinSetNdd SemilinSetNdd::null() {
  if(!SemilinSetNdd::elem_null)
    SemilinSetNdd::elem_null = std::shared_ptr<SemilinSetNdd>(new SemilinSetNdd());
  return *SemilinSetNdd::elem_null;
}

SemilinSetNdd SemilinSetNdd::one() {
  if(!SemilinSetNdd::elem_one)
    SemilinSetNdd::elem_one = std::shared_ptr<SemilinSetNdd>(new SemilinSetNdd(Genepi(solver, std::vector<int>(k,0), false), {std::vector<int>(k,0)}));
  return *SemilinSetNdd::elem_one;
}


std::string SemilinSetNdd::string() const {
  std::stringstream result;
  auto calculated_offsets = this->offsets;
  result << "calculated offsets:\t\t" << serialize_offsets(calculated_offsets) << std::endl;
  auto cleaned_calculated_offsets = this->getUniqueOffsets(calculated_offsets);
  result << "calculated offsets (clean):\t" << serialize_offsets(cleaned_calculated_offsets) << std::endl;
  result << "automaton written:\t" << this->set.output("result", static_i++, "") << std::endl;
  return result.str();
}

const bool SemilinSetNdd::is_idempotent = true;
const bool SemilinSetNdd::is_commutative = true;
std::shared_ptr<SemilinSetNdd> SemilinSetNdd::elem_null;
std::shared_ptr<SemilinSetNdd> SemilinSetNdd::elem_one;
genepi_solver* SemilinSetNdd::solver;
genepi_plugin* SemilinSetNdd::plugin;
std::unordered_map<VarId, int> SemilinSetNdd::var_map;
int SemilinSetNdd::k = 0;
