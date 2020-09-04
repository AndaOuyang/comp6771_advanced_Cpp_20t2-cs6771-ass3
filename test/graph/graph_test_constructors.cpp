#include "gdwg/graph.hpp"
#include <catch2/catch.hpp>

//-------------------------------------------------------------------------------------------------
//          This file test constructors, copy assignment and move assignment
//-------------------------------------------------------------------------------------------------

// should create an empty graph
TEST_CASE("defualt constructor") {
	auto const g1 = gdwg::graph<int, double>();
	CHECK(g1.empty());
	auto g2 = gdwg::graph<int, double>();
	CHECK(g1 == g2);
}

TEST_CASE("node list constructor") {
	auto const g = gdwg::graph<int, std::string>{1, 3, 5};
	REQUIRE(not g.empty());
	CHECK(g.is_node(1));
	CHECK(g.is_node(3));
	CHECK(g.is_node(5));
	CHECK(not g.is_node(4));
}

TEST_CASE("range constructor of node") {
	auto vec = std::vector<std::string>{"aaa", "bbb", "ccc", "ddd"};
	auto const g = gdwg::graph<std::string, int>(vec.begin(), vec.end());
	REQUIRE(not g.empty());
	CHECK(g.is_node("aaa"));
	CHECK(g.is_node("bbb"));
	CHECK(g.is_node("ccc"));
	CHECK(g.is_node("ddd"));
}

// range constructor of value_type should inset all nodes appears and insert all the valur_types as
// edges
TEST_CASE("range constructor of value_type") {
	auto vec = std::vector<gdwg::graph<int, char>::value_type>{{1, 1, 'a'},
	                                                           {2, 1, 'b'},
	                                                           {2, 1, 'z'},
	                                                           {3, 5, 'c'}};
	auto const g = gdwg::graph<int, char>(vec.begin(), vec.end());

	SECTION("all nodes are inserted") {
		REQUIRE(not g.empty());
		CHECK(g.is_node(1));
		CHECK(g.is_node(2));
		CHECK(g.is_node(3));
		CHECK(g.is_node(5));
		CHECK(g.nodes().size() == 4);
	}

	// g.debug_check_addr_no_repeat();

	SECTION("all edges are connected in outgoing direction, but not ingoing direction") {
		CHECK(g.is_connected(1, 1));
		CHECK(g.is_connected(2, 1));
		CHECK(g.is_connected(3, 5));
		CHECK(not g.is_connected(1, 2));
		CHECK(not g.is_connected(5, 3));
	}

	SECTION("only the specified edges are inserted") {
		auto vec_weight_1_1 = g.weights(1, 1);
		REQUIRE(vec_weight_1_1.size() == 1);
		CHECK(vec_weight_1_1[0] == 'a');

		auto vec_weight_2_1 = g.weights(2, 1);
		REQUIRE(vec_weight_2_1.size() == 2);
		CHECK(vec_weight_2_1[0] == 'b');
		CHECK(vec_weight_2_1[1] == 'z');

		auto vec_weight_3_5 = g.weights(3, 5);
		REQUIRE(vec_weight_3_5.size() == 1);
		CHECK(vec_weight_3_5[0] == 'c');
	}
}

// graph(graph const& other);
// const lvalue copy constructor: *this == other should hold
TEST_CASE("const lvalue copy constructor") {
	auto vec = std::vector<gdwg::graph<int, char>::value_type>{{1, 1, 'a'},
	                                                           {2, 1, 'b'},
	                                                           {2, 1, 'z'},
	                                                           {3, 5, 'c'}};
	auto g1 = gdwg::graph<int, char>(vec.begin(), vec.end());
	auto g2 = gdwg::graph<int, char>(g1);
	CHECK(g1 == g2);
	CHECK(std::addressof(g1) != std::addressof(g2));
	SECTION("modifying one graph won't influence the other") {
		g1.insert_node(99);
		CHECK(g1 != g2);
		g2.insert_node(99);
		CHECK(g1 == g2);
	}
}

