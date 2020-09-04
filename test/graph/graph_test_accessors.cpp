#include "gdwg/graph.hpp"
#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <string>
#include <vector>

//-------------------------------------------------------------------------------------------------
//                             test accessors, comparisons, extractor
//-------------------------------------------------------------------------------------------------

// [[nodiscard]] auto is_node(N const& value) -> bool;
// check whether a node is inside a graph
TEST_CASE("is_node()") {
	auto g = gdwg::graph<int, std::string>{1, 2, 3};
	CHECK(g.is_node(1) == true);
	CHECK(g.is_node(0) == false);
}

// [[nodiscard]] auto empty() -> bool;
// check whether a graph is empty: has no node nor edge
TEST_CASE("empty()") {
	auto g = gdwg::graph<int, std::string>{};
	CHECK(g.empty());
	g.insert_node(5);
	CHECK(not g.empty());
	g.erase_node(5);
	CHECK(g.empty());
}

// [[nodiscard]] auto is_connected(N const& src, N const& dst) -> bool;
// check whether there is an edge from src to fst
// throw exception if either src or dst doesn't exist

TEST_CASE("is_connected") {
	SECTION("regular case") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(1, 1, "pig");
		g.insert_edge(1, 2, "cat");
		g.insert_edge(1, 2, "dog");
		g.insert_edge(1, 3, "rat");

		CHECK(g.is_connected(1, 2));
		CHECK(not g.is_connected(2, 1));
		g.erase_edge(1, 2, "cat");
		CHECK(g.is_connected(1, 2));
		g.erase_edge(1, 2, "dog");
		CHECK(not g.is_connected(1, 2));
	}
	// if src or dst doesn't exist, throw exception
	SECTION("exception") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		auto const message = std::string("Cannot call gdwg::graph<N, E>::is_connected if src or dst "
		                                 "node don't exist in the graph");
		REQUIRE_THROWS(g.is_connected(5, 3));
		CHECK_THROWS_MATCHES(g.is_connected(5, 3),
		                     std::runtime_error,
		                     Catch::Matchers::Message(message));
	}
}

// [[nodiscard]] auto nodes() -> std::vector<N>;
// return a vector containing all nodes
TEST_CASE("nodes()") {
	// empty graph should return empty vector
	SECTION("empty graph") {
		auto const g = gdwg::graph<int, std::string>{};
		CHECK(g.nodes().empty());
	}
	// regular case: should return nodes in order
	// should change nodes after insertion of deletion
	SECTION("regular case") {
		auto g = gdwg::graph<int, std::string>{1, 5, 3, 2, 4};
		CHECK(g.nodes() == std::vector<int>{1, 2, 3, 4, 5});
		g.erase_node(3);
		CHECK(g.nodes() == std::vector<int>{1, 2, 4, 5});
		g.insert_node(7);
		CHECK(g.nodes() == std::vector<int>{1, 2, 4, 5, 7});
	}
}

// [[nodiscard]] auto weights(N const& src, N const& dst) -> std::vector<E>;
// return all the weights from src to dst
// throw exception if src or dsr not exist
TEST_CASE("weights(): get the vector of weights from src to dst") {
	SECTION("regular case") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(1, 1, "pig");
		g.insert_edge(1, 2, "cat");
		g.insert_edge(1, 2, "dog");
		g.insert_edge(2, 1, "monkey");

		CHECK(g.weights(1, 2) == std::vector<std::string>{"cat", "dog"});
	}
	// throw exception if src or dst not exist
	SECTION("exception") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		auto const message = std::string("Cannot call gdwg::graph<N, E>::weights if src or dst node "
		                                 "don't exist in the graph");
		REQUIRE_THROWS(g.weights(5, 3));
		CHECK_THROWS_MATCHES(g.weights(5, 3), std::runtime_error, Catch::Matchers::Message(message));
	}
}

// [[nodiscard]] auto find(N const& src, N const& dst, E const& weight) -> iterator;
// find iterator pointing to the edge {src, dst, weight}
// return graph.end() if not found
TEST_CASE("find") {
	auto g = gdwg::graph<int, std::string>{1, 2, 3};
	g.insert_edge(1, 1, "pig");
	g.insert_edge(1, 2, "cat");
	g.insert_edge(1, 2, "dog");
	g.insert_edge(2, 1, "monkey");
	auto const g2 = gdwg::graph<int, std::string>(g);

	// find first element
	auto it = g2.find(1, 1, "pig");
	CHECK(it == g2.begin());
	CHECK(*it == ranges::common_tuple<int, int, std::string>{1, 1, "pig"});

	// find in middle
	it = g2.find(1, 2, "dog");
	CHECK(*it == ranges::common_tuple<int, int, std::string>{1, 2, "dog"});

	// not found ==> should return g2.end()
	it = g2.find(1, 2, "pig");
	CHECK(it == g2.end());
}

