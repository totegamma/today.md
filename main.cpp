#include <iostream>
#include <cstdlib>
#include <CLI/CLI.hpp>

struct config_t {
	std::string editor;
	std::string workingDir;
	std::string path;
	int projectID;
	bool configured;
	CLI::App* sub_init;
	CLI::Option* conf_option;
};

void registerOptions(CLI::App& app, config_t& conf) {

	app.set_config("--config", ".today.conf", "Read an ini file")
		-> transform(CLI::FileOnDefaultPath(getenv("HOME")));

	CLI::Option* editor_option = 
	app.add_option("--editor", conf.editor, "editor")
		-> required(true);

	app.add_option("--workingDir", conf.workingDir, "project folder")
		-> required(true);

	app.add_option("--id", conf.projectID, "project ID")
		-> default_str("0");

	conf.conf_option = app.add_option("--configured", conf.configured, "hidden option")
		-> group("");

	conf.sub_init = app.add_subcommand("init", "initialize working directory");
	conf.sub_init->add_option("path", conf.path, "deploy path")
		-> check(CLI::ExistingDirectory)
		-> configurable(false);

}

int main(int argc, char** argv) {

	CLI::App app{"today.md: document based todo management tool"};
	config_t conf;

	registerOptions(app, conf);

	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError &e) {
		return app.exit(e);
	}

	if (!conf.configured) {
		std::cout << "not configured" << std::endl;
		conf.conf_option->add_result("true");

		std::ofstream out(std::string(getenv("HOME")) + "/.today.conf");
		out << app.config_to_str(true, true);
		out.close();
		return 0;
	}

	if (app.got_subcommand(conf.sub_init)) {
		std::cout << "subcommand: init" << std::endl;
		std::cout << "path: " << conf.path << std::endl;
	} else {
		std::cout << "default" << std::endl;
		std::cout << "editor: " << conf.editor << std::endl;
		//system("vim test.txt");
	}


	return 0;
}