// copy assignment: won't change the address of g2 graph container
// other behaviors the same as copy constructor
TEST_CASE("copy assignment") {
	auto vec = std::vector<gdwg::graph<int, char>::value_type>{{1, 1, 'a'},
	                                                           {2, 1, 'b'},
	                                                           {2, 1, 'z'},
	                                                           {3, 5, 'c'}};
	auto g1 = gdwg::graph<int, char>(vec.begin(), vec.end());
	auto g2 = gdwg::graph<int, char>{};
	auto* addr_of_g2_original = std::addressof(g2);
	g2 = g1;
	CHECK(std::addressof(g2) == addr_of_g2_original);
	CHECK(g1 == g2);
	CHECK(std::addressof(g1) != std::addressof(g2));
	SECTION("modifying one graph won't influence the other") {
		g1.insert_node(99);
		CHECK(g1 != g2);
		g2.insert_node(99);
		CHECK(g1 == g2);
	}
}

// graph(graph&& other) noexcept;
// after move constructor,graph will equals original other, and other will become empty
TEST_CASE("move constructor") {
	SECTION("regular case") {
		auto vec = std::vector<gdwg::graph<int, char>::value_type>{{1, 1, 'a'},
		                                                           {2, 1, 'b'},
		                                                           {2, 1, 'z'},
		                                                           {3, 5, 'c'}};
		auto orig = gdwg::graph<int, char>(vec.begin(), vec.end());
		// copy the content for further accessment (move_to == copy)
		auto const copy = orig;
		auto move_to = gdwg::graph<int, char>(std::move(orig));
		CHECK(move_to == copy);

		// NOLINTNEXTLINE(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
		CHECK(orig.empty());
	}
	// iterator in the original graph should be valid in the new graph
	SECTION("test iterator") {
		auto vec = std::vector<gdwg::graph<int, char>::value_type>{{1, 1, 'a'},
		                                                           {2, 1, 'b'},
		                                                           {2, 1, 'z'},
		                                                           {3, 5, 'c'}};
		auto orig = gdwg::graph<int, char>(vec.begin(), vec.end());
		auto it = orig.find(2, 1, 'z');
		REQUIRE(it != orig.end());
		auto move_to = gdwg::graph<int, char>(std::move(orig));
		// iterator still valid
		CHECK(*it == ranges::common_tuple<int, int, char>{2, 1, 'z'});
		// can erase
		REQUIRE(move_to.find(2, 1, 'z') != move_to.end());
		move_to.erase_edge(it);
		CHECK(move_to.find(2, 1, 'z') == move_to.end());
	}
}

// move assignment: won't change the address of the graph container
// behavior the same as move constructor
TEST_CASE("move assignment") {
	SECTION("regular case") {
		auto vec = std::vector<gdwg::graph<int, char>::value_type>{{1, 1, 'a'},
		                                                           {2, 1, 'b'},
		                                                           {2, 1, 'z'},
		                                                           {3, 5, 'c'}};
		auto orig = gdwg::graph<int, char>(vec.begin(), vec.end());
		// copy the content for further accessment (move_to == copy)
		auto const copy = orig;
		auto move_to = gdwg::graph<int, char>{};
		auto* addr_of_move_to_original = std::addressof(move_to);

		move_to = std::move(orig);
		CHECK(move_to == copy);
		CHECK(std::addressof(move_to) == addr_of_move_to_original);

		// NOLINTNEXTLINE(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
		CHECK(orig.empty());
	}

	// iterator in the original graph should be valid in the new graph
	SECTION("test iterator") {
		auto vec = std::vector<gdwg::graph<int, char>::value_type>{{1, 1, 'a'},
		                                                           {2, 1, 'b'},
		                                                           {2, 1, 'z'},
		                                                           {3, 5, 'c'}};
		auto orig = gdwg::graph<int, char>(vec.begin(), vec.end());
		auto it = orig.find(2, 1, 'z');
		REQUIRE(it != orig.end());
		auto move_to = gdwg::graph<int, char>{};

		move_to = std::move(orig);
		// iterator still valid
		CHECK(*it == ranges::common_tuple<int, int, char>{2, 1, 'z'});
		// can erase
		REQUIRE(move_to.find(2, 1, 'z') != move_to.end());
		move_to.erase_edge(it);
		CHECK(move_to.find(2, 1, 'z') == move_to.end());
	}
}
