#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <vector>

#include <CLI/CLI.hpp>

struct config_t {
	std::string editor;
	std::string path;
	std::vector<std::string> projects;
	int projectID;
	bool configured;
	CLI::App* sub_init;
	CLI::Option* conf_option;
	CLI::Option* projects_option;
};

void registerOptions(CLI::App& app, config_t& conf) {

	app.set_config("--config", ".today.conf", "Read an ini file")
		-> transform(CLI::FileOnDefaultPath(getenv("HOME"), false));

	app.add_option("--editor", conf.editor, "editor")
		-> default_val("vim");
	app.add_option("--id", conf.projectID, "project ID")
		-> default_val("0");
	conf.projects_option = app.add_option("--projects", conf.projects, "project list");

	conf.conf_option = app.add_option("--configured", conf.configured, "hidden option")
		-> group("");

	conf.sub_init = app.add_subcommand("init", "initialize working directory");
	conf.sub_init->add_option("path", conf.path, "deploy path")
		-> default_val(".")
		-> configurable(false);

}

namespace commands {
	int init(CLI::App& app, config_t& conf) {
		std::string path = std::filesystem::absolute(conf.path);
		if (std::find(conf.projects.begin(), conf.projects.end(), path) == conf.projects.end()) {

			conf.conf_option->add_result("true");			
			conf.projects_option->add_result(path);

			std::ofstream out(std::string(getenv("HOME")) + "/.today.conf");
			out << app.config_to_str(true, true);
			out.close();

			system(std::string("git init " + path).c_str());

		} else {
			std::cout << "project already registerd!" << std::endl;
			return -1;
		}
		return 0;
	}

	int today(CLI::App& app, config_t& conf) {

		std::string workingDir = conf.projects[conf.projectID];
		std::cout << workingDir << std::endl;
		
		std::cout << "default" << std::endl;
		std::cout << "editor: " << conf.editor << std::endl;
		system(std::string(conf.editor + " " + workingDir + "/today.md").c_str());
		return 0;
	}
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

	if (app.got_subcommand(conf.sub_init))
		return commands::init(app, conf);

	if (!conf.configured) {
		std::cout << "There are no configuration file!" << std::endl;
		std::cout << "You can create it by hand, or just do 'today.md init' to Setup automatically." << std::endl;
		return -1;
	}

	// default
	return commands::today(app, conf);

	// never reach
	return 0;
}
