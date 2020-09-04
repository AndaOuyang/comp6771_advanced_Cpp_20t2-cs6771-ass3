#ifndef GDWG_GRAPH_HPP
#define GDWG_GRAPH_HPP

#include <concepts/concepts.hpp>
#include <fmt/format.h>
#include <initializer_list>
#include <memory>
#include <ostream>
#include <range/v3/algorithm.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/iterator.hpp>
#include <range/v3/utility.hpp>
#include <range/v3/view.hpp>
#include <set>
#include <type_traits>
#include <typeinfo>

namespace gdwg {
	template<concepts::regular N, concepts::regular E>
	requires concepts::totally_ordered<N> //
	   and concepts::totally_ordered<E> //
	   class graph {
	private:
		// struct edge_type = {ptr_src, ptr_dst, ptr_weight}
		// edge_type is totally ordered, by comparing src, then dst, then weight
		struct edge_type;
		template<concepts::regular T>
		requires concepts::totally_ordered<T> struct compare_ptr_by_content;

	public:
		class iterator;

		struct value_type {
			N from;
			N to;
			E weight;
		};

		graph() noexcept = default;

		graph(std::initializer_list<N> il) noexcept
		: graph(il.begin(), il.end()) {}

		template<ranges::forward_iterator I, ranges::sentinel_for<I> S>
		requires ranges::indirectly_copyable<I, N*> graph(I first, S last) {
			ranges::for_each(first, last, [this](N const& n) {
				auto smart_ptr = std::make_shared<N>(static_cast<N>(n));
				nodes_.emplace(smart_ptr);
			});
		}
		template<ranges::forward_iterator I, ranges::sentinel_for<I> S>
		requires ranges::indirectly_copyable<I, value_type*> graph(I first, S last) {
			ranges::for_each(first, last, [this](value_type const& v) {
				insert_node(v.from);
				insert_node(v.to);
				insert_edge(v.from, v.to, v.weight);
			});
		}
		graph(graph&& other) noexcept {
			nodes_ = std::move(other.nodes_);
			all_edges_ = std::move(other.all_edges_);
		}

		auto operator=(graph&& other) noexcept -> graph& {
			if (this == &other) {
				return *this;
			}
			nodes_ = std::move(other.nodes_);
			all_edges_ = std::move(other.all_edges_);
			return *this;
		}

		graph(graph const& other) {
			for (auto const& ptr_node : other.nodes_) {
				auto const& node = *ptr_node;
				insert_node(node);
			}
			for (auto const& edge : other.all_edges_) {
				auto const& src = *edge.src;
				auto const& dst = *edge.dst;
				auto const& weight = *edge.weight;
				insert_edge(src, dst, weight);
			}
		}

		auto operator=(graph const& other) -> graph& {
			if (this == &other or *this == other) {
				return *this;
			}
			clear();
			for (auto const ptr_node : other.nodes_) {
				auto const& node = *ptr_node;
				insert_node(node);
			}
			for (edge_type const edge : other.all_edges_) {
				auto const& src = *edge.src;
				auto const& dst = *edge.dst;
				auto const& weight = *edge.weight;
				insert_edge(src, dst, weight);
			}
			return *this;
		}

		//---------------------------- modifiers -----------------------------------------
		auto insert_node(N const& value) -> bool {
			if (is_node(value)) {
				return false;
			}
			nodes_.emplace(std::make_shared<N>(value));
			return true;
		}

		auto insert_edge(N const& src, N const& dst, E const& weight) -> bool {
			if (not is_node(src) or not is_node(dst)) {
				throw std::runtime_error("Cannot call gdwg::graph<N, E>::insert_edge when either src "
				                         "or dst node does not exist");
			}
			// initialize min_weight and max_weight for first edge, in case that <E> may not have a
			// default constructor
			if (all_edges_.empty()) {
				min_weight_ = weight;
				max_weight_ = weight;
			}
			// get the smart pointers of src and dst which are stored in the set
			auto& src_ptr = *nodes_.find(std::make_shared<N>(src));
			auto& dst_ptr = *nodes_.find(std::make_shared<N>(dst));
			auto weight_ptr = std::make_shared<E>(weight);
			auto new_edge = edge_type{src_ptr, dst_ptr, weight_ptr};

			// new_edge already exist ==> should return false
			if (all_edges_.count(new_edge)) {
				return false;
			}

			all_edges_.emplace(new_edge);
			// private helper function: update min_weight_ and max_weight
			update_weight_limits(weight);
			return true;
		}

		// insert new_data to nodes and then merge_replace_node(old_data, new_date)
		auto replace_node(N const& old_data, N const& new_data) -> bool {
			if (not is_node(old_data)) {
				throw std::runtime_error("Cannot call gdwg::graph<N, E>::replace_node on a node that "
				                         "doesn't exist");
			}
			if (is_node(new_data)) {
				return false;
			}
			auto ptr_new_node = std::make_shared<N>(new_data);
			nodes_.emplace(ptr_new_node);
			merge_replace_node(old_data, new_data);
			return true;
		}

