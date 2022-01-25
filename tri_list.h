#ifndef TRI_LIST_H
#define TRI_LIST_H

#include <variant>
#include <vector>

// std::viewable_range, is_tri_list_correct
template <typename T1, typename T2, typename T3>
class tri_list {
  using elt_t = std::variant<T1, T2, T3>;
  std::vector<elt_t> buffer;

  template <typename T>
  concept EltType = 
    (std::same_as(T, T1) && !std::same_as(T, T2) && !std::same_as(T, T3))
    || (!std::same_as(T, T1) && std::same_as(T, T2) && !std::same_as(T, T3))
    || (!std::same_as(T, T1) && !std::same_as(T, T2) && std::same_as(T, T3));

public:
  tri_list() = default;
  tri_list(std::initializer_list<elt_t> elements) : buffer(elements);
  
  template <EltType T>
  void push_back(const T& t) { buffer.push_back(t); }
  
  template <EltType T, modifier<T> F>
  void modify_only(F m = F{});
  
  template <EltType T>
  void reset();
  
  template <EltType T>
  auto range_over();

  begin();
  end();
};

// spelnia concept: modifier
template <typename T>
T identity(T x) { return x; }

// 
// template <modifier
// compose

#endif // TRI_LIST_H
