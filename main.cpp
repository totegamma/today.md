#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <ranges>
#include <regex>
#include <map>

#include <CLI/CLI.hpp>

std::string defaultTemplate = 
	#include "defaultTemplate.txt"
;

struct config_t {
	std::string editor;
	std::string path;
	std::string today;
	std::string memo;
	std::vector<std::string> projects;
	std::vector<std::string> reflectSections;
	int projectID;
	int newID;
	bool configured;
	CLI::App* sub_init;
	CLI::App* sub_projects;
	CLI::App* sub_switch;
	CLI::App* sub_memo;
	CLI::App* sub_reflect;
	CLI::Option* projectID_option;
	CLI::Option* conf_option;
	CLI::Option* projects_option;
	CLI::Option* today_option;
	CLI::Option* memo_option;
};

void writeoutConfig(CLI::App& app) {
	std::ofstream out(std::string(getenv("HOME")) + "/.today.conf");
	out << app.config_to_str(true, true);
	out.close();
}

void getDateString(std::string& out) {
	time_t t = time(nullptr);
	const tm* localTime = localtime(&t);
	std::stringstream s;
	s << std::setw(2) << std::setfill('0') << localTime->tm_year - 100;
	s << std::setw(2) << std::setfill('0') << localTime->tm_mon + 1;
	s << std::setw(2) << std::setfill('0') << localTime->tm_mday;
	out = s.str();
}

void getTimeString(std::string& out) {
	time_t t = time(nullptr);
	const tm* localTime = localtime(&t);
	std::stringstream s;
	s << std::setw(2) << std::setfill('0') << localTime->tm_hour;
	s << std::setw(2) << std::setfill('0') << localTime->tm_min;
	out = s.str();
}

void rotate(config_t& conf) {
	std::string path = conf.projects[conf.projectID] + "/" + conf.today;

	if (!std::filesystem::is_directory(path)) {
		std::filesystem::create_directory(path);
	}

	if (std::filesystem::exists(conf.projects[conf.projectID] + "/today.md")) {
		std::filesystem::rename(conf.projects[conf.projectID] + "/today.md", path + "/today.md");
	}
}

void registerOptions(CLI::App& app, config_t& conf) {

	app.set_config("--config", ".today.conf", "Read an ini file")
		-> transform(CLI::FileOnDefaultPath(getenv("HOME"), false));

	app.add_option("--editor", conf.editor, "editor")
		-> default_val("vim");
	conf.projectID_option = app.add_option("--id", conf.projectID, "project ID")
		-> default_val("0");
	conf.projects_option = app.add_option("--projects", conf.projects, "project list");

	conf.conf_option = app.add_option("--configured", conf.configured, "hidden option")
		-> group("");

	std::string todayString; getDateString(todayString);
	conf.today_option = app.add_option("--today", conf.today)
		-> default_val(todayString)
		-> group("");

	conf.sub_init = app.add_subcommand("init", "initialize working directory");
	conf.sub_init->add_option("path", conf.path, "deploy path")
		-> default_val(".")
		-> configurable(false);

	conf.sub_projects = app.add_subcommand("projects", "show projects");

	conf.sub_switch = app.add_subcommand("switch", "switch project");
	conf.sub_switch->add_option("id", conf.newID, "project id to switch to")
		-> configurable(false)
		-> required(true);

	conf.sub_memo = app.add_subcommand("memo", "write memo");
	conf.memo_option = conf.sub_memo->add_option("filename", conf.memo, "memo file name")
		-> configurable(false);

	conf.sub_reflect = app.add_subcommand("reflect", "split today.md to section file");
	conf.sub_reflect->add_option("--sections", conf.reflectSections, "sections to reflect")
		-> default_val("DoNext");
}