		auto merge_replace_node(N const& old_data, N const& new_data) -> void {
			if (not is_node(old_data) or not is_node(new_data)) {
				throw std::runtime_error("Cannot call gdwg::graph<N, E>::merge_replace_node on old or "
				                         "new data if they don't exist in the graph");
			}
			if (old_data == new_data) {
				return;
			}
			auto ptr_old_node = *nodes_.find(std::make_shared<N>(old_data));
			auto ptr_new_node = *nodes_.find(std::make_shared<N>(new_data));
			nodes_.erase(ptr_old_node);

			// the code blow looks urgly, but I have no idea how to relace with range loop or
			// functions: Sometimes it = all_edges.erase(it) and sometimes ++it ==> to maintian O(N)
			// complexity <== erasing element of set from known iterator is armotized O(1)
			auto update_edge = [&old_data, &ptr_new_node](edge_type& edge) {
				if (*edge.src == old_data) {
					edge.src = ptr_new_node;
				}
				if (*edge.dst == old_data) {
					edge.dst = ptr_new_node;
				}
			};
			for (auto it = all_edges_.begin(); it != all_edges_.end();) {
				auto edge = *it;
				if (*edge.src == old_data or *edge.dst == old_data) {
					// I don't know how to replace range function because of this line
					// below: it = all_edges_.erase(it);
					it = all_edges_.erase(it);
					update_edge(edge);
					all_edges_.emplace(edge);
					continue;
				}
				++it;
			}
		}

		auto erase_node(N const& value) -> bool {
			if (not is_node(value)) {
				return false;
			}
			auto ptr_to_remove = *nodes_.find(std::make_shared<N>(value));
			nodes_.erase(ptr_to_remove);

			for (auto it = all_edges_.begin(); it != all_edges_.end();) {
				auto const& edge = *it;
				if (*edge.src == value or *edge.dst == value) {
					it = all_edges_.erase(it);
					continue;
				}
				++it;
			}
			return true;
		}

		// remove edge from set<edge_type> all_edges_ ==> O(log(e))
		auto erase_edge(N const& src, N const& dst, E const& weight) -> bool {
			if (not is_node(src) or not is_node(dst)) {
				throw std::runtime_error("Cannot call gdwg::graph<N, E>::erase_edge on src or dst if "
				                         "they don't exist in the graph");
			}
			auto ptr_src = std::make_shared<N>(src);
			auto ptr_dst = std::make_shared<N>(dst);
			auto edge = edge_type{ptr_src, ptr_dst, std::make_shared<E>(weight)};

			if (not all_edges_.count(edge)) { // O(log(e))
				return false;
			}
			all_edges_.erase(edge); // O(log(e))
			return true;
		}

		// erase from set<edge_type> all_edges_ using known iterator ==> amortized O(1)
		auto erase_edge(iterator i) -> iterator {
			auto edge_iter = get_inner(i); // read from iterator ==> O(1)
			auto edge_iter_returned = all_edges_.erase(edge_iter);
			return iterator(edge_iter_returned);
		}

		// erase from set<edge_type> all_edges_ using knwon iterator range: O(d)
		auto erase_edge(iterator i, iterator s) -> iterator {
			auto edge_iter_begin = get_inner(i); // read from iterator ==> O(1)
			auto edge_iter_end = get_inner(s); // read from iterator ==> O(1)
			auto edge_iter_returned = all_edges_.erase(edge_iter_begin, edge_iter_end);
			return iterator(edge_iter_returned);
		}

		// clear all nodes and edges
		auto clear() noexcept -> void {
			nodes_.clear();
			all_edges_.clear();
		}

		//-------------------------------- Accessors --------------------------------------------
		[[nodiscard]] auto is_node(N const& value) const -> bool {
			auto ptr_to_varify = std::make_shared<N>(value);
			return nodes_.count(ptr_to_varify);
		}

		[[nodiscard]] auto empty() const -> bool {
			return nodes_.empty();
		}

		// min_edge = {src, dst, min_weight}
		// max_edge = {src, dst, max_weight}
		// use lower bound to find whehter there is an edge >= min_edge.
		// if so and the found edge is within the range of [min_edge, max_edge],
		// then src and dst are connected
		[[nodiscard]] auto is_connected(N const& src, N const& dst) const -> bool {
			if (not is_node(src) or not is_node(dst)) {
				throw std::runtime_error("Cannot call gdwg::graph<N, E>::is_connected if src or dst "
				                         "node don't exist in the graph");
			}
			auto ptr_src = std::make_shared<N>(src);
			auto ptr_dst = std::make_shared<N>(dst);
			auto min_edge = edge_type{ptr_src, ptr_dst, std::make_shared<E>(min_weight_)};
			auto max_edge = edge_type{ptr_src, ptr_dst, std::make_shared<E>(max_weight_)};
			auto iter = all_edges_.lower_bound(min_edge);
			return iter != all_edges_.end() and *iter >= min_edge and *iter <= max_edge;
		}

