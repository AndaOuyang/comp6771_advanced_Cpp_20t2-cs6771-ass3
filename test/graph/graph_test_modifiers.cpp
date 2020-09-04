#include "gdwg/graph.hpp"
#include <catch2/catch.hpp>
#include <vector>

//-------------------------------------------------------------------------------------------------
//                                This file test modifiers
//-------------------------------------------------------------------------------------------------

// auto insert_node(N const& value) -> bool;
// insert the node into graph
// return true if node is inserted. return false if the node was already exist
TEST_CASE("insert node") {
	SECTION("return true if insertion is successful") {
		auto g = gdwg::graph<int, double>();
		CHECK(g.insert_node(1) == true);
		CHECK(g.insert_node(2) == true);
		CHECK(g.insert_node(7) == true);
		CHECK(g.insert_node(8) == true);
		CHECK(g.nodes() == std::vector<int>{1, 2, 7, 8});
	}
	SECTION("return false if node already exist") {
		auto g = gdwg::graph<int, double>{3};
		CHECK(g.insert_node(3) == false);
		CHECK(g.insert_node(1) == true);
		CHECK(g.nodes() == std::vector<int>{1, 3});
		CHECK(g.insert_node(1) == false);
		CHECK(g.nodes() == std::vector<int>{1, 3});
	}
}

// auto insert_edge(N const& src, N const& dst, E const& weight) -> bool;
// insert edge: will insert the edge if possible
// return true if inserted. return false if the edge was already exist
// throw exeption if src or dst doesn't exist
TEST_CASE("insert edge") {
	// after successful insertion, src-->dst will become connected
	SECTION("successful insertion should return true") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		CHECK(g.is_connected(1, 2) == false);
		CHECK(g.insert_edge(1, 2, "dog") == true);
		CHECK(g.is_connected(1, 2) == true);
		CHECK(g.insert_edge(2, 3, "cat") == true);
	}
	// if edge was already in the graph, return false
	SECTION("unsuccessful insertion should return false") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		CHECK(g.insert_edge(1, 2, "dog") == true);
		CHECK(g.insert_edge(1, 2, "dog") == false);
	}
	// even if edge {src, dst, weight1} already exist, the insertion of oppsite directioned edge or
	// with a different weight is valid
	SECTION("insert different weights or different directions") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		CHECK(g.insert_edge(1, 2, "dog") == true);
		CHECK(g.insert_edge(1, 2, "dog") == false);

		CHECK(g.insert_edge(1, 2, "cat") == true); // different weight
		CHECK(g.insert_edge(2, 1, "dog") == true); // different direction

		CHECK(g.weights(1, 2) == std::vector<std::string>{"cat", "dog"});
		CHECK(g.weights(2, 1) == std::vector<std::string>{"dog"});
	}
	// if either src or dst isn't in the graph, should throw exeption
	SECTION("test exeption: src or dst doesn't exist") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		auto const message = std::string("Cannot call gdwg::graph<N, E>::insert_edge when either src "
		                                 "or dst node does not exist");
		REQUIRE_THROWS(g.insert_edge(1, 5, "dog"));
		CHECK_THROWS_MATCHES(g.insert_edge(1, 5, "dog"),
		                     std::runtime_error,
		                     Catch::Matchers::Message(message));
		CHECK_THROWS_MATCHES(g.insert_edge(4, 1, "dog"),
		                     std::runtime_error,
		                     Catch::Matchers::Message(message));
	}
}

// auto replace_node(N const& old_data, N const& new_data) -> bool;
// replace node if old_data exist and new_data doesn't exist
// return true if the replacement is successful
// return false and do nothing if new_data already exist
// throw exeption if old_data doesn't exist

TEST_CASE("replace node") {
	// successful replacement should erase old_node, insert new_node and change every old_node in
	// existing edges to new_node
	SECTION("successful replacement") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(1, 1, "pig");
		g.insert_edge(1, 2, "cat");
		g.insert_edge(1, 2, "dog");
		g.insert_edge(1, 3, "rat");
		g.insert_edge(2, 1, "ox");

		CHECK(g.replace_node(1, 9) == true);
		REQUIRE(g.is_node(9));
		REQUIRE(not g.is_node(1));
		CHECK(g.find(1, 1, "pig") == g.end());

		CHECK(g.find(9, 9, "pig") != g.end());
		CHECK(g.find(9, 2, "cat") != g.end());
		CHECK(g.find(9, 2, "dog") != g.end());
		CHECK(g.find(9, 3, "rat") != g.end());
		CHECK(g.find(2, 9, "ox") != g.end());
	}

	// if new_node already exist, do nothing and return false: old_node should remain in the graph
	SECTION("failed replacement: new node already existed") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(1, 1, "pig");
		g.insert_edge(1, 2, "cat");
		g.insert_edge(1, 2, "dog");
		g.insert_edge(1, 3, "rat");
		g.insert_edge(2, 1, "ox");

		// copy the original to see whether any node or edge changed
		auto const copy = gdwg::graph<int, std::string>(g);

		CHECK(g.replace_node(1, 3) == false);
		REQUIRE(g.is_node(1));
		CHECK(g == copy);
	}

	// should throw exception if old_node doesn't exist
	SECTION("exception: old_node doesn't exist") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		auto const message = std::string("Cannot call gdwg::graph<N, E>::replace_node on a node that "
		                                 "doesn't exist");
		REQUIRE_THROWS(g.replace_node(5, 3));
		CHECK_THROWS_MATCHES(g.replace_node(5, 3),
		                     std::runtime_error,
		                     Catch::Matchers::Message(message));
	}
}

