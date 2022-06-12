#include <iostream>

#include <CLI/CLI.hpp>

int main(int argc, char** argv) {

	CLI::App app{"today.md: document based todo management tool"};

	CLI::App* sub_init = app.add_subcommand("init", "initialize working directory");

	std::string path;
	sub_init->add_option("path", path, "deploy path")
		-> check(CLI::ExistingDirectory);

	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError &e) {
		return app.exit(e);
	}

	if (sub_init) {
		std::cout << "subcommand: init" << std::endl;
		std::cout << "path: " << path << std::endl;
	} else {
		std::cout << "default" << std::endl;
		std::cout << "path: " << path << std::endl;
	}

	return 0;
}