		// inorder travelsal of set<ptr_N> ==> O(N) time complexity
		[[nodiscard]] auto nodes() const -> std::vector<N> {
			auto result = std::vector<N>{};
			ranges::transform(nodes_, ranges::back_inserter(result), [](auto const& ptr_node) {
				return *ptr_node;
			});
			return result;
		}

		// use lower_bound() and upper_bound() to find the begin iterator and end iterator of edges
		// from src to dst
		[[nodiscard]] auto weights(N const& src, N const& dst) const -> std::vector<E> {
			if (not is_node(src) or not is_node(dst)) {
				throw std::runtime_error("Cannot call gdwg::graph<N, E>::weights if src or dst node "
				                         "don't exist in the graph");
			}
			auto ptr_src = std::make_shared<N>(src);
			auto ptr_dst = std::make_shared<N>(dst);
			auto edge_from = edge_type{ptr_src, ptr_dst, std::make_shared<E>(min_weight_)};
			auto edge_to = edge_type{ptr_src, ptr_dst, std::make_shared<E>(max_weight_)};

			// all edges from src to dst must be within the range [edge_from, edge_to]
			// ==> use lower_bound and upper_bound to find the iterators
			auto iter_begin = all_edges_.lower_bound(edge_from);
			auto iter_end = all_edges_.upper_bound(edge_to);

			auto result = std::vector<E>{};
			ranges::transform(iter_begin,
			                  iter_end,
			                  ranges::back_inserter(result),
			                  [](edge_type const& edge) { return *edge.weight; });

			return result;
		}

		[[nodiscard]] auto find(N const& src, N const& dst, E const& weight) const -> iterator {
			auto edge = edge_type{std::make_shared<N>(src),
			                      std::make_shared<N>(dst),
			                      std::make_shared<E>(weight)};
			auto it_edge = all_edges_.find(edge);
			return iterator(it_edge);
		}

		[[nodiscard]] auto connections(N const& src) const -> std::vector<N> {
			if (not is_node(src)) {
				throw std::runtime_error("Cannot call gdwg::graph<N, E>::connections if src doesn't "
				                         "exist in the graph");
			}
			// can encapsulate the blow codes in finding the iter_from iter_to? no time to implement
			auto min_node = *nodes_.begin();
			auto max_node = *nodes_.rbegin();
			auto ptr_src = *nodes_.find(std::make_shared<N>(src));

			auto edge_from = edge_type{ptr_src, min_node, std::make_shared<E>(min_weight_)};
			auto edge_to = edge_type{ptr_src, max_node, std::make_shared<E>(max_weight_)};

			auto iter_from = all_edges_.lower_bound(edge_from);
			auto iter_to = all_edges_.upper_bound(edge_to);

			// put every dst into set. set can eleminate duplication by itself
			auto result = std::set<N>{};
			ranges::for_each(iter_from, iter_to, [&result](edge_type const& edge) {
				result.emplace(*edge.dst);
			});
			return result | ranges::to<std::vector>;
		}

		//--------------------------------- range access ------------------------------------
		[[nodiscard]] auto begin() const -> iterator {
			return iterator(all_edges_.begin());
		}

		[[nodiscard]] auto end() const -> iterator {
			return iterator(all_edges_.end());
		}

		// ------------------------------ comparisons --------------------------------------

