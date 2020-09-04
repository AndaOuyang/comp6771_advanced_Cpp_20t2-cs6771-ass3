#include "gdwg/graph.hpp"
#include <catch2/catch.hpp>
#include <memory>
#include <range/v3/range.hpp>

#include <range/v3/algorithm.hpp>
#include <range/v3/functional.hpp>
#include <range/v3/iterator.hpp>
#include <range/v3/range.hpp>
#include <range/v3/view.hpp>
#include <utility>

//-------------------------------------------------------------------------------------------------
//                          iterators range access begin() end()
//-------------------------------------------------------------------------------------------------
static_assert(ranges::bidirectional_iterator<gdwg::graph<int, int>::iterator>);
static_assert(ranges::bidirectional_range<gdwg::graph<int, int>>);
static_assert(ranges::bidirectional_range<gdwg::graph<int, int> const>);
static_assert(ranges::output_iterator<gdwg::graph<int, std::string>::iterator,
                                      ranges::common_tuple<int, int, std::string>>);

// [[nodiscard]] auto begin() const -> iterator;
// [[nodiscard]] auto begin() const -> iterator;
// test graph.begin() and graph.end()
TEST_CASE("begin() and end()") {
	SECTION("empty graph: begin() == end()") {
		auto const g = gdwg::graph<int, std::string>{1, 2, 3};
		CHECK(g.begin() == g.end());
	}
	SECTION("g.begin() points to first element") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(1, 1, "pig");
		g.insert_edge(1, 2, "cat");
		g.insert_edge(1, 2, "dog");
		g.insert_edge(2, 1, "monkey");
		CHECK(*g.begin() == ranges::common_tuple<int, int, std::string>{1, 1, "pig"});
	}
	SECTION("(--g.end()) points to last element") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(1, 1, "pig");
		g.insert_edge(1, 2, "cat");
		g.insert_edge(1, 2, "dog");
		g.insert_edge(2, 1, "monkey");
		CHECK(*(ranges::prev(g.end())) == ranges::common_tuple<int, int, std::string>{2, 1, "monkey"});
	}
}

// iterator default constructor
// all default constructed iterator from the same graph type should equal
TEST_CASE("iterator default constructor") {
	auto it1 = gdwg::graph<int, std::string>::iterator();
	auto it2 = gdwg::graph<int, std::string>::iterator();
	CHECK(it1 == it2);
}

// auto operator*() -> ranges::common_tuple<N const&, N const&, E const&>;
// Iterator source already tested in many other tests

// Iterator traversal
// prefix ++ --:
TEST_CASE("prefix ++/--") {
	auto g = gdwg::graph<int, std::string>{1, 2, 3};
	g.insert_edge(1, 1, "pig");
	g.insert_edge(1, 2, "cat");
	g.insert_edge(1, 2, "dog");
	g.insert_edge(2, 1, "monkey");

	auto it1 = g.begin();
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 1, "pig"});
	it1 = ++it1;
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 2, "cat"});

	// iterate through the range
	++it1;
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 2, "dog"});
	++it1;
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{2, 1, "monkey"});
	++it1;
	CHECK(it1 == g.end());

	// iterate back to begin()
	it1 = --it1;
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{2, 1, "monkey"});
	--it1;
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 2, "dog"});
	--it1;
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 2, "cat"});
	--it1;
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 1, "pig"});
	CHECK(it1 == g.begin());
}

// auto operator++(int) -> iterator;
// auto operator--(int) -> iterator;
// postfix ++/--: return a copy of original iterator
TEST_CASE("post fix ++/--") {
	auto g = gdwg::graph<int, std::string>{1, 2, 3};
	g.insert_edge(1, 1, "pig");
	g.insert_edge(1, 2, "cat");
	g.insert_edge(1, 2, "dog");
	g.insert_edge(2, 1, "monkey");

	auto it1 = g.begin();
	it1 = it1++; // this will assign a copy of the original iterator
	CHECK(it1 == g.begin());
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 1, "pig"});

	it1++; // iterate through
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 2, "cat"});

	it1 = it1--; // this will assign a copy of the original iterator
	// it1 will still point to {1, 2, "cat"}
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 2, "cat"});

	it1--; // iterate back
	CHECK(*it1 == ranges::common_tuple<int, int, std::string>{1, 1, "pig"});
}

// auto operator==(iterator const& other) -> bool;
// Returns: true if *this and other are pointing to elements in the same range, and false otherwise.

TEST_CASE("iterator ==") {
	auto g = gdwg::graph<int, std::string>{1, 2, 3};
	g.insert_edge(1, 1, "pig");
	g.insert_edge(1, 2, "cat");
	g.insert_edge(1, 2, "dog");
	g.insert_edge(2, 1, "monkey");

	auto g2 = gdwg::graph<int, std::string>(g);
	// iterator from different container should be different
	CHECK(g.begin() != g2.begin());
	CHECK(g.begin() == g.begin());

	auto it1 = g.begin();
	++++it1; // points to {1, 2, "dog"};
	auto it2 = g.find(1, 2, "dog");
	CHECK(it1 == it2);
}