// auto merge_replace_node(N const& old_data, N const& new_data) -> void;
// replace the old node with the new node. erase the old node. revise all the relevant edges as well
// if revised edge duplicate, only keep one
// throw exception if either old node or new node doesn't exist
TEST_CASE("merge replace node") {
	// sucessful replacement: relace 1 with 2
	SECTION("successful replacement") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(1, 1, "pig");
		// both {1, 1, "cat"}, {1, 2, "cat"} and {2, 2, "cat"} will become {2, 2, "cat"}. However, the
		// result will only keep one
		g.insert_edge(1, 1, "cat");
		g.insert_edge(1, 2, "cat");
		g.insert_edge(2, 2, "cat");
		g.insert_edge(1, 3, "rat");
		g.insert_edge(2, 1, "ox");
		g.insert_edge(3, 1, "dog");
		g.insert_edge(3, 2, "fox");

		auto expected_result = gdwg::graph<int, std::string>{2, 3};
		expected_result.insert_edge(2, 2, "pig");
		expected_result.insert_edge(2, 2, "cat");
		expected_result.insert_edge(2, 3, "rat");
		expected_result.insert_edge(2, 2, "ox");
		expected_result.insert_edge(3, 2, "dog");
		expected_result.insert_edge(3, 2, "fox");

		g.merge_replace_node(1, 2);
		CHECK(g == expected_result);
	}

	SECTION("merge replace exception: either old node or new node doesn't exist") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		auto const message = std::string("Cannot call gdwg::graph<N, E>::merge_replace_node on old "
		                                 "or new data if they don't exist in the graph");
		REQUIRE_THROWS(g.merge_replace_node(5, 3));
		CHECK_THROWS_MATCHES(g.merge_replace_node(5, 3),
		                     std::runtime_error,
		                     Catch::Matchers::Message(message));
		CHECK_THROWS_MATCHES(g.merge_replace_node(3, 5),
		                     std::runtime_error,
		                     Catch::Matchers::Message(message));
	}
}

// auto erase_node(N const& value) -> bool;
// erase node and all its ingoing and outgoing edges if exist
// return true if node exist, false elsewise
TEST_CASE("erase node") {
	// erase node 1, should remove all its ingoing and outgoing edges
	SECTION("successful erase") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(1, 1, "pig");
		g.insert_edge(1, 2, "cat");
		g.insert_edge(1, 3, "rat");
		g.insert_edge(2, 1, "ox");
		g.insert_edge(3, 1, "sheep");
		g.insert_edge(3, 2, "monkey");
		g.insert_edge(3, 3, "lion");

		auto expected_result = gdwg::graph<int, std::string>{2, 3};
		expected_result.insert_edge(3, 2, "monkey");
		expected_result.insert_edge(3, 3, "lion");

		CHECK(g.erase_node(1) == true);
		CHECK(g == expected_result);
	}
	// erasing a node with no edge. only the node is erased, no segmentation fault
	SECTION("erase a node with no edge") {
		// node 1 has no edge. will remove node 1
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(3, 2, "monkey");
		g.insert_edge(3, 3, "lion");

		auto expected_result = gdwg::graph<int, std::string>{2, 3};
		expected_result.insert_edge(3, 2, "monkey");
		expected_result.insert_edge(3, 3, "lion");

		REQUIRE(g != expected_result);
		CHECK(g.erase_node(1) == true);
		CHECK(g == expected_result);
	}
	// try to erase a non-existed node, should return false and change nothing
	SECTION("failed erase node") {
		auto g = gdwg::graph<int, std::string>{2, 3};
		g.insert_edge(3, 2, "monkey");
		g.insert_edge(3, 3, "lion");

		auto expected_result = gdwg::graph<int, std::string>(g);

		CHECK(g.erase_node(1) == false);
		CHECK(g == expected_result);
	}
}