		// check address equal, then check number of nodes and edges, then check values
		[[nodiscard]] auto operator==(graph const& other) const -> bool {
			if (this == &other) {
				return true;
			}
			// number of edges or number of nodes not equal ==> return false
			if (this->all_edges_.size() != other.all_edges_.size()
			    || this->nodes_.size() != other.nodes_.size())
			{
				return false;
			}
			// return true iff all nodes and edges are equal
			auto pred_node = [](std::shared_ptr<N> const& a, std::shared_ptr<N> const& b) {
				return *a == *b;
			};
			if (not ranges::equal(nodes_, other.nodes_, pred_node)) {
				return false;
			}
			return ranges::equal(all_edges_, other.all_edges_);
		}
		// ------------------------------ extractor ----------------------------------------
		friend auto operator<<(std::ostream& os, graph const& g) -> std::ostream& {
			if (g.empty()) {
				return os;
			}
			auto it_begin = g.all_edges_.begin();
			auto it_end = g.all_edges_.begin();

			for (auto const ptr_node : g.nodes_) {
				auto const& node = *ptr_node;
				os << node << " (\n";
				// it_begin: the first iter such that *iter->src >= node
				// it_end:   the first iter such that *iter->src >  node
				// if current node has connection, then print the range of [it_begin, iter_end)
				// if current node has no connection, then it_begin == it_end: src > node, range
				// [it_begin, iter_end) is empty
				it_begin = ranges::find_if(it_begin, g.all_edges_.end(), [&node](edge_type const& edge) {
					return *edge.src >= node;
				});
				it_end = ranges::find_if(it_begin, g.all_edges_.end(), [&node](edge_type const& edge) {
					return *edge.src > node;
				});
				ranges::for_each(it_begin, it_end, [&os](edge_type const& edge) {
					os << fmt::format("  {} | {}\n", *(edge.dst), *(edge.weight));
				});
				os << ")\n";
			}
			return os;
		}
		// ------------------------------ Iterator -----------------------------------------
		class iterator {
		private:
			using inner_iter_type = typename std::set<edge_type>::const_iterator;
			using result_type = ranges::common_tuple<N const&, N const&, E const&>;

		public:
			using value_type = ranges::common_tuple<N, N, E>;
			using difference_type = std::ptrdiff_t;
			using iterator_category = std::bidirectional_iterator_tag;

			// Iterator constructor
			iterator() = default;

			explicit iterator(inner_iter_type inner)
			: inner_{inner} {}
			// Iterator source
			auto operator*() const -> ranges::common_tuple<N const&, N const&, E const&> {
				auto const& edge = *inner_;
				return result_type{*edge.src, *edge.dst, *edge.weight};
			}
			// Iterator traversal
			auto operator++() -> iterator& {
				++inner_;
				return *this;
			}
			auto operator++(int) -> iterator {
				auto temp = *this;
				++*this;
				return temp;
			}
			auto operator--() -> iterator& {
				--inner_;
				return *this;
			}
			auto operator--(int) -> iterator {
				auto temp = *this;
				--*this;
				return temp;
			}

			// Iterator comparison
			auto operator==(iterator const& other) const -> bool {
				return this->inner_ == other.inner_;
			}

		private:
			friend auto graph::get_inner(iterator& i) -> inner_iter_type;
			// explicit iterator(unspecified);
			inner_iter_type inner_;
		};

		// Your member functions go here
	private:
		template<concepts::regular T>
		requires concepts::totally_ordered<T> struct compare_ptr_by_content {
			auto operator()(std::shared_ptr<T> const& a, std::shared_ptr<T> const& b) const noexcept
			   -> bool {
				return *a < *b;
			}
		};

		// struct edge_type = {ptr_src, ptr_dst, ptr_weight}
		// edge_type is totally ordered, by comparing src, then dst, then weight
		struct edge_type {
			std::shared_ptr<N> src;
			std::shared_ptr<N> dst;
			std::shared_ptr<E> weight;

			// make edge totally ordered
			friend auto operator==(edge_type const& a, edge_type const& b) -> bool {
				return *a.src == *b.src and *a.dst == *b.dst and *a.weight == *b.weight;
			}
			friend auto operator<(edge_type const& a, edge_type const& b) -> bool {
				if (*a.src == *b.src) {
					if (*a.dst == *b.dst) {
						return *a.weight < *b.weight;
					}
					return *a.dst < *b.dst;
				}
				return *a.src < *b.src;
			}
			friend auto operator<=(edge_type const& a, edge_type const& b) -> bool {
				return (a < b or a == b);
			}
			friend auto operator>(edge_type const& a, edge_type const& b) -> bool {
				return (b < a);
			}
			friend auto operator>=(edge_type const& a, edge_type const& b) -> bool {
				return (b < a or b == a);
			}
		};

		// access the inner pointer of iterator: inner pointer = pointer to this.all_edges_
		auto get_inner(iterator& i) -> typename std::set<edge_type>::const_iterator {
			return i.inner_;
		}
		// helper function to maintain max_weight_ and min_weight_ at every insertion of edge
		auto update_weight_limits(E const& weight) -> void {
			if (weight > max_weight_) {
				max_weight_ = weight;
			}
			if (weight < min_weight_) {
				min_weight_ = weight;
			}
		}

		std::set<edge_type> all_edges_;

		std::set<std::shared_ptr<N>, compare_ptr_by_content<N>> nodes_;

		// below are min and max weight limits ever occured
		// so that I can find connection by all_edges_.lower_bound({src, dst, min_weight})

		// only update in edge insertion not deletion. because after deletion,
		// (min_weight_ <= every_weight <= max_weight) still holds, no need to update
		E min_weight_;
		E max_weight_;

	}; // namespace gdwg
} // namespace gdwg

#endif // GDWG_GRAPH_HPP