void dumpSection(std::string input, std::vector<std::string>& whitelist, std::string projectDir) {

	std::regex re(R"(^# (.*))");
	std::smatch m;

	std::string sectionName = "_head";
	std::map<std::string, std::string> sections;

	for (std::string elem : input | std::views::split(std::views::single('\n')) 
								  | std::views::transform([](auto a) {
										auto b = a | std::views::common;
										return std::string{b.begin(), b.end()};
									})) {
		if (std::regex_match(elem, m, re)) {
			sectionName = m[1].str();
			sections[sectionName] = "";
		} else {
			sections[sectionName] += elem + "\n";
		}
	}

	for (auto elem : whitelist) {
		if (sections.contains(elem)) {
			std::ofstream out(projectDir + "/." + elem);
			out << sections[elem];
			out.close();
		}

	}

}

namespace commands {
	int init(CLI::App& app, config_t& conf) {
		std::string path = std::filesystem::absolute(conf.path);
		if (std::find(conf.projects.begin(), conf.projects.end(), path) == conf.projects.end()) {

			if (!conf.configured) conf.conf_option->add_result("true");
			conf.projects_option->add_result(path);

			system(std::string("git init " + path).c_str());

			std::ofstream out(path + "/.template.md");
			out << defaultTemplate;
			out.close();

			writeoutConfig(app);

		} else {
			std::cout << "project already registerd!" << std::endl;
			return -1;
		}
		return 0;
	}

	int today(CLI::App& app, config_t& conf) {
		std::string projectDir = conf.projects[conf.projectID];

		std::string today; getDateString(today);
		if (conf.today != today) {
			rotate(conf);
			conf.today_option->clear();
			conf.today_option->add_result(today);
			writeoutConfig(app);
		}

		if (!std::filesystem::exists(projectDir + "/today.md")) {
			if (std::filesystem::exists(projectDir + "/.template.md")) {
				std::filesystem::copy_file(projectDir + "/.template.md", projectDir + "/today.md");
			}
		}
		
		system(std::string(conf.editor + " " + projectDir + "/today.md").c_str());
		return 0;
	}

	int memo(CLI::App& app, config_t& conf) {

		std::string todayString; getDateString(todayString);
		std::string path = conf.projects[conf.projectID] + "/" + todayString;

		if (!std::filesystem::is_directory(path)) {
			std::filesystem::create_directory(path);
		}

		std::string timeString; getTimeString(timeString);
		system(std::string(conf.editor + " " + path + "/" + (conf.memo_option->empty() ? timeString : conf.memo) + ".md").c_str());

		return 0;
	}

	int projects(CLI::App& app, config_t& conf) {
		auto output = std::views::iota(0, (int)conf.projects.size())
					| std::views::transform([&](int i){
							if (i == conf.projectID) {
								return "\e[33m[" + std::to_string(i) + "] " + conf.projects[i] + "\e[0m";
							} else {
								return "[" + std::to_string(i) + "] " + conf.projects[i];
							}
						})
					| std::views::common;
		std::copy(output.begin(), output.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
		return 0;
	}

	int switchto(CLI::App& app, config_t& conf) {

		if (conf.newID < 0 || conf.projects.size() <= conf.newID) {
			std::cout << "project ID out of range" << std::endl;
			return -1;
		}

		conf.projectID_option->clear();
		conf.projectID_option->add_result(std::to_string(conf.newID));

		writeoutConfig(app);
		return 0;
	}

	int reflect(CLI::App& app, config_t& conf) {
		std::string projectDir = conf.projects[conf.projectID];

		if (std::filesystem::exists(projectDir + "/today.md")) {
			std::ifstream t(projectDir + "/today.md");
			std::stringstream buffer;
			buffer << t.rdbuf();
			dumpSection(buffer.str(), conf.reflectSections, projectDir);
		}

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

	if (app.got_subcommand(conf.sub_projects)) {
		return commands::projects(app, conf);
	}
	if (app.got_subcommand(conf.sub_switch)) {
		return commands::switchto(app, conf);
	}
	if (app.got_subcommand(conf.sub_memo)) {
		return commands::memo(app, conf);
	}
	if (app.got_subcommand(conf.sub_reflect)) {
		return commands::reflect(app, conf);
	}

	// default
	return commands::today(app, conf);

	// never reach
	return 0;
}
