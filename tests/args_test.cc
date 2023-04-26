#include "xpano/cli/args.h"

#include <catch2/catch_test_macros.hpp>

#include "tests/utils.h"

TEST_CASE("Args parse empty") {
  auto test_args = xpano::tests::Args("xpano");

  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(args);
  REQUIRE(args->input_paths.empty());
  REQUIRE(!args->output_path);
  REQUIRE(args->run_gui == false);
  REQUIRE(args->print_help == false);
  REQUIRE(args->print_version == false);
}

TEST_CASE("Args parse cli") {
  auto test_args = xpano::tests::Args("xpano", "input1.jpg", "input2.jpg",
                                      "--output=output.jpg");

  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(args);
  REQUIRE(args->input_paths.size() == 2);
  REQUIRE(args->input_paths[0] == "input1.jpg");
  REQUIRE(args->input_paths[1] == "input2.jpg");
  REQUIRE(args->output_path);
  REQUIRE(*args->output_path == "output.jpg");
}

TEST_CASE("Args parse gui") {
  auto test_args =
      xpano::tests::Args("xpano", "input1.jpg", "input2.jpg", "--gui");

  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(args);
  REQUIRE(args->input_paths.size() == 2);
  REQUIRE(args->input_paths[0] == "input1.jpg");
  REQUIRE(args->input_paths[1] == "input2.jpg");
  REQUIRE(!args->output_path);
  REQUIRE(args->run_gui == true);
}

TEST_CASE("Args parse help") {
  auto test_args = xpano::tests::Args("xpano", "--help");

  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(args);
  REQUIRE(args->input_paths.empty());
  REQUIRE(!args->output_path);
  REQUIRE(args->print_help == true);
}

TEST_CASE("Args parse version") {
  auto test_args = xpano::tests::Args("xpano", "--version");

  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(args);
  REQUIRE(args->input_paths.empty());
  REQUIRE(!args->output_path);
  REQUIRE(args->print_version == true);
}

TEST_CASE("Args parse missing inputs") {
  auto test_args = xpano::tests::Args("xpano", "--output=output.jpg");
  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(!args);
}

TEST_CASE("Args parse unsupported output extension") {
  auto test_args = xpano::tests::Args("xpano", "input1.jpg", "input2.jpg",
                                      "--output=output.exr");
  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(!args);
}

TEST_CASE("Args parse unsupported input extension") {
  auto test_args = xpano::tests::Args("xpano", "input1.exr", "input2.jpg",
                                      "input3.jpg", "--output=output.jpg");
  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(args);
  REQUIRE(args->input_paths.size() == 2);
  REQUIRE(args->input_paths[0] == "input2.jpg");
  REQUIRE(args->input_paths[1] == "input3.jpg");
  REQUIRE(args->output_path);
  REQUIRE(*args->output_path == "output.jpg");
}

TEST_CASE("Args parse no supported input") {
  auto test_args = xpano::tests::Args("xpano", "input1.exr", "input2.exr",
                                      "--output=output.jpg");
  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(!args);
}

TEST_CASE("Args parse gui and output incompatible") {
  auto test_args = xpano::tests::Args("xpano", "input1.jpg", "input2.jpg",
                                      "--output=output.jpg", "--gui");
  auto args = xpano::cli::ParseArgs(test_args.GetArgc(), test_args.GetArgv());
  REQUIRE(!args);
}