// auto erase_edge(N const& src, N const& dst, E const& weight) -> bool;
// erase edge by value
// if the edge exist, erase it and return true
// if the edge doesn't exist, do nothing and return false
// if src or dst doesn't exist, throw exeption
TEST_CASE("erase edge by value: src, dst, weight") {
	// erase {3, 2, "monkey"}, should return true
	SECTION("successful erase") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(3, 2, "monkey");
		g.insert_edge(3, 3, "lion");

		auto expected_result = gdwg::graph<int, std::string>{1, 2, 3};
		expected_result.insert_edge(3, 3, "lion");

		CHECK(g.erase_edge(3, 2, "monkey") == true);
		CHECK(g == expected_result);
	}
	// return false if edge doesn't exist but src and dst exist
	SECTION("failed erase") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		g.insert_edge(3, 2, "monkey");
		g.insert_edge(3, 3, "lion");

		auto expected_result = gdwg::graph<int, std::string>{1, 2, 3};
		expected_result.insert_edge(3, 2, "monkey");
		expected_result.insert_edge(3, 3, "lion");

		CHECK(g.erase_edge(3, 2, "cat") == false);
		CHECK(g == expected_result);
	}
	// throw exeption if src or dst does't exist
	SECTION("erase by value exception: either old node or new node doesn't exist") {
		auto g = gdwg::graph<int, std::string>{1, 2, 3};
		auto const message = std::string("Cannot call gdwg::graph<N, E>::erase_edge on src or dst if "
		                                 "they don't exist in the graph");
		REQUIRE_THROWS(g.erase_edge(5, 3, "cat"));
		CHECK_THROWS_MATCHES(g.erase_edge(5, 3, "cat"),
		                     std::runtime_error,
		                     Catch::Matchers::Message(message));
		CHECK_THROWS_MATCHES(g.erase_edge(3, 5, "cat"),
		                     std::runtime_error,
		                     Catch::Matchers::Message(message));
	}
}

// auto erase_edge(iterator i) -> iterator;
// erase edge by single iterator
// return the iterator pointing to the element after the erased one. if there is no such element,
// return end()
TEST_CASE("erase edge by single iterator") {
	auto g = gdwg::graph<int, std::string>{1, 2, 3};
	g.insert_edge(1, 1, "pig");
	g.insert_edge(1, 2, "cat");
	g.insert_edge(1, 3, "rat");
	g.insert_edge(2, 1, "ox");
	g.insert_edge(3, 1, "sheep");
	g.insert_edge(3, 2, "monkey");
	g.insert_edge(3, 3, "lion");

	// erase in begin()
	auto iter = g.begin();
	REQUIRE(*iter == ranges::common_tuple<int, int, std::string>{1, 1, "pig"});
	iter = g.erase_edge(iter);
	CHECK(*iter == ranges::common_tuple<int, int, std::string>{1, 2, "cat"});
	CHECK(iter == g.begin());

	// erase in middle
	iter = g.find(2, 1, "ox");
	REQUIRE(iter != g.end());
	REQUIRE(*iter == ranges::common_tuple<int, int, std::string>{2, 1, "ox"});
	iter = g.erase_edge(iter);
	REQUIRE(iter != g.end());
	CHECK(*iter == ranges::common_tuple<int, int, std::string>{3, 1, "sheep"});

	// erase last
	iter = g.find(3, 3, "lion");
	REQUIRE(iter != g.end());
	iter = g.erase_edge(iter);
	CHECK(iter == g.end());
}

// auto erase_edge(iterator i, iterator s) -> iterator;
// range erase: erase all edges in the range [i, s)
// return the the iterator pointing to the next element of s
TEST_CASE("erase edge by iterator range") {
	// *i = {1, 3, "rat"}, *s = { 3, 2, "monkey" }
	// erase every edges in between
	auto g = gdwg::graph<int, std::string>{1, 2, 3};
	g.insert_edge(1, 1, "pig");
	g.insert_edge(1, 2, "cat");
	g.insert_edge(1, 3, "rat");
	g.insert_edge(2, 1, "ox");
	g.insert_edge(3, 1, "sheep");
	g.insert_edge(3, 2, "monkey");
	g.insert_edge(3, 3, "lion");

	auto expected_result = gdwg::graph<int, std::string>{1, 2, 3};
	expected_result.insert_edge(1, 1, "pig");
	expected_result.insert_edge(1, 2, "cat");
	expected_result.insert_edge(3, 2, "monkey");
	expected_result.insert_edge(3, 3, "lion");

	auto i = g.find(1, 3, "rat");
	auto s = g.find(3, 2, "monkey");
	auto result_iter = g.erase_edge(i, s);
	REQUIRE(result_iter != g.end());
	CHECK(*result_iter == ranges::common_tuple<int, int, std::string>{3, 2, "monkey"});
	CHECK(g == expected_result);
}

// auto clear() noexcept -> void;
// clear all nodes and edges of the graph
TEST_CASE("clear graph") {
	auto g = gdwg::graph<int, std::string>{1, 2, 3};
	g.insert_edge(1, 1, "pig");
	g.insert_edge(1, 2, "cat");
	g.insert_edge(1, 3, "rat");
	g.insert_edge(2, 1, "ox");
	g.insert_edge(3, 1, "sheep");
	g.insert_edge(3, 2, "monkey");
	g.insert_edge(3, 3, "lion");

	g.clear();
	CHECK(g.empty());
}
