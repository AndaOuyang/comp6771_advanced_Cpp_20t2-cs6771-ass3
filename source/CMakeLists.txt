cxx_executable(
   TARGET "client"
   FILENAME "client.cpp"
   LINK absl::flat_hash_set absl::flat_hash_map gsl::gsl-lite-v1 fmt::fmt-header-only range-v3
)