// [[nodiscard]] auto connections(N const& src) -> std::vector<N>;
// Returns: A sequence of nodes (found from any immediate outgoing edge) connected to src, sorted in
// ascending order, with respect to the connected nodes.
// throw exception if src not exist
TEST_CASE("connections") {
	// return vector of connected nodes by outgoing edges in ascending order
	// repeated node should show only once
	SECTION("regular case") {
		auto g = gdwg::graph<std::string, int>{"sydney", "melbourn", "brisbane", "perth", "wollongong"};
		g.insert_edge("sydney", "melbourn", 5);
		// repeated brisbane
		g.insert_edge("sydney", "brisbane", 3);
		g.insert_edge("sydney", "brisbane", 4);
		g.insert_edge("sydney", "wollongong", 1);
		g.insert_edge("perth", "sydney", 15);
		auto const g2 = gdwg::graph<std::string, int>(g);

		CHECK(g2.connections("sydney")
		      == std::vector<std::string>{"brisbane", "melbourn", "wollongong"});
	}
	// throw exception if src not exist
	SECTION("exception") {
		auto g = gdwg::graph<std::string, int>{"sydney", "melbourn", "brisbane", "perth", "wollongong"};
		auto const message = std::string("Cannot call gdwg::graph<N, E>::connections if src doesn't "
		                                 "exist in the graph");
		REQUIRE_THROWS(g.connections("adelaide"));
		CHECK_THROWS_MATCHES(g.connections("adelaide"),
		                     std::runtime_error,
		                     Catch::Matchers::Message(message));
	}
}

// comparison
// [[nodiscard]] auto operator==(graph const& other) -> bool;
// return true iff all nodes and edges in 2 graphs are equal
TEST_CASE("operator == ") {
	auto g1 = gdwg::graph<int, std::string>{1, 2, 3};
	g1.insert_edge(1, 1, "pig");
	g1.insert_edge(1, 2, "cat");
	g1.insert_edge(1, 2, "dog");
	g1.insert_edge(2, 1, "monkey");

	auto g2 = gdwg::graph<int, std::string>(g1);
	REQUIRE(g1 == g2);

	// inser a node to g2, then the 2 graphs should not equal
	g2.insert_node(5);
	CHECK(g1 != g2);
	g1.insert_node(6);
	CHECK(g1 != g2);
	g1.insert_node(5);
	g2.insert_node(6);
	CHECK(g1 == g2);

	// insert an edge to g2, then the 2 graphs should not equal
	g2.insert_edge(1, 5, "fox");
	CHECK(g1 != g2);
	// insert edge in opposite direction, not equal as well
	g1.insert_edge(5, 1, "fox");
	CHECK(g1 != g2);

	g2.insert_edge(5, 1, "fox");
	g1.insert_edge(1, 5, "fox");
	REQUIRE(g1 == g2);
}

// extractor
// friend auto operator<<(std::ostream& os, graph const& g) -> std::ostream&;
// example case in the assignment specification
TEST_CASE("Extractor << ") {
	using graph = gdwg::graph<int, int>;
	auto const v = std::vector<graph::value_type>{
	   {4, 1, -4},
	   {3, 2, 2},
	   {2, 4, 2},
	   {2, 1, 1},
	   {6, 2, 5},
	   {6, 3, 10},
	   {1, 5, -1},
	   {3, 6, -8},
	   {4, 5, 3},
	   {5, 2, 7},
	};

	auto g = graph(v.begin(), v.end());
	g.insert_node(64);
	auto const expected_output = std::string_view(R"(1 (
  5 | -1
)
2 (
  1 | 1
  4 | 2
)
3 (
  2 | 2
  6 | -8
)
4 (
  1 | -4
  5 | 3
)
5 (
  2 | 7
)
6 (
  2 | 5
  3 | 10
)
64 (
)
)");
	CHECK(fmt::format("{}", g) == expected_output);
}