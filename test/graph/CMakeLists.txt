cxx_test(
   TARGET graph_test1
   FILENAME "graph_test1.cpp"
   LINK absl::flat_hash_set absl::flat_hash_map gsl::gsl-lite-v1 fmt::fmt-header-only range-v3
)


cxx_test(
   TARGET graph_test_constructors
   FILENAME "graph_test_constructors.cpp"
   LINK absl::flat_hash_set absl::flat_hash_map gsl::gsl-lite-v1 fmt::fmt-header-only range-v3
)

cxx_test(
   TARGET graph_test_modifiers
   FILENAME "graph_test_modifiers.cpp"
   LINK absl::flat_hash_set absl::flat_hash_map gsl::gsl-lite-v1 fmt::fmt-header-only range-v3
)

cxx_test(
   TARGET graph_test_accessors
   FILENAME "graph_test_accessors.cpp"
   LINK absl::flat_hash_set absl::flat_hash_map gsl::gsl-lite-v1 fmt::fmt-header-only range-v3
)

cxx_test(
   TARGET graph_test_iterators
   FILENAME "graph_test_iterators.cpp"
   LINK absl::flat_hash_set absl::flat_hash_map gsl::gsl-lite-v1 fmt::fmt-header-only range-v3
)
